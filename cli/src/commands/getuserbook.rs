use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;

use crate::appcontext::AppContext;
use crate::config::VERSION;
use crate::log;
use crate::messages::{Messages, UserBook};
use crate::utils::{GraphQLQueryExt, normalize_identifiers, send_error, send_msg};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getedition.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
  variables_derives = "Debug"
)]
pub struct GetEdition;

#[allow(clippy::trivially_copy_pass_by_ref)]
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

pub fn run(context: &mut AppContext, args: &GetUserBook) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  let (linked_id, isbn) = normalize_identifiers(context, args.linked_id, args.content_id.as_deref());
  let book = get_book(context, isbn, linked_id)?;

  if let Some(user_book) = book.user_book {
    send_msg(&Messages::UserBook(UserBook {
      user_book_id: user_book.id,
      status_id: user_book.status_id,
      rating: user_book.rating,
      review_has_spoilers: user_book.review_has_spoilers,
      review_raw: user_book.review_raw,
      reviewed_at: user_book.reviewed_at,
      sponsored_review: user_book.sponsored_review,
    }))?;
  }

  Ok(())
}

pub struct Book {
  pub user_book: Option<get_edition::GetEditionBooksUserBooks>,
  pub book_id: i64,
  pub edition_id: i64,
  pub pages: i64,
}

pub fn get_book(context: &mut AppContext, isbn: Vec<String>, linked_id: i64) -> Result<Book> {
  let user_id = context.user.id;
  let isbn_display = isbn.join(", ");

  // retrieve book, edition and maybe user book and user book read
  let book = match GetEdition::send_request(
    context,
    get_edition::Variables {
      isbn,
      linked_id,
      user_id,
    },
  )?
  .books
  .into_iter()
  .next()
  {
    Some(book) => book,
    None => send_error(
      context,
      "BOOK_NOT_FOUND",
      if linked_id != 0 {
        format!(
          "Unable to find book or edition with id <i>{linked_id}</i> on Hardcover.app. Please manually un-link and re-link book."
        )
      } else {
        format!(
          "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{isbn_display}</i>. Please manually link book."
        )
      },
    ),
  };
  let user_book = book.user_books.into_iter().next();

  let edition_id = user_book
    .as_ref()
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .filter(filter_edition)
    .or(book.id_edition.first().filter(filter_edition))
    .or(book.isbn_edition.first().filter(filter_edition))
    .or(
      user_book
        .as_ref()
        .and_then(|user_book| user_book.edition.as_ref())
        .filter(filter_edition),
    )
    .or(book.default_ebook_edition.as_ref().filter(filter_edition))
    .or(book.default_cover_edition.as_ref().filter(filter_edition))
    .or(book.ebook_edition.first().filter(filter_edition))
    .or(book.paper_edition.first().filter(filter_edition))
    .unwrap_or_else(|| {
      panic!(
        "Unable to find an edition for book <i>{}</i>. Does the book have any non-audiobook editions?",
        book.id
      )
    })
    .id;

  let pages = user_book.as_ref()
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .and_then(map_pages)
    .or(book.id_edition.first().and_then(map_pages))
    .or(book.isbn_edition.first().and_then(map_pages))
    .or(
      user_book.as_ref()
        .and_then(|user_book| user_book.edition.as_ref()).and_then(map_pages),
    )
    .or(book.default_ebook_edition.as_ref().and_then(map_pages))
    .or(book.default_cover_edition.as_ref().and_then(map_pages))
    .or(book.ebook_edition.first().and_then(map_pages))
    .or(book.paper_edition.first().and_then(map_pages))
    .or(book.pages)
    .unwrap_or_else(|| panic!("Unable to find the total page count for book <i>{}</i>. Please update the book on Hardcover.app with the correct page count.",
        book.id));

  Ok(Book {
    user_book,
    book_id: book.id,
    edition_id,
    pages,
  })
}
