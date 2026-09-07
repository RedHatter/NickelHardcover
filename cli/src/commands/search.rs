use anyhow::{Context, Result};
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::Value;

use macros::AggregateErrors;

use crate::log;
use crate::messages::{Messages, SearchPages, SearchResult, Series};
use crate::utils::{GraphQLQueryExt, VERSION, send_msg};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/searchbooks.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
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

pub fn run(args: Search) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  let res = SearchBooks::send_request(search_books::Variables {
    query: args.query,
    limit: args.limit,
    page: args.page,
  })?
  .search
  .context("Failed to find field <i>search</i> in Hardcover.app results")?
  .results
  .context("Failed to find field <i>results</i> in Hardcover.app results")?;

  let results = res
    .get("hits")
    .and_then(Value::as_array)
    .context("Failed to find field <i>hits</i> in Hardcover.app results")?
    .iter()
    .filter_map(|hit| hit.get("document"))
    .map(|doc| SearchResult {
      authors: doc
        .get("contributions")
        .and_then(Value::as_array)
        .iter()
        .flat_map(|contributions| {
          contributions.iter().filter_map(|item| match item.get("contribution") {
            Some(Value::Null) | None => item
              .get("author")
              .and_then(|author| author.get("name"))
              .and_then(Value::as_str)
              .map(str::to_string),
            _ => None,
          })
        })
        .collect::<Vec<_>>(),
      id: doc.get("id").and_then(Value::as_str).map(str::to_string),
      image: doc
        .get("image")
        .and_then(|image| image.get("url"))
        .and_then(Value::as_str)
        .map(str::to_string),
      rating: doc.get("rating").and_then(Value::as_u64),
      release_year: doc.get("release_year").and_then(Value::as_i64),
      series: doc.get("featured_series").map(|featured_series| Series {
        name: featured_series
          .get("series")
          .and_then(|s| s.get("name"))
          .and_then(Value::as_str)
          .map(str::to_string),
        position: featured_series.get("position").and_then(Value::as_u64),
        primary_books_count: featured_series
          .get("series")
          .and_then(|series| series.get("primary_books_count"))
          .and_then(Value::as_u64),
      }),
      title: doc.get("title").and_then(Value::as_str).map(str::to_string),
      users_count: doc.get("users_count").and_then(Value::as_u64),
    })
    .collect::<Vec<_>>();

  send_msg(&Messages::SearchPages(SearchPages {
    results,
    page: res.get("page").and_then(Value::as_u64).unwrap_or(0),
    total: match res.get("found").and_then(Value::as_f64) {
      Some(0.0) => 0,
      Some(n) => ((n / args.limit as f64).ceil() as u64).max(1),
      None => 1,
    },
  }))
}
