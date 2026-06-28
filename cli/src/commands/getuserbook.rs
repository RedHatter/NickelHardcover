use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use macros::AggregateErrors;

use crate::commands::getuser::get_user;
use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION, book_not_found, normalize_identifiers};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getedition.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
pub struct GetEdition;

fn filter_edition(edition: &&get_edition::Edition) -> bool {
  edition.reading_format_id != 2
}

fn map_pages(edition: &get_edition::Edition) -> Option<i64> {
  if edition.reading_format_id == 2 {
    None
  } else {
    edition.pages
  }
}

/// Retrieve user book including review.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user-book")]
pub struct GetUserBook {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,
}

pub fn run(args: GetUserBook) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  let (book, _, _) = get_book(isbn, linked_id)?;

  let user_book = book.user_books.first().map_or(json!({}), |user_book| {
    json!( {
      "user_book_id": user_book.id,
      "status_id": user_book.status_id,
      "rating": user_book.rating,
      "review_has_spoilers": user_book.review_has_spoilers,
      "review_raw": user_book.review_raw,
      "reviewed_at": user_book.reviewed_at,
      "sponsored_review": user_book.sponsored_review,
    })
  });

  log!("BEGIN_JSON\n{user_book}");

  Ok(())
}

pub fn get_book(isbn: Vec<String>, linked_id: i64) -> Result<(get_edition::GetEditionEditionsBook, i64, i64)> {
  let user_id = get_user()?.id;
  let isbn_display = isbn.join(", ");

  // retrieve book, edition and maybe user book and user book read
  let book = match GetEdition::send_request(get_edition::Variables {
    isbn,
    linked_id,
    user_id,
  })?
  .editions
  .into_iter()
  .next()
  {
    Some(edition) => edition.book,
    None => book_not_found(&if linked_id != 0 {
      format!(
        "Unable to find book or edition with id <i>{linked_id}</i> on Hardcover.app. Please manually un-link and re-link book."
      )
    } else {
      format!(
        "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{isbn_display}</i>. Please manually link book."
      )
    }),
  };

  let edition_id = book
    .user_books
    .first()
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .filter(filter_edition)
    .or(book.id_edition.first().filter(filter_edition))
    .or(book.isbn_edition.first().filter(filter_edition))
    .or(
      book
        .user_books
        .first()
        .and_then(|user_book| user_book.edition.as_ref())
        .filter(filter_edition),
    )
    .or(book.default_ebook_edition.as_ref().filter(filter_edition))
    .or(book.default_cover_edition.as_ref().filter(filter_edition))
    .or(book.ebook_edition.first().filter(filter_edition))
    .or(book.paper_edition.first().filter(filter_edition))
    .expect(&format!(
      "Unable to find an edition for book <i>{}</i>. Does the book have any non-audiobook editions?",
      book.id
    ))
    .id;

  let pages = book
    .user_books
    .first()
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .and_then(map_pages)
    .or(book.id_edition.first().and_then(map_pages))
    .or(book.isbn_edition.first().and_then(map_pages))
    .or(
      book
        .user_books
        .first()
        .and_then(|user_book| user_book.edition.as_ref()).and_then(map_pages),
    )
    .or(book.default_ebook_edition.as_ref().and_then(map_pages))
    .or(book.default_cover_edition.as_ref().and_then(map_pages))
    .or(book.ebook_edition.first().and_then(map_pages))
    .or(book.paper_edition.first().and_then(map_pages))
    .or(book.pages)
    .expect(&format!(
        "Unable to find the total page count for book <i>{}</i>. Please update the book on Hardcover.app with the correct page count.",
        book.id)
      );

  Ok((book, edition_id, pages))
}
