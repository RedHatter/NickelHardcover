use anyhow::{Context, Result};
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::{Value, json};

use macros::AggregateErrors;

use crate::hardcover::date;
use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/geteditions.graphql",
  response_derives = "Debug,AggregateErrors,Serialize",
  variables_derives = "Debug"
)]
struct GetEditions;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/geteditions.graphql",
  response_derives = "Debug,AggregateErrors,Serialize",
  variables_derives = "Debug"
)]
struct GetEditionsWithLang;

/// Lists all editions for a given book.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-editions")]
pub struct ListEditions {
  /// hardcover.app book id
  #[argh(option)]
  book_id: i64,

  /// limit the format type
  #[argh(option)]
  reading_format: Option<i64>,

  /// limit the language
  #[argh(option)]
  language: Option<String>,
}

pub async fn run(args: ListEditions) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let reading_format = match args.reading_format {
    Some(format @ (1 | 2 | 4)) => vec![format],
    Some(format) => panic!("{format} is not a valid --reading-format value. Expected 1, 2, or 4."),
    None => vec![1, 4],
  };

  let (mut languages, editions) = if let Some(language) = args.language {
    let res = GetEditionsWithLang::send_request(get_editions_with_lang::Variables {
      book_id: args.book_id,
      reading_format,
      language,
    })
    .await?;

    (
      res
        .languages
        .into_iter()
        .filter_map(|o| o.language)
        .map(|l| l.language)
        .collect::<Vec<_>>(),
      res
        .editions
        .into_iter()
        .map(|o| Ok(build_edition(serde_json::from_value(serde_json::to_value(o)?)?)))
        .collect::<Result<Vec<_>>>()
        .context("Failed to convert between edition structs")?,
    )
  } else {
    let editions = GetEditions::send_request(get_editions::Variables {
      book_id: args.book_id,
      reading_format,
    })
    .await?
    .editions;

    (
      editions
        .iter()
        .filter_map(|o| o.language.as_ref())
        .map(|l| l.language.clone())
        .collect::<Vec<_>>(),
      editions.into_iter().map(build_edition).collect::<Vec<_>>(),
    )
  };

  languages.sort();
  languages.dedup();

  log!(
    "BEGIN_JSON\n{}",
    json!({ "editions": editions, "languages": languages })
  );

  Ok(())
}

fn build_edition(o: get_editions::GetEditionsEditions) -> Value {
  json!({
    "asin": o.asin,
    "contributions": o.contributions
      .into_iter()
      .filter_map(|c| c.author)
      .map(|a| a.name)
      .collect::<Vec<_>>(),
    "country": o.country.and_then(|c| c.name),
    "edition_format": o.edition_format,
    "edition_information": o.edition_information,
    "id": o.id,
    "image": o.image.and_then(|i| i.url),
    "isbn_10": o.isbn_10,
    "isbn_13": o.isbn_13,
    "language": o.language.map(|l| l.language),
    "pages": o.pages,
    "publisher": o.publisher.and_then(|p| p.name),
    "reading_format": match o.reading_format_id {
      1 => "Physical Book",
      2 => "Audiobook",
      4 => "E-Book",
      _ => "Unknown"
    },
    "release_date": o.release_date,
    "score": o.score,
    "title": o.title,
    "users_count": o.users_count
  })
}
