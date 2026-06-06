use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use macros::{AggregateErrors, SendRequest};

use crate::config::{CONFIG, JournalPrivacy};
use crate::hardcover::{date, get_book, jsonb};
use crate::log;
use crate::utils::{VERSION, normalize_identifiers};

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertreadingjournal.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertReadingJournal;

/// Insert a new reading journal entry.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "insert-journal")]
pub struct InsertJournal {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,

  /// note text
  #[argh(option)]
  text: String,

  /// current read percentage
  #[argh(option)]
  percentage: f64,

  /// set journal privacy
  #[argh(option)]
  privacy: Option<JournalPrivacy>,
}

pub async fn run(args: InsertJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  let (book, edition_id, pages) = get_book(isbn, linked_id).await?;

  InsertReadingJournal::send_request(insert_reading_journal::Variables {
    book_id: book.id,
    edition_id,
    event: "note".into(),
    privacy_setting_id: args.privacy.unwrap_or(CONFIG.journal_privacy).get_value().await?,
    entry: args.text,
    action_at: None,
    metadata: Some(json!({
      "page": (pages as f64 * (args.percentage / 100.0)).round() as i64,
      "possible": pages,
      "percent": args.percentage,
    })),
  })
  .await?;

  Ok(())
}
