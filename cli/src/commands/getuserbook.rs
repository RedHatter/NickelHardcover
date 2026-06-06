use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::hardcover::get_book;
use crate::log;
use crate::utils::{VERSION, normalize_identifiers};

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

pub async fn run(args: GetUserBook) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  let (book, _, _) = get_book(isbn, linked_id).await?;

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
