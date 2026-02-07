use graphql_client::GraphQLQuery;
use serde_json::{Map, Value};

use super::utils::{jsonb, send_request};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/search.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct SearchBooks;

pub async fn search_books(query: String, limit: i64, page: i64) -> Map<String, Value> {
  return send_request::<search_books::Variables, search_books::ResponseData>(
    SearchBooks::build_query(search_books::Variables { query, limit, page }),
  )
  .await
  .search
  .expect("Failed to find field `search` in Hardcover.app results")
  .results
  .expect("Failed to find field `results` in Hardcover.app results");
}
