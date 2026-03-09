use argh::FromArgs;
use graphql_client::GraphQLQuery;

use crate::config::log;
use crate::hardcover::{date, get_book, send_request};
use crate::isbn::get_isbn;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
)]
struct InsertReadingJournal;

/// Insert a new reading journal entry.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "insert-journal")]
pub struct InsertJournal {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// note text
  #[argh(option)]
  text: String,

  /// current read percentage
  #[argh(option)]
  percentage: f64,
}

pub async fn run(args: InsertJournal) -> Result<(), String> {
  log(format!("{:?}", args))?;

  if args.content_id.is_none() && args.book_id.is_none() {
    panic!("One of --content-id or --book-id is required");
  }

  let isbn = args.content_id.map(|id| get_isbn(&id)).unwrap_or(Vec::new());
  let book_id = args.book_id.unwrap_or(0);

  let (book, edition_id, pages, _) = get_book(isbn, book_id).await?;

  let res = send_request::<insert_reading_journal::Variables, insert_reading_journal::ResponseData>(
    InsertReadingJournal::build_query(insert_reading_journal::Variables {
      book_id: book.id,
      edition_id,
      event: "note".into(),
      entry: args.text,
      action_at: None,
      page: (pages as f64 * (args.percentage / 100.0)).round() as i64,
      possible: pages,
      percent: args.percentage,
    }),
  )
  .await?;

  if let Some(errors) = res.insert_reading_journal.and_then(|res| res.errors) {
    return Err(
      errors
        .iter()
        .filter_map(|error| error.as_deref())
        .collect::<Vec<&str>>()
        .join("<br>"),
    );
  }

  Ok(())
}
