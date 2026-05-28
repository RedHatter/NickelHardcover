use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use macros::{AggregateErrors, SendRequest};

use crate::config::{CONFIG, JournalPrivacy};
use crate::hardcover::{date, get_book, jsonb};
use crate::isbn::get_isbn;
use crate::utils::{VERSION, log};

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
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

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

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

pub async fn run(args: InsertJournal) -> Result<(), String> {
  log(format!("{} {:?}", &*VERSION, args))?;

  if args.content_id.is_none() && args.book_id.is_none() {
    panic!("One of --content-id or --book-id is required");
  }

  let isbn = args.content_id.map(|id| get_isbn(&id)).unwrap_or(Vec::new());
  let book_id = args.book_id.unwrap_or(0);

  let (book, edition_id, pages, _) = get_book(isbn, book_id).await?;

  InsertReadingJournal::send_request(insert_reading_journal::Variables {
    book_id: book.id,
    edition_id,
    event: "note".into(),
    privacy_setting_id: args.privacy.unwrap_or(CONFIG.journal_privacy) as i64,
    entry: args.text,
    action_at: None,
    metadata: match json!({
      "page": (pages as f64 * (args.percentage / 100.0)).round() as i64,
      "possible": pages,
      "percent": args.percentage,
    }) {
      serde_json::Value::Object(obj) => Some(obj),
      _ => None,
    },
  })
  .await?;

  Ok(())
}
