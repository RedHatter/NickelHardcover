#![allow(non_camel_case_types)]

use graphql_client::{GraphQLQuery, QueryBody, Response};
use reqwest::Client;
use serde::{Deserialize, Serialize};

use crate::config::CONFIG;

pub type date = String;
pub type jsonb = serde_json::Map<String, serde_json::Value>;
pub type numeric = f32;
pub type timestamp = String;

static USER_AGENT: &str = concat!(env!("CARGO_PKG_NAME"), "/", env!("CARGO_PKG_VERSION"),);

pub async fn send_request<T: Serialize, R: for<'a> Deserialize<'a> + std::fmt::Debug>(request_body: QueryBody<T>) -> R {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in `.adds/nickelpagesync/config.ini`"
  );

  let client = Client::builder()
    .user_agent(USER_AGENT)
    .build()
    .expect("Failed to construct http client");
  let res: Response<R> = client
    .post("https://api.hardcover.app/v1/graphql")
    .header("authorization", &CONFIG.authorization)
    .json(&request_body)
    .send()
    .await
    .expect("Failed to send Hardcover.app request")
    .json()
    .await
    .expect("Failed to parse Hardcover.app response");

  if let Some(errors) = res.errors {
    panic!(
      "Hardcover.app request failed with the following message:\n  {}",
      errors
        .iter()
        .map(|err| err.message.clone())
        .collect::<Vec<String>>()
        .join("\n  ")
    );
  }

  res.data.expect("Missing field `data` on Hardcover.app response")
}

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug,Clone",
  variables_derives = "Deserialize"
)]
pub struct GetUserId;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct GetEdition;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Default,Debug",
  skip_serializing_none
)]
pub struct UpdateUserBook;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Default",
  skip_serializing_none
)]
pub struct InsertUserBook;

pub async fn update_or_insert_user_book(
  isbn: Vec<String>,
  book_id: i64,
  object: update_user_book::UserBookUpdateInput,
) -> (i64, i64, i64, Option<i64>) {
  println!("{:#?}", object);

  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(GetUserId::build_query(
    get_user_id::Variables {},
  ))
  .await
  .me
  .first()
  .expect("Failed to find Hardcover.app user")
  .id;

  println!("Found user id {user_id}");

  let all_isbns = isbn.join(", ");

  // retrieve book, edition and maybe user book and user book read
  let res = send_request::<get_edition::Variables, get_edition::ResponseData>(GetEdition::build_query(
    get_edition::Variables { isbn, book_id, user_id },
  ))
  .await;

  let book = &res
    .editions
    .first()
    .expect(&format!(
      "Failed to find edition with ISBN/ASIN `{all_isbns}` or book_id `{book_id}`"
    ))
    .book;

  let edition_id = book
    .user_books
    .first()
    .and_then(|user_book| user_book.edition.as_ref().map(|edition| edition.id))
    .or(book.editions.first().map(|edition| edition.id)) // ISBN edition
    .or(book.default_ebook_edition.as_ref().map(|edition| edition.id))
    .or(book.default_cover_edition.as_ref().map(|edition| edition.id))
    .expect(&format!(
      "Failed to select edition for book with ISBN/ASIN `{all_isbns}` or id `{book_id}`"
    ));

  let pages = book
    .user_books
    .first()
    .and_then(|user_book| user_book.edition.as_ref().and_then(|edition| edition.pages))
    .or(book.editions.first().and_then(|edition| edition.pages)) // ISBN edition
    .or(book.default_ebook_edition.as_ref().and_then(|edition| edition.pages))
    .or(book.default_cover_edition.as_ref().and_then(|edition| edition.pages))
    .or(book.pages)
    .expect(&format!(
      "Failed to find page count for book with ISBN/ASIN `{all_isbns}` or id `{book_id}`"
    ));

  let user_read_id = if let Some(user_book) = book.user_books.first() {
    if object.review_slate.is_some()
      || object
        .status_id
        .is_some_and(|status_id| status_id != user_book.status_id)
    {
      println!("Update user book `{}`", user_book.id);

      let res = send_request::<update_user_book::Variables, update_user_book::ResponseData>(
        UpdateUserBook::build_query(update_user_book::Variables {
          user_book_id: user_book.id,
          object,
        }),
      )
      .await;

      if let Some(error) = res.update_user_book.as_ref().and_then(|res| res.error.as_ref()) {
        panic!("{error}");
      }

      res
        .update_user_book
        .and_then(|update| update.user_book)
        .expect("Failed to find user book after setting status")
        .user_book_reads
        .first()
        .map(|read| read.id)
    } else {
      user_book.user_book_reads.first().map(|user_read| user_read.id)
    }
  } else {
    // Insert new user book
    println!("Insert user book for book `{book_id}` and edition `{edition_id}`",);

    let res = send_request::<insert_user_book::Variables, insert_user_book::ResponseData>(InsertUserBook::build_query(
      insert_user_book::Variables {
        object: insert_user_book::UserBookCreateInput {
          book_id,
          edition_id: Some(edition_id),
          status_id: object.status_id,
          rating: object.rating,
          review_slate: object.review_slate,
          sponsored_review: object.sponsored_review,
          reviewed_at: object.reviewed_at,
          review_has_spoilers: object.review_has_spoilers,
          ..insert_user_book::UserBookCreateInput::default()
        },
      },
    ))
    .await;

    if let Some(error) = res.insert_user_book.as_ref().and_then(|res| res.error.as_ref()) {
      panic!("{error}");
    }

    res
      .insert_user_book
      .and_then(|insert| insert.user_book)
      .and_then(|user_book| user_book.user_book_reads.first().map(|read| read.id))
  };

  (book.id, edition_id, pages, user_read_id)
}
