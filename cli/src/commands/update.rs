use std::convert::identity;
use std::ops::Sub;

use argh::FromArgs;
use chrono::{Local, Utc};
use graphql_client::GraphQLQuery;
use itertools::{Either, Itertools};
use serde_json::json;

use macros::{AggregateErrors, SendRequest};

use crate::bookmarks::get_bookmarks;
use crate::config::{CONFIG, SyncBookmarks};
use crate::hardcover::send_request;
use crate::hardcover::{
  bigint, date, jsonb, timestamptz, update_or_insert_user_book, update_user_book::UserBookUpdateInput,
};
use crate::isbn::get_isbn;
use crate::utils::{VERSION, debug_log, log};

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updateread.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateRead;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertread.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertRead;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertreadingjournal.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertReadingJournal;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updatejournal.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateJournal;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getjournalquotes.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct GetJournalQuotes;

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

  let mut bookmarks = get_bookmarks(args.content_id)?;

  log(format!("{} bookmarks", bookmarks.len()))?;

  if bookmarks.is_empty() {
    return Ok(());
  }

  debug_log(&format!("{:?}", bookmarks))?;

  let reading_journals = GetJournalQuotes::send_request(get_journal_quotes::Variables {
    book_id: result.book_id,
    user_id: result.user_id,
  })
  .await?
  .reading_journals;

  match CONFIG.sync_bookmarks {
    SyncBookmarks::Finished => {
      bookmarks.sort_by(|a, b| a.location.unwrap_or(0.0).total_cmp(&b.location.unwrap_or(0.0)));
    }
    _ => {
      bookmarks.sort_by(|a, b| a.date_created.cmp(&b.date_created));
    }
  }

  let (insert_bookmarks, update_bookmarks): (Vec<_>, Vec<_>) = bookmarks.iter().partition_map(|bookmark| {
    let note = bookmark.annotation.as_deref().map(str::trim);
    let quote = bookmark.text.trim();
    let entry = if let Some(note) = note
      && !note.is_empty()
    {
      format!("{quote}\n━━━\n{note}")
    } else {
      quote.to_string()
    };

    if let Some(journal) = reading_journals
      .iter()
      .find(|journal| journal.action_at.sub(bookmark.date_created).num_seconds() == 0)
    {
      if journal.entry.as_ref() != Some(&entry) {
        Either::Right(Some(update_journal::Variables {
          journal_id: journal.id,
          entry,
        }))
      } else {
        Either::Right(None)
      }
    } else {
      Either::Left(insert_reading_journal::Variables {
        book_id: result.book_id,
        edition_id: result.edition_id,
        event: "quote".into(),
        privacy_setting_id: CONFIG.journal_privacy as i64,
        entry,
        action_at: Some(
          match CONFIG.sync_bookmarks {
            SyncBookmarks::Finished => Utc::now(),
            _ => bookmark.date_created,
          }
          .format("%+")
          .to_string(),
        ),
        metadata: bookmark.location.map(|location| {
          json!({
            "position": {
              "page": (result.pages as f64 * location).round() as i64,
              "possible": result.pages,
              "percent": location * 100.0,
            }
          })
        }),
      })
    }
  });

  let insert_body = insert_bookmarks
    .into_iter()
    .map(InsertReadingJournal::build_query)
    .collect::<Vec<_>>();

  if !insert_body.is_empty() {
    log(format!(
      "Insert {} quotes for book `{}` and edition `{}`",
      insert_body.len(),
      result.book_id,
      result.edition_id,
    ))?;
    send_request::<_, Vec<graphql_client::Response<insert_reading_journal::ResponseData>>>(
      insert_reading_journal::OPERATION_NAME,
      insert_body,
    )
    .await?;
  }

  let update_body = update_bookmarks
    .into_iter()
    .filter_map(identity)
    .map(UpdateJournal::build_query)
    .collect::<Vec<_>>();

  if !update_body.is_empty() {
    log(format!(
      "Update quotes `{}`",
      update_body.iter().map(|q| q.variables.journal_id).join(", ")
    ))?;
    send_request::<_, Vec<graphql_client::Response<update_journal::ResponseData>>>(
      update_journal::OPERATION_NAME,
      update_body,
    )
    .await?;
  }

  Ok(())
}
