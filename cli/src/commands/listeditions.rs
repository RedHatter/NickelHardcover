use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::Value;

use macros::AggregateErrors;

use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/geteditions.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors,Serialize",
  variables_derives = "Debug,Deserialize",
  variable_types("Int","Vec<Int>","Value"),
)]
struct GetEditions;

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

pub fn run(args: ListEditions) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  let reading_format = match args.reading_format {
    Some(format @ (1 | 2 | 4)) => vec![format],
    Some(format) => panic!("{format} is not a valid --reading-format value. Expected 1, 2, or 4."),
    None => vec![1, 4],
  };

  let res = GetEditions::send_request(get_editions::Variables {
    book_id: args.book_id,
    reading_format,
    where_: match args.language {
      Some(language) => serde_json::json!({
        "language": {
          "language": {
            "_eq": language
          }
        }
      }),
      None => Value::Object(serde_json::Map::new()),
    },
  })?;

  let mut languages = res
    .languages
    .into_iter()
    .filter_map(|o| o.language)
    .map(|l| l.language)
    .collect::<Vec<_>>();

  languages.sort();
  languages.dedup();

  log!(
    "BEGIN_JSON\n{}",
    serde_json::json!({
      "languages": languages,
      "editions": res
        .editions
        .into_iter()
        .map(|o|
          serde_json::json!({
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
        )
        .collect::<Vec<_>>()
    })
  )
}
