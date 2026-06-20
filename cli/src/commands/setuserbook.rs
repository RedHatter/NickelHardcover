use anyhow::{Context, Result};
use chrono::Local;
use graphql_client::GraphQLQuery;

use macros::AggregateErrors;

use crate::commands::getuserbook::{get_book, get_edition::GetEditionEditionsBook};
use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION, normalize_identifiers};

use argh::FromArgs;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertuserbook.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug,Default",
  skip_serializing_none
)]
struct InsertUserBook;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updateuserbook.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug,Default",
  skip_serializing_none
)]
pub struct UpdateUserBook;

/// Update user book.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "set-user-book")]
pub struct SetUserBook {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,

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

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  let (book, edition_id, _) = get_book(isbn, linked_id).await?;

  update_or_insert_user_book(
    book,
    edition_id,
    update_user_book::UserBookUpdateInput {
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
      ..update_user_book::UserBookUpdateInput::default()
    },
  )
  .await?;

  Ok(())
}

pub async fn update_or_insert_user_book(
  book: GetEditionEditionsBook,
  edition_id: i64,
  object: update_user_book::UserBookUpdateInput,
) -> Result<(i64, Option<i64>, Option<String>)> {
  let (user_book_id, user_read_id, started_at) = if let Some(user_book) = book.user_books.into_iter().next() {
    if object.review_slate.is_some()
      || (object.rating.is_some() && object.rating != user_book.rating)
      || object
        .review_has_spoilers
        .is_some_and(|review_has_spoilers| review_has_spoilers != user_book.review_has_spoilers)
      || object
        .sponsored_review
        .is_some_and(|sponsored_review| sponsored_review != user_book.sponsored_review)
      || object
        .status_id
        .is_some_and(|status_id| status_id != user_book.status_id)
    {
      log!("Update user book `{}`", user_book.id);

      UpdateUserBook::send_request(update_user_book::Variables {
        user_book_id: user_book.id,
        object,
      })
      .await?
      .update_user_book
      .and_then(|update| update.user_book)
      .context(format!("Failed to find updated user book <i>{}</i>", user_book.id))?
      .user_book_reads
      .into_iter()
      .next()
      .map_or((user_book.id, None, None), |read| {
        (user_book.id, Some(read.id), read.started_at)
      })
    } else {
      user_book
        .user_book_reads
        .into_iter()
        .next()
        .map_or((user_book.id, None, None), |read| {
          (user_book.id, Some(read.id), read.started_at)
        })
    }
  } else {
    // Insert new user book
    log!("Insert user book for book `{}` and edition `{edition_id}`", book.id);

    let user_book = InsertUserBook::send_request(insert_user_book::Variables {
      object: insert_user_book::UserBookCreateInput {
        book_id: book.id,
        edition_id: Some(edition_id),
        status_id: object.status_id,
        rating: object.rating,
        review_slate: object.review_slate,
        sponsored_review: object.sponsored_review,
        reviewed_at: object.reviewed_at,
        review_has_spoilers: object.review_has_spoilers,
        ..insert_user_book::UserBookCreateInput::default()
      },
    })
    .await?
    .insert_user_book
    .and_then(|update| update.user_book)
    .context("Failed to find inserted user book")?;
    user_book
      .user_book_reads
      .into_iter()
      .next()
      .map_or((user_book.id, None, None), |read| {
        (user_book.id, Some(read.id), read.started_at)
      })
  };

  if let Some(id) = user_read_id {
    log!("user read `{id}`");
  }

  Ok((user_book_id, user_read_id, started_at))
}
