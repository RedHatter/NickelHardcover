use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::{Value, json};

use macros::AggregateErrors;

use crate::commands::getuser::get_user;
use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION, normalize_identifiers};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getreadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct GetReadingJournal;

/// Retrieve reading journal entries.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-journal")]
pub struct ListJournal {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,

  /// how many results to return
  #[argh(option)]
  limit: i64,

  /// how many results to skip
  #[argh(option)]
  offset: i64,
}

pub fn run(args: &ListJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  let user_id = get_user()?.id;

  let journals = GetReadingJournal::send_request(get_reading_journal::Variables {
    isbn,
    linked_id,
    user_id,
    limit: args.limit,
    offset: args.offset,
  })?
  .reading_journals
  .iter()
  .map(|journal| {
    json!({
      "id": journal.id,
      "event": journal.event,
      "entry": journal.entry,
      "action_at": journal.action_at,
      "metadata": match journal.metadata.get("review") {
        Some(review) => json!({ "review": reduce_slate(review).trim() }),
        None => json!(journal.metadata)
      },
    })
  })
  .collect::<Vec<_>>();

  log!("Found {}", journals.len())?;
  log!("BEGIN_JSON\n{}", json!({ "reading_journals": journals}))?;

  Ok(())
}

pub fn reduce_slate(data: &Value) -> String {
  match data {
    Value::Array(array) => array.iter().map(reduce_slate).collect::<String>(),
    Value::Object(map) => {
      let mut str = match map.get("type").and_then(Value::as_str) {
        Some("paragraph") => "\n\n".into(),
        _ => String::new(),
      };

      let value = match map.get("object").and_then(Value::as_str) {
        Some("text") => map.get("text").and_then(Value::as_str).unwrap_or("").into(),
        _ => map.iter().map(|(_, value)| reduce_slate(value)).collect::<String>(),
      };

      str += &value;
      str
    }
    _ => String::new(),
  }
}
