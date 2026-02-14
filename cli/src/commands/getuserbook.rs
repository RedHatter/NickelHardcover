use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::{Value, json};

use crate::hardcover::{GetEdition, GetUserId, get_edition, get_user_id, send_request};
use crate::isbn::get_isbn;

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

pub async fn run(args: GetUserBook) -> Result<(), String> {
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

  let all_isbns = isbn.join(", ");

  let res = send_request::<get_edition::Variables, get_edition::ResponseData>(GetEdition::build_query(
    get_edition::Variables { isbn, book_id, user_id },
  ))
  .await?;

  let user_book = res
    .editions
    .first()
    .expect(&if book_id != 0 {
      format!("Unable to find book id <i>{book_id}</i> on Hardcover.app. Please manually un-link and re-link book.")
    } else {
      format!(
        "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{all_isbns}</i>. Please manually link book."
      )
    })
    .book
    .user_books
    .first()
    .map(|user_book| {
      json!( {
        "user_book_id": user_book.id,
        "status_id": user_book.status_id,
        "rating": user_book.rating,
        "review_has_spoilers": user_book.review_has_spoilers,
        "review_text": user_book
          .review_slate
          .get("document")
          .map(reduce_slate)
          .unwrap_or(String::new())
          .trim()
          .to_string(),
        "reviewed_at": user_book.reviewed_at.clone(),
        "sponsored_review": user_book.sponsored_review,
      })
    })
    .unwrap_or(json!({}));

  println!("{user_book}");

  Ok(())
}

fn reduce_slate(data: &Value) -> String {
  return match data {
    Value::Array(array) => array.iter().map(reduce_slate).collect::<Vec<String>>().join(""),
    Value::Object(map) => {
      let mut str = match map.get("type").and_then(Value::as_str) {
        Some("paragraph") => "\n\n".into(),
        _ => String::new(),
      };

      let value = match map.get("object").and_then(Value::as_str) {
        Some("text") => map.get("text").and_then(Value::as_str).unwrap_or("").into(),
        _ => map
          .iter()
          .map(|(_, value)| reduce_slate(value))
          .collect::<Vec<String>>()
          .join(""),
      };

      str.push_str(&value);
      str
    }
    _ => String::new(),
  };
}
