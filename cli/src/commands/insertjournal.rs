use argh::FromArgs;
use graphql_client::GraphQLQuery;

use crate::config::log;
use crate::hardcover::{GetEdition, GetUserId, date, get_edition, get_user_id, send_request};
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

  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(GetUserId::build_query(
    get_user_id::Variables {},
  ))
  .await?
  .me
  .first()
  .ok_or("Failed to find Hardcover.app user")?
  .id;

  log(format!("user {user_id}"))?;

  let all_isbns = isbn.join(", ");

  let res = send_request::<get_edition::Variables, get_edition::ResponseData>(GetEdition::build_query(
    get_edition::Variables { isbn, book_id, user_id },
  ))
  .await?;

  let book = &res
    .editions
    .first()
    .expect(&if book_id != 0 {
      format!("Unable to find book id <i>{book_id}</i> on Hardcover.app. Please manually un-link and re-link book.")
    } else {
      format!(
        "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{all_isbns}</i>. Please manually link book."
      )
    })
    .book;

  let edition_id = book
    .user_books
    .first()
    .and_then(|user_book| user_book.edition.as_ref().map(|edition| edition.id))
    .or(book.editions.first().map(|edition| edition.id)) // ISBN edition
    .or(book.default_ebook_edition.as_ref().map(|edition| edition.id))
    .or(book.default_cover_edition.as_ref().map(|edition| edition.id))
    .ok_or(format!("Failed to select edition for book <i>{}</i>", book.id))?;

  let pages = book
    .user_books
    .first()
    .and_then(|user_book| user_book.edition.as_ref().and_then(|edition| edition.pages))
    .or(book.editions.first().and_then(|edition| edition.pages)) // ISBN edition
    .or(book.default_ebook_edition.as_ref().and_then(|edition| edition.pages))
    .or(book.default_cover_edition.as_ref().and_then(|edition| edition.pages))
    .or(book.pages)
    .expect(&format!(
        "Unable to find the total page count for book <i>{}</i>. Please update the book on Hardcover.app with the correct page count.",
        book.id)
      );

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
