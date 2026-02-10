use graphql_client::GraphQLQuery;

use super::utils::{date, send_request};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/journal.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct InsertJournal;

pub async fn insert_journal(entry: insert_journal::Variables) {
  println!(
    "insert `{}` for book `{}` and edition `{}` at `{}`",
    entry.event, entry.book_id, entry.edition_id, entry.action_at
  );

  let res =
    send_request::<insert_journal::Variables, insert_journal::ResponseData>(InsertJournal::build_query(entry)).await;

  if let Some(errors) = res.insert_reading_journal.and_then(|res| res.errors) {
    panic!("{:#?}", errors);
  }
}
