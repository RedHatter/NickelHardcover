use anyhow::Result;
use chrono::Local;

use crate::hardcover::{update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::log;
use crate::utils::{VERSION, normalize_identifiers};

use argh::FromArgs;

/// Update user book.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "set-user-book")]
pub struct SetUserBook {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// the status id
  #[argh(option)]
  status: Option<i64>,

  /// book rating
  #[argh(option)]
  rating: Option<f32>,

  /// review body text
  #[argh(option)]
  text: Option<String>,

  /// sponsored or ARC Review
  #[argh(option)]
  sponsored: Option<bool>,

  /// review contains spoilers
  #[argh(option)]
  spoilers: Option<bool>,
}

pub async fn run(args: SetUserBook) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (book_id, isbn) = normalize_identifiers(args.book_id, args.content_id.as_deref());

  update_or_insert_user_book(
    isbn,
    book_id,
    UserBookUpdateInput {
      status_id: args.status,
      review_has_spoilers: args.spoilers,
      sponsored_review: args.sponsored,
      rating: args.rating,
      reviewed_at: args.text.as_ref().map(|_| Local::now().format("%Y-%m-%d").to_string()),
      review_slate: args.text.map(|text| {
        serde_json::json!({
          "document": {
            "object": "document",
            "children": text
              .split('\n')
              .filter(|s| !s.is_empty())
              .map(|s| {
                serde_json::json!({
                  "data": {},
                  "type": "paragraph",
                  "object": "block",
                  "children": [
                    {
                      "text": s,
                      "object": "text"
                    }
                  ]
                })
              })
              .collect::<Vec<_>>()
          }
        })
      }),
      ..UserBookUpdateInput::default()
    },
  )
  .await?;

  Ok(())
}
