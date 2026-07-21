use std::ops::Sub;

use anyhow::Result;
use argh::FromArgs;
use chrono::{Duration, Utc};
use graphql_client::GraphQLQuery;
use itertools::{Either, Itertools};
use serde_json::json;

use macros::AggregateErrors;

use crate::commands::getuser::get_user;
use crate::commands::getuserbook::get_book;
use crate::config::{CONFIG, SyncBookmarks};
use crate::database::{Bookmark, get_bookmarks};
use crate::hardcover::send_request;
use crate::utils::{GraphQLQueryExt, VERSION, normalize_identifiers};
use crate::{debug_log, log};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertreadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertReadingJournal;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updatereadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateReadingJournal;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getjournalquotes.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
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

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,
}

pub fn run(args: &UpdateJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, Some(&args.content_id));
  let (book, edition_id, pages) = get_book(isbn, linked_id)?;
  update_journal(&args.content_id, book.id, edition_id, pages)?;

  Ok(())
}

pub fn update_journal(content_id: &str, book_id: i64, edition_id: i64, pages: i64) -> Result<()> {
  let mut bookmarks = get_bookmarks(content_id)?;

  log!("{} bookmarks", bookmarks.len())?;

  if bookmarks.is_empty() {
    return Ok(());
  }

  debug_log!("{:?}", bookmarks)?;

  let user_id = get_user()?.id;

  let reading_journals = if CONFIG.sync_bookmarks == SyncBookmarks::Finished {
    bookmarks.sort_by(|a, b| a.location.unwrap_or(0.0).total_cmp(&b.location.unwrap_or(0.0)));
    vec![]
  } else {
    bookmarks.sort_by_key(|bookmark| bookmark.date_created);

    let mut offset = 0;
    let mut reading_journals = Vec::new();

    loop {
      let entries = GetJournalQuotes::send_request(get_journal_quotes::Variables {
        book_id,
        user_id,
        offset,
      })?
      .reading_journals;
      let len = entries.len();
      reading_journals.extend(entries);

      if len < 100 {
        break;
      }

      offset += 100;
    }

    reading_journals
  };

  let privacy_setting_id = CONFIG.journal_privacy.get_value()?;

  let (insert_bookmarks, update_bookmarks): (Vec<_>, Vec<_>) =
    bookmarks.iter().enumerate().partition_map(|(i, bookmark)| {
      build_journal_quote(
        i,
        bookmark,
        reading_journals
          .iter()
          .find(|journal| journal.action_at.sub(bookmark.date_created).num_seconds() == 0),
        book_id,
        edition_id,
        privacy_setting_id,
        pages,
      )
    });

  if !insert_bookmarks.is_empty() {
    log!(
      "Insert {} quotes for book `{}` and edition `{}`",
      insert_bookmarks.len(),
      book_id,
      edition_id,
    )?;
    debug_log!("InsertReadingJournal, {:?}", insert_bookmarks)?;

    for chunk in insert_bookmarks
      .into_iter()
      .map(InsertReadingJournal::build_query)
      .collect::<Vec<_>>()
      .chunks(5)
    {
      send_request::<_, Vec<graphql_client::Response<insert_reading_journal::ResponseData>>>(
        insert_reading_journal::OPERATION_NAME,
        chunk,
      )?;
    }
  }

  let update_bookmarks = update_bookmarks.into_iter().flatten().collect::<Vec<_>>();

  if !update_bookmarks.is_empty() {
    log!(
      "Update {} quotes for book `{}` and edition `{}`",
      update_bookmarks.len(),
      book_id,
      edition_id,
    )?;
    debug_log!("UpdateReadingJournal, {:?}", update_bookmarks)?;

    for chunk in update_bookmarks
      .into_iter()
      .map(UpdateReadingJournal::build_query)
      .collect::<Vec<_>>()
      .chunks(5)
    {
      send_request::<_, Vec<graphql_client::Response<update_reading_journal::ResponseData>>>(
        update_reading_journal::OPERATION_NAME,
        chunk,
      )?;
    }
  }

  Ok(())
}

fn build_journal_quote(
  i: usize,
  bookmark: &Bookmark,
  journal: Option<&get_journal_quotes::GetJournalQuotesReadingJournals>,
  book_id: i64,
  edition_id: i64,
  privacy_setting_id: i64,
  pages: i64,
) -> Either<insert_reading_journal::Variables, Option<update_reading_journal::Variables>> {
  let note = bookmark.annotation.as_deref().map(str::trim);
  let highlight = bookmark.text.trim();
  let entry = if let Some(note) = note
    && !note.is_empty()
  {
    format!("{highlight}\n━━━\n{note}")
  } else {
    highlight.to_string()
  };

  if let Some(journal) = journal {
    if journal.entry.as_ref() == Some(&entry) {
      Either::Right(None)
    } else {
      Either::Right(Some(update_reading_journal::Variables {
        journal_id: journal.id,
        entry,
      }))
    }
  } else {
    Either::Left(insert_reading_journal::Variables {
      book_id,
      edition_id,
      event: "quote".into(),
      privacy_setting_id,
      entry,
      action_at: Some(
        if CONFIG.sync_bookmarks == SyncBookmarks::Finished {
          Utc::now() + Duration::seconds(i as i64)
        } else {
          bookmark.date_created
        }
        .format("%+")
        .to_string(),
      ),
      metadata: bookmark.location.map(|location| {
        json!({
          "position": {
            "type": "pages",
            "value": (pages as f64 * location).round() as i64,
            "possible": pages,
            "percent": location * 100.0,
          }
        })
      }),
    })
  }
}
