use anyhow::{Context, Result};
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use itertools::{Either, Itertools};
use jiff::{SignedDuration, Timestamp};
use serde_json::json;

use crate::commands::getuser::get_user;
use crate::commands::getuserbook::{Book, get_book};
use crate::config::{CONFIG, SyncBookmarks};
use crate::database::{Bookmark, get_bookmarks};
use crate::hardcover::batch_requests;
use crate::utils::{GraphQLQueryExt, VERSION, normalize_identifiers};
use crate::{debug_log, log};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertreadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
  variables_derives = "Debug"
)]
struct InsertReadingJournal;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updatereadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
  variables_derives = "Debug"
)]
struct UpdateReadingJournal;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getjournalquotes.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
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
  let book = get_book(isbn, linked_id)?;
  update_journal(&args.content_id, &book)?;

  Ok(())
}

pub fn update_journal(content_id: &str, book: &Book) -> Result<()> {
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
        book_id: book.book_id,
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

  let mutations = bookmarks
    .iter()
    .enumerate()
    .map(|(i, bookmark)| {
      build_journal_quote(
        i,
        bookmark,
        reading_journals
          .iter()
          .find(|journal| journal.action_at.duration_since(bookmark.date_created).abs().as_secs() == 0),
        book,
      )
    })
    .flatten_ok()
    .collect::<Result<Vec<_>>>()?;

  if !mutations.is_empty() {
    log!(
      "Insert or update {} annotations for book `{}` and edition `{}`",
      mutations.len(),
      book.book_id,
      book.edition_id,
    )?;
    debug_log!("UpdateReadingJournal / InsertReadingJournal, {:?}", mutations)?;
    batch_requests::<_, serde_json::Value>(
      "UpdateReadingJournal / InsertReadingJournal",
      mutations
        .into_iter()
        .map(|m| {
          m.either(
            |left| serde_json::to_value(UpdateReadingJournal::build_query(left)),
            |right| serde_json::to_value(InsertReadingJournal::build_query(right)),
          )
        })
        .collect::<Result<Vec<_>, _>>()
        .context("Failed to serialize <i>UpdateReadingJournal / InsertReadingJournal</i> bodies")?,
    )?;
  }

  Ok(())
}

fn build_journal_quote(
  i: usize,
  bookmark: &Bookmark,
  journal: Option<&get_journal_quotes::GetJournalQuotesReadingJournals>,
  book: &Book,
) -> Result<Option<Either<update_reading_journal::Variables, insert_reading_journal::Variables>>> {
  let note = bookmark.annotation.as_deref().map(str::trim);
  let highlight = bookmark.text.trim();
  let entry = if let Some(note) = note
    && !note.is_empty()
  {
    format!("{highlight}\n━━━\n{note}")
  } else {
    highlight.to_string()
  };

  Ok(if let Some(journal) = journal {
    if journal.entry.as_ref() == Some(&entry) {
      None
    } else {
      Some(Either::Left(update_reading_journal::Variables {
        journal_id: journal.id,
        entry,
      }))
    }
  } else {
    Some(Either::Right(insert_reading_journal::Variables {
      book_id: book.book_id,
      edition_id: book.edition_id,
      event: "quote".into(),
      privacy_setting_id: CONFIG.journal_privacy.get_value()?,
      entry,
      action_at: Some(
        if CONFIG.sync_bookmarks == SyncBookmarks::Finished {
          Timestamp::now() + SignedDuration::from_secs(i as i64)
        } else {
          bookmark.date_created
        }
        .to_string(),
      ),
      metadata: bookmark.location.map(|location| {
        json!({
          "position": {
            "type": "pages",
            "value": (book.pages as f64 * location).round() as i64,
            "possible": book.pages,
            "percent": location * 100.0,
          }
        })
      }),
    }))
  })
}
