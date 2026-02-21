use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::{Value, json};

use crate::{
  config::log,
  hardcover::{jsonb, send_request},
};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct SearchBooks;

/// Search for books.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "search")]
pub struct Search {
  /// how many results per page
  #[argh(option)]
  limit: i64,

  /// which page
  #[argh(option)]
  page: i64,

  /// search query
  #[argh(option)]
  query: String,
}

pub async fn run(args: Search) -> Result<(), String> {
  log(format!("{:?}", args))?;

  let results = send_request::<search_books::Variables, search_books::ResponseData>(SearchBooks::build_query(
    search_books::Variables {
      query: args.query,
      limit: args.limit,
      page: args.page,
    },
  ))
  .await?
  .search
  .ok_or("Failed to find field <i>search</i> in Hardcover.app results")?
  .results
  .ok_or("Failed to find field <i>results</i> in Hardcover.app results")?;

  let hits = results
    .get("hits")
    .and_then(Value::as_array)
    .ok_or("Failed to find field <i>hits</i> in Hardcover.app results")?
    .iter()
    .map(|hit| {
      hit.get("document").map(|doc| {
        json!({
          "id": doc.get("id"),
          "title": doc.get("title"),
          "release_year": doc.get("release_year"),
          "users_count": doc.get("users_count"),
          "rating": doc.get("rating"),
          "authors": doc.get("contributions").and_then(Value::as_array).map(|contributions|
            contributions.iter()
              .filter_map(|item| match item.get("contribution") {
                Some(Value::Null) | None => item.get("author").and_then(|author| author.get("name")),
                _ => None
              })
              .collect::<Vec<_>>()
          ),
          "image": doc.get("image").and_then(|image| image.get("url")),
          "series": doc.get("featured_series").map(|featured_series| json!({
            "name": featured_series.get("series").map(|s| s.get("name")),
            "position": featured_series.get("position"),
            "primary_books_count": featured_series.get("series").and_then(|series| series.get("primary_books_count"))
          }))
        })
      })
    })
    .collect::<Vec<_>>();

  log(format!(
    "BEGIN_JSON\n{}",
    json!({
      "results": hits,
      "page": results.get("page"),
      "total": results.get("found").and_then(Value::as_i64).unwrap_or(args.limit) / args.limit
    })
    .to_string()
  ))?;

  Ok(())
}
