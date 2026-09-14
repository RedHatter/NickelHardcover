use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::Value;

use crate::appcontext::AppContext;
use crate::log;
use crate::messages::{Journal, JournalList, Messages, Metadata};
use crate::utils::{GraphQLQueryExt, VERSION, normalize_identifiers, send_msg};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getreadingjournal.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
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

pub fn run(context: &mut AppContext, args: &ListJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  let (linked_id, isbn) = normalize_identifiers(context, args.linked_id, args.content_id.as_deref());
  let user_id = context.user.id;

  let reading_journals = GetReadingJournal::send_request(
    context,
    get_reading_journal::Variables {
      isbn,
      linked_id,
      user_id,
      limit: args.limit,
      offset: args.offset,
    },
  )?
  .reading_journals
  .iter()
  .map(|journal| Journal {
    id: journal.id,
    event: journal.event.clone(),
    entry: journal.entry.clone(),
    action_at: journal.action_at,
    metadata: Metadata {
      list_name: journal
        .metadata
        .get("list_name")
        .and_then(Value::as_str)
        .map(str::to_string),
      progress: journal.metadata.get("progress").and_then(Value::as_u64),
      progress_was: journal.metadata.get("progress_was").and_then(Value::as_u64),
      prompt: journal
        .metadata
        .get("prompt")
        .and_then(Value::as_str)
        .map(str::to_string),
      rating: journal.metadata.get("rating").and_then(Value::as_u64),
      review: journal
        .metadata
        .get("review")
        .map(|review| reduce_slate(review).trim().to_string()),
    },
  })
  .collect::<Vec<_>>();

  log!("Found {}", reading_journals.len())?;
  send_msg(&Messages::JournalList(JournalList { reading_journals }))
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
