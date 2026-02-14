use std::panic;

use chrono::Local;
use serde_json::Value;

use crate::hardcover::{update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::isbn::get_isbn;

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
  #[argh(switch)]
  sponsored: Option<bool>,

  /// review contains spoilers
  #[argh(switch)]
  spoilers: Option<bool>,
}

pub async fn run(args: SetUserBook) -> Result<(), String> {
  if args.content_id.is_none() && args.book_id.is_none() {
    panic!("One of --content-id or --book-id is required");
  }

  if args.status.is_none() && args.text.is_none() {
    panic!("At least one of --status or --text is required");
  }

  update_or_insert_user_book(
    args.content_id.map(|id| get_isbn(&id)).unwrap_or(Vec::new()),
    args.book_id.unwrap_or(0),
    UserBookUpdateInput {
      status_id: args.status,
      review_has_spoilers: args.spoilers,
      sponsored_review: args.sponsored,
      rating: args
        .rating
        .and_then(|rating| if rating == 0.0 { None } else { Some(rating) }),
      reviewed_at: if args.text.is_some() {
        Some(Local::now().format("%Y-%m-%d").to_string())
      } else {
        None
      },
      review_slate: args
        .text
        .map(|text| {
          serde_json::json!({
            "document": {
              "object": "document",
              "children": text
                .split('\n')
                .filter(|str| !str.is_empty())
                .map(|str| {
                  serde_json::json!({
                    "data": {},
                    "type": "paragraph",
                    "object": "block",
                    "children": [
                      {
                        "text": str,
                        "object": "text"
                      }
                    ]
                  })
                })
                .collect::<Vec<Value>>()
            }
          })
          .as_object()
          .map(|obj| obj.clone())
          .ok_or("Failed to generate slate json")
        })
        .transpose()?,
      ..UserBookUpdateInput::default()
    },
  )
  .await?;

  Ok(())
}
