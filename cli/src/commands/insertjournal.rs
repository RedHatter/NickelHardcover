use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use crate::appcontext::AppContext;
use crate::commands::getuserbook::get_book;
use crate::config::{JournalPrivacy, VERSION};
use crate::log;
use crate::utils::{GraphQLQueryExt, Percentage, normalize_identifiers};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertreadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
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

  /// current read percentage (0-100)
  #[argh(option)]
  percentage: Percentage,

  /// set journal privacy
  #[argh(option)]
  privacy: Option<JournalPrivacy>,
}

pub fn run(context: &mut AppContext, args: InsertJournal) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  let (linked_id, isbn) = normalize_identifiers(context, args.linked_id, args.content_id.as_deref());
  let book = get_book(context, isbn, linked_id)?;
  let privacy_setting_id = args
    .privacy
    .unwrap_or(context.config.journal_privacy)
    .get_value(context)?;

  InsertReadingJournal::send_request(
    context,
    insert_reading_journal::Variables {
      book_id: book.book_id,
      edition_id: book.edition_id,
      event: "note".into(),
      privacy_setting_id,
      entry: args.text,
      action_at: None,
      metadata: Some(json!({
        "page": (book.pages as f64 * (args.percentage.0 / 100.0)).round() as i64,
        "possible": book.pages,
        "percent": args.percentage.0,
      })),
    },
  )?;

  Ok(())
}
