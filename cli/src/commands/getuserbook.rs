use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::commands::getuser::get_user;
use crate::hardcover::{GetEdition, get_edition};
use crate::log;
use crate::utils::{VERSION, book_not_found, normalize_identifiers};

/// Retrieve user book including review.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user-book")]
pub struct GetUserBook {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,
}

pub async fn run(args: GetUserBook) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (book_id, isbn) = normalize_identifiers(args.book_id, args.content_id.as_deref());
  let isbn_display = isbn.join(", ");
  let user_id = get_user().await?.id;

  let user_books = match GetEdition::send_request(get_edition::Variables { isbn, book_id, user_id })
    .await?
    .editions
    .into_iter()
    .next()
  {
    Some(edition) => edition.book.user_books,
    None => book_not_found(&if book_id != 0 {
      format!("Unable to find book id <i>{book_id}</i> on Hardcover.app. Please manually un-link and re-link book.")
    } else {
      format!(
        "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{isbn_display}</i>. Please manually link book."
      )
    }),
  };

  let user_book = user_books.first().map_or(json!({}), |user_book| {
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
