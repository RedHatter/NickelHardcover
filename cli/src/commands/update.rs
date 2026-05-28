use argh::FromArgs;
use chrono::Local;
use graphql_client::GraphQLQuery;
use serde_json::json;

use macros::{AggregateErrors, SendRequest};

use crate::bookmarks::get_bookmarks;
use crate::config::{CONFIG, SyncBookmarks};
use crate::hardcover::{
  bigint, date, jsonb, timestamptz, update_or_insert_user_book, update_user_book::UserBookUpdateInput,
};
use crate::isbn::get_isbn;
use crate::utils::{VERSION, debug_log, log};

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateRead;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertRead;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertReadingJournal;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateJournal;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
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
  log(format!("{} {:?}", &*VERSION, args))?;

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

  let progress_pages = (result.pages * args.value) / 100;

  if let Some(user_read_id) = result.user_read_id {
    log(format!(
      "Update read `{user_read_id}` for edition `{}` to page `{progress_pages}`",
      result.edition_id
    ))?;

    UpdateRead::send_request(update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id: result.edition_id,
      started_at: result.started_at.unwrap_or(Local::now().format("%Y-%m-%d").to_string()),
    })
    .await?;
  } else {
    log(format!(
      "Insert new read for edition `{}` at page `{progress_pages}`",
      result.edition_id
    ))?;

    InsertRead::send_request(insert_read::Variables {
      user_book_id: result.user_book_id,
      progress_pages,
      edition_id: result.edition_id,
      started_at: result.started_at.unwrap_or(Local::now().format("%Y-%m-%d").to_string()),
    })
    .await?;
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
  let mut bookmarks = get_bookmarks(args.content_id, after)?;

  match CONFIG.sync_bookmarks {
    SyncBookmarks::Finished => {
      bookmarks.sort_by(|a, b| a.location.unwrap_or(0.0).total_cmp(&b.location.unwrap_or(0.0)));
    }
    _ => {
      bookmarks.sort_by(|a, b| a.date_created.cmp(&b.date_created));
    }
  }

  log(format!("{} bookmarks", bookmarks.len()))?;

  debug_log(&format!("{:?}", bookmarks))?;

  for bookmark in bookmarks.iter() {
    let reading_journals = if args.after.clone().is_some_and(|after| bookmark.date_created < after) {
      GetJournal::send_request(get_journal::Variables {
        user_id: result.user_id,
        action_at: bookmark.date_created.clone(),
      })
      .await?
      .reading_journals
    } else {
      Vec::new()
    };

    let entry = if let Some(entry) = bookmark.annotation.as_ref()
      && !entry.trim().is_empty()
    {
      format!("{}\n━━━\n{}", bookmark.text.trim(), entry.trim())
    } else {
      bookmark.text.clone()
    };
    let action_at = match CONFIG.sync_bookmarks {
      SyncBookmarks::Finished => Some(Local::now().format("%+").to_string()),
      _ => Some(bookmark.date_created.clone()),
    };
    let metadata = match bookmark.location.map(|location| {
      json!({
        "position": {
          "page": (result.pages as f64 * location).round() as i64,
          "possible": result.pages,
          "percent": location * 100.0,
        }
      })
    }) {
      Some(serde_json::Value::Object(obj)) => Some(obj),
      _ => None,
    };

    insert_or_update_journal(
      insert_reading_journal::Variables {
        book_id: result.book_id,
        edition_id: result.edition_id,
        event: "quote".into(),
        privacy_setting_id: CONFIG.journal_privacy as i64,
        entry,
        action_at: action_at.clone(),
        metadata,
      },
      &reading_journals,
    )
    .await?;
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

      UpdateJournal::send_request(update_journal::Variables {
        journal_id: journal.id,
        entry: object.entry,
      })
      .await?;
    } else {
      log(format!("Skipping `{}` with id `{}`", object.event, journal.id))?;
    }
  } else {
    log(format!(
      "Insert `{}` for book `{}` and edition `{}`",
      object.event, object.book_id, object.edition_id,
    ))?;

    InsertReadingJournal::send_request(object).await?;
  }

  Ok(())
}
