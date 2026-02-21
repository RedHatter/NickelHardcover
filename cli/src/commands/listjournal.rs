use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use crate::commands::getuserbook::reduce_slate;
use crate::config::log;
use crate::hardcover::{GetUserId, bigint, get_user_id, jsonb, send_request, timestamptz};
use crate::isbn::get_isbn;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct GetReadingJournal;

/// Retrieve reading journal entries.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-journal")]
pub struct ListJournal {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// how many results to return
  #[argh(option)]
  limit: i64,

  /// how many results to skip
  #[argh(option)]
  offset: i64,
}

pub async fn run(args: ListJournal) -> Result<(), String> {
  log(format!("{:?}", args))?;

  if args.content_id.is_none() && args.book_id.is_none() {
    panic!("One of --content-id or --book-id is required");
  }

  let isbn = args.content_id.map(|id| get_isbn(&id)).unwrap_or(Vec::new());
  let book_id = args.book_id.unwrap_or(0);

  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(GetUserId::build_query(
    get_user_id::Variables {},
  ))
  .await?
  .me
  .first()
  .ok_or("Failed to find Hardcover.app user")?
  .id;

  log(format!("user {user_id}"))?;

  let journals = send_request::<get_reading_journal::Variables, get_reading_journal::ResponseData>(
    GetReadingJournal::build_query(get_reading_journal::Variables {
      isbn,
      book_id,
      user_id,
      limit: args.limit,
      offset: args.offset,
    }),
  )
  .await?
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

  log(format!("Found {}", journals.len()))?;

  log(format!("BEGIN_JSON\n{}", json!({ "reading_journals": journals})))?;

  Ok(())
}
