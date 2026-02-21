use argh::FromArgs;
use chrono::Local;
use graphql_client::GraphQLQuery;

use crate::bookmarks::get_bookmarks;
use crate::config::{CONFIG, SyncBookmarks, log};
use crate::hardcover::{
  bigint, date, send_request, timestamptz, update_or_insert_user_book, update_user_book::UserBookUpdateInput,
};
use crate::isbn::get_isbn;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct UpdateRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct InsertReadingJournal;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct UpdateJournal;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct GetJournal;

/// Update read percentage and create journal entries for bookmarks.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update")]
pub struct Update {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: String,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// read percentage
  #[argh(option)]
  value: i64,

  /// process bookmarks created after this ISO 8601 date
  #[argh(option)]
  after: Option<String>,
}

pub async fn run(args: Update) -> Result<(), String> {
  log(format!("{:?}", args))?;

  let isbn = if args.book_id.is_none() {
    get_isbn(&args.content_id)
  } else {
    Vec::new()
  };

  let result = update_or_insert_user_book(
    isbn,
    args.book_id.unwrap_or(0),
    UserBookUpdateInput {
      status_id: Some(2),
      ..UserBookUpdateInput::default()
    },
  )
  .await?;

  let user_read_id = result
    .user_read_id
    .ok_or("Failed to find user read after updating or inserting user book")?;
  let progress_pages = (result.pages * args.value) / 100;

  log(format!(
    "Update read `{user_read_id}` for edition `{}` to page `{progress_pages}`",
    result.edition_id
  ))?;

  let res = send_request::<update_read::Variables, update_read::ResponseData>(UpdateRead::build_query(
    update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id: result.edition_id,
      started_at: result.started_at.unwrap_or(Local::now().format("%Y-%m-%d").to_string()),
    },
  ))
  .await?;

  if let Some(error) = res.update_user_book_read.and_then(|res| res.error) {
    return Err(error);
  }

  if CONFIG.sync_bookmarks == SyncBookmarks::Never
    || (CONFIG.sync_bookmarks == SyncBookmarks::Finished && args.value != 100)
  {
    return Ok(());
  }

  let after = match CONFIG.sync_bookmarks {
    SyncBookmarks::Finished => None,
    _ => args.after.as_ref(),
  };
  let bookmarks = get_bookmarks(args.content_id, after)?;

  log(format!("{} bookmarks", bookmarks.len()))?;

  for bookmark in bookmarks.iter() {
    let reading_journals = if args.after.clone().is_some_and(|after| bookmark.date_created < after) {
      send_request::<get_journal::Variables, get_journal::ResponseData>(GetJournal::build_query(
        get_journal::Variables {
          user_id: result.user_id,
          action_at: bookmark.date_created.clone(),
        },
      ))
      .await?
      .reading_journals
    } else {
      Vec::new()
    };

    let page = (result.pages as f64 * bookmark.location).round() as i64;
    let percent = bookmark.location * 100.0;
    let action_at = match CONFIG.sync_bookmarks {
      SyncBookmarks::Finished => None,
      _ => Some(bookmark.date_created.clone()),
    };

    insert_or_update_journal(
      insert_reading_journal::Variables {
        book_id: result.book_id,
        edition_id: result.edition_id,
        event: "quote".into(),
        entry: bookmark.text.clone(),
        action_at: action_at.clone(),
        page,
        possible: result.pages,
        percent,
      },
      &reading_journals,
    )
    .await?;

    if let Some(entry) = bookmark.annotation.clone()
      && !entry.is_empty()
    {
      insert_or_update_journal(
        insert_reading_journal::Variables {
          book_id: result.book_id,
          edition_id: result.edition_id,
          event: "note".into(),
          entry,
          action_at: action_at.clone(),
          page,
          possible: result.pages,
          percent,
        },
        &reading_journals,
      )
      .await?;
    }
  }

  Ok(())
}

pub async fn insert_or_update_journal(
  object: insert_reading_journal::Variables,
  reading_journals: &Vec<get_journal::GetJournalReadingJournals>,
) -> Result<(), String> {
  if let Some(journal) = reading_journals
    .iter()
    .find(|journal| journal.event.as_ref() == Some(&object.event))
  {
    if journal.entry.as_ref() != Some(&object.entry) {
      log(format!("Update `{}` with id `{}`", object.event, journal.id))?;

      let res = send_request::<update_journal::Variables, update_journal::ResponseData>(UpdateJournal::build_query(
        update_journal::Variables {
          journal_id: journal.id,
          entry: object.entry,
        },
      ))
      .await?;

      if let Some(errors) = res.update_reading_journal.and_then(|res| res.errors) {
        return Err(
          errors
            .iter()
            .filter_map(|error| error.as_deref())
            .collect::<Vec<&str>>()
            .join("<br>"),
        );
      }
    } else {
      log(format!("Skipping `{}` with id `{}`", object.event, journal.id))?;
    }
  } else {
    log(format!(
      "Insert `{}` for book `{}` and edition `{}`",
      object.event, object.book_id, object.edition_id,
    ))?;

    let res = send_request::<insert_reading_journal::Variables, insert_reading_journal::ResponseData>(
      InsertReadingJournal::build_query(object),
    )
    .await?;

    if let Some(errors) = res.insert_reading_journal.and_then(|res| res.errors) {
      return Err(
        errors
          .iter()
          .filter_map(|error| error.as_deref())
          .collect::<Vec<&str>>()
          .join("<br>"),
      );
    }
  }

  Ok(())
}
