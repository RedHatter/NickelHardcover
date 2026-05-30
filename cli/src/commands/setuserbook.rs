use std::panic;

use anyhow::Result;
use chrono::Local;

use crate::hardcover::{update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::isbn::get_isbn;
use crate::log;
use crate::utils::VERSION;

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

  if args.content_id.is_none() && args.book_id.is_none() {
    panic!("One of --content-id or --book-id is required");
  }

  update_or_insert_user_book(
    args.content_id.iter().flat_map(|id| get_isbn(&id)).collect::<Vec<_>>(),
    args.book_id.unwrap_or(0),
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
