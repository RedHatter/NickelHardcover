use std::convert::identity;
use std::ops::Sub;

use anyhow::Result;
use argh::FromArgs;
use chrono::Utc;
use graphql_client::GraphQLQuery;
use itertools::{Either, Itertools};
use serde_json::json;

use macros::{AggregateErrors, SendRequest};

use crate::bookmarks::get_bookmarks;
use crate::commands::getuser::get_user;
use crate::config::{CONFIG, SyncBookmarks};
use crate::hardcover::{bigint, date, get_book, jsonb, send_request, timestamptz};
use crate::utils::{VERSION, normalize_identifiers};
use crate::{debug_log, log};

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
  query_path = "src/graphql/mutations/updatereadingjournal.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateReadingJournal;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getjournalquotes.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct GetJournalQuotes;

/// Create journal entries for bookmarks.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update-journal")]
pub struct UpdateJournal {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: String,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,
}

pub async fn run(args: UpdateJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (book_id, isbn) = normalize_identifiers(args.book_id, Some(&args.content_id));
  let (_, edition_id, pages) = get_book(isbn, book_id).await?;
  update_journal(&args.content_id, book_id, edition_id, pages).await?;

  Ok(())
}

pub async fn update_journal(content_id: &str, book_id: i64, edition_id: i64, pages: i64) -> Result<()> {
  let mut bookmarks = get_bookmarks(content_id)?;

  log!("{} bookmarks", bookmarks.len());

  if bookmarks.is_empty() {
    return Ok(());
  }

  debug_log!("{:?}", bookmarks);

  let user_id = get_user().await?.id;

  let reading_journals = GetJournalQuotes::send_request(get_journal_quotes::Variables { book_id, user_id })
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

  let privacy_setting_id = CONFIG.journal_privacy.get_value().await?;

  let (insert_bookmarks, update_bookmarks): (Vec<_>, Vec<_>) = bookmarks.iter().partition_map(|bookmark| {
    let note = bookmark.annotation.as_deref().map(str::trim);
    let highlight = bookmark.text.trim();
    let entry = if let Some(note) = note
      && !note.is_empty()
    {
      format!("{highlight}\n━━━\n{note}")
    } else {
      highlight.to_string()
    };

    if let Some(journal) = reading_journals
      .iter()
      .find(|journal| journal.action_at.sub(bookmark.date_created).num_seconds() == 0)
    {
      if journal.entry.as_ref() != Some(&entry) {
        Either::Right(Some(update_reading_journal::Variables {
          journal_id: journal.id,
          entry,
        }))
      } else {
        Either::Right(None)
      }
    } else {
      Either::Left(insert_reading_journal::Variables {
        book_id,
        edition_id,
        event: "quote".into(),
        privacy_setting_id,
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
              "page": (pages as f64 * location).round() as i64,
              "possible": pages,
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
    log!(
      "Insert {} quotes for book `{}` and edition `{}`",
      insert_body.len(),
      book_id,
      edition_id,
    );
    send_request::<_, Vec<graphql_client::Response<insert_reading_journal::ResponseData>>>(
      insert_reading_journal::OPERATION_NAME,
      insert_body,
    )
    .await?;
  }

  let update_body = update_bookmarks
    .into_iter()
    .filter_map(identity)
    .map(UpdateReadingJournal::build_query)
    .collect::<Vec<_>>();

  if !update_body.is_empty() {
    log!(
      "Update quotes `{}`",
      update_body.iter().map(|q| q.variables.journal_id).join(", ")
    );
    send_request::<_, Vec<graphql_client::Response<update_reading_journal::ResponseData>>>(
      update_reading_journal::OPERATION_NAME,
      update_body,
    )
    .await?;
  }

  Ok(())
}
