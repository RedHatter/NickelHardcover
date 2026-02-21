#![allow(non_camel_case_types)]

use graphql_client::{GraphQLQuery, QueryBody, Response};
use reqwest::Client;
use serde::{Deserialize, Serialize};

use crate::config::{CONFIG, debug_log, log, report};

pub type date = String;
pub type jsonb = serde_json::Map<String, serde_json::Value>;
pub type numeric = f32;
pub type bigint = i64;
pub type timestamp = String;
pub type timestamptz = String;

static USER_AGENT: &str = concat!(env!("CARGO_PKG_NAME"), "/", env!("CARGO_PKG_VERSION"),);

pub async fn send_request<T: Serialize + std::fmt::Debug, R: for<'a> Deserialize<'a> + std::fmt::Debug>(
  request_body: QueryBody<T>,
) -> Result<R, String> {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in `.adds/nickelpagesync/config.ini`"
  );

  debug_log(&format!(
    "{}, {:?}",
    request_body.operation_name, request_body.variables
  ))?;

  let client = Client::builder()
    .user_agent(USER_AGENT)
    .build()
    .map_err(report(&format!(
      "Failed to construct http client for {}",
      request_body.operation_name
    )))?;
  let res: Response<R> = client
    .post("https://api.hardcover.app/v1/graphq")
    .header("authorization", &CONFIG.authorization)
    .json(&request_body)
    .send()
    .await
    .map_err(report(&format!(
      "Failed to send request {}",
      request_body.operation_name
    )))?
    .json()
    .await
    .map_err(report(&format!(
      "Failed to parse {} response",
      request_body.operation_name
    )))?;

  debug_log(&format!("{:?}", res))?;

  if let Some(errors) = res.errors {
    return Err(format!(
      "{} request failed:<br>{}",
      request_body.operation_name,
      errors
        .iter()
        .map(|err| err.message.as_ref())
        .collect::<Vec<&str>>()
        .join("<br>")
    ));
  }

  match res.data {
    Some(data) => Ok(data),
    None => Err(format!("Empty response for {}: {:?}", request_body.operation_name, res)),
  }
}

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug,Clone",
  variables_derives = "Deserialize,Debug"
)]
pub struct GetUserId;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize,Debug"
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
  variables_derives = "Deserialize,Default,Debug",
  skip_serializing_none
)]
pub struct InsertUserBook;

pub struct UserBookResult {
  pub book_id: i64,
  pub edition_id: i64,
  pub pages: i64,
  pub user_id: i64,
  pub user_read_id: Option<i64>,
  pub started_at: Option<String>,
}

pub async fn update_or_insert_user_book(
  isbn: Vec<String>,
  book_id: i64,
  object: update_user_book::UserBookUpdateInput,
) -> Result<UserBookResult, String> {
  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(GetUserId::build_query(
    get_user_id::Variables {},
  ))
  .await?
  .me
  .first()
  .ok_or("Failed to find Hardcover.app user")?
  .id;

  log(format!("user {user_id}"))?;

  let all_isbns = isbn.join(", ");

  // retrieve book, edition and maybe user book and user book read
  let res = send_request::<get_edition::Variables, get_edition::ResponseData>(GetEdition::build_query(
    get_edition::Variables { isbn, book_id, user_id },
  ))
  .await?;

  let book = &res
    .editions
    .first()
    .expect(&if book_id != 0 {
      format!("Unable to find book id <i>{book_id}</i> on Hardcover.app. Please manually un-link and re-link book.")
    } else {
      format!(
        "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{all_isbns}</i>. Please manually link book."
      )
    })
    .book;

  let edition_id = book
    .user_books
    .first()
    .and_then(|user_book| user_book.edition.as_ref().map(|edition| edition.id))
    .or(book.editions.first().map(|edition| edition.id)) // ISBN edition
    .or(book.default_ebook_edition.as_ref().map(|edition| edition.id))
    .or(book.default_cover_edition.as_ref().map(|edition| edition.id))
    .ok_or(format!("Failed to select edition for book <i>{}</i>", book.id))?;

  let pages = book
    .user_books
    .first()
    .and_then(|user_book| user_book.edition.as_ref().and_then(|edition| edition.pages))
    .or(book.editions.first().and_then(|edition| edition.pages)) // ISBN edition
    .or(book.default_ebook_edition.as_ref().and_then(|edition| edition.pages))
    .or(book.default_cover_edition.as_ref().and_then(|edition| edition.pages))
    .or(book.pages)
    .expect(&format!(
        "Unable to find the total page count for book <i>{}</i>. Please update the book on Hardcover.app with the correct page count.",
        book.id)
      );

  let (user_read_id, started_at) = if let Some(user_book) = book.user_books.first() {
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
      log(format!("Update user book `{}`", user_book.id))?;

      let res = send_request::<update_user_book::Variables, update_user_book::ResponseData>(
        UpdateUserBook::build_query(update_user_book::Variables {
          user_book_id: user_book.id,
          object,
        }),
      )
      .await?;

      if let Some(error) = res.update_user_book.as_ref().and_then(|res| res.error.clone()) {
        return Err(error);
      }

      let user_book = res
        .update_user_book
        .and_then(|update| update.user_book)
        .ok_or(format!("Failed to find updated user book <i>{}</i>", user_book.id))?;
      let read = user_book.user_book_reads.first();

      (read.map(|read| read.id), read.and_then(|read| read.started_at.clone()))
    } else {
      let read = user_book.user_book_reads.first();
      (read.map(|read| read.id), read.and_then(|read| read.started_at.clone()))
    }
  } else {
    // Insert new user book
    log(format!(
      "Insert user book for book `{book_id}` and edition `{edition_id}`",
    ))?;

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
    .await?;

    if let Some(error) = res.insert_user_book.as_ref().and_then(|res| res.error.clone()) {
      return Err(error);
    }

    let user_book = res
      .insert_user_book
      .and_then(|update| update.user_book)
      .ok_or("Failed to find inserted user book")?;
    let read = user_book.user_book_reads.first();

    (read.map(|read| read.id), read.and_then(|read| read.started_at.clone()))
  };

  if let Some(id) = user_read_id {
    log(format!("user read `{id}`"))?;
  }

  Ok(UserBookResult {
    book_id: book.id,
    edition_id,
    pages,
    user_id,
    user_read_id,
    started_at,
  })
}
