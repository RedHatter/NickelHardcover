#![allow(non_camel_case_types)]

use std::{fmt::Debug, sync::LazyLock};

use anyhow::{Context, Result, bail};
use chrono::{DateTime, Utc};
use graphql_client::GraphQLQuery;
use itertools::Itertools;
use reqwest::{Client, StatusCode};
use serde::{Serialize, de::DeserializeOwned};
use tokio::time::sleep;
use tokio_retry::{Retry, strategy::ExponentialBackoff};

use macros::{AggregateErrors, SendRequest};

use crate::commands::getuser::get_user;
use crate::config::CONFIG;
use crate::utils::{AggregateErrors, VERSION};
use crate::{debug_log, log};

pub type date = String;
pub type citext = String;
pub type jsonb = serde_json::Value;
pub type numeric = f32;
pub type bigint = i64;
pub type timestamp = String;
pub type timestamptz = DateTime<Utc>;

static CLIENT: LazyLock<Result<Client, reqwest::Error>> = LazyLock::new(|| {
  Client::builder()
    .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), &*VERSION))
    .build()
});

async fn try_request<T: Serialize>(request_body: &T) -> Result<reqwest::Response> {
  let res = CLIENT
    .as_ref()
    .context("Failed to construct http client")?
    .post("https://api.hardcover.app/v1/graphql")
    .header("authorization", &CONFIG.authorization)
    .json(&request_body)
    .send()
    .await
    .context("Failed to send request")?;

  if res.status() == StatusCode::TOO_MANY_REQUESTS {
    let timestamp = res
      .headers()
      .get("ratelimit-reset")
      .context("Failed to get <i>ratelimit-reset</i> header from http 429")?
      .to_str()
      .context("Failed to get <i>ratelimit-reset</i> header value")?
      .parse::<i64>()
      .context("Failed to parse <i>ratelimit-reset</i> header")?;
    let duration = chrono::DateTime::from_timestamp(timestamp, 0)
      .context(format!("Failed to create DateTime from timestamp <i>{timestamp}</i>"))?
      .signed_duration_since(chrono::Utc::now())
      .to_std()
      .context("Timestamp is in the past")?;
    log!("Encountered http 429 sleeping for {}", duration.as_secs());
    sleep(duration).await;
  }

  Ok(res.error_for_status()?)
}

pub async fn send_request<T: Serialize, R: DeserializeOwned + Debug + AggregateErrors>(
  operation_name: &str,
  request_body: T,
) -> Result<R> {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in `.adds/nickelpagesync/config.ini`"
  );

  let res = Retry::spawn(ExponentialBackoff::from_millis(10).take(3), async || {
    try_request(&request_body).await
  })
  .await
  .context(format!("<i>{operation_name}</i> request failed"))?;

  let data: R = res
    .json()
    .await
    .context(format!("Failed to parse <i>{operation_name}</i> response"))?;

  debug_log!("{:?}", data);

  let errors = data.errors().join("<br>>");
  if !errors.is_empty() {
    bail!("{operation_name} has errors<br>{errors}");
  }

  Ok(data)
}

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getedition.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
pub struct GetEdition;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updateuserbook.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug,Default",
  skip_serializing_none
)]
pub struct UpdateUserBook;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertuserbook.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug,Default",
  skip_serializing_none
)]
struct InsertUserBook;

pub struct UserBookResult {
  pub book_id: i64,
  pub edition_id: i64,
  pub pages: i64,
  pub user_book_id: i64,
  pub user_read_id: Option<i64>,
  pub started_at: Option<String>,
}

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

pub async fn get_book(isbn: Vec<String>, book_id: i64) -> Result<(get_edition::GetEditionEditionsBook, i64, i64)> {
  let user_id = get_user().await?.id;

  let all_isbns = isbn.join(", ");

  // retrieve book, edition and maybe user book and user book read
  let book = GetEdition::send_request(get_edition::Variables { isbn, book_id, user_id })
    .await?
    .editions
    .into_iter()
    .next()
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
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .filter(filter_edition)
    .or(book.isbn_edition.first().filter(filter_edition))
    .or(
      book
        .user_books
        .first()
        .and_then(|user_book| user_book.edition.as_ref())
        .filter(filter_edition),
    )
    .or(book.default_ebook_edition.as_ref().filter(filter_edition))
    .or(book.default_cover_edition.as_ref().filter(filter_edition))
    .or(book.ebook_edition.first().filter(filter_edition))
    .or(book.paper_edition.first().filter(filter_edition))
    .expect(&format!(
      "Unable to find an edition for book <i>{}</i>. Does the book have any non-audiobook editions?",
      book.id
    ))
    .id;

  let pages = book
    .user_books
    .first()
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .and_then(map_pages)
    .or(book.isbn_edition.first().and_then(map_pages))
    .or(
      book
        .user_books
        .first()
        .and_then(|user_book| user_book.edition.as_ref()).and_then(map_pages),
    )
    .or(book.default_ebook_edition.as_ref().and_then(map_pages))
    .or(book.default_cover_edition.as_ref().and_then(map_pages))
    .or(book.ebook_edition.first().and_then(map_pages))
    .or(book.paper_edition.first().and_then(map_pages))
    .or(book.pages)
    .expect(&format!(
        "Unable to find the total page count for book <i>{}</i>. Please update the book on Hardcover.app with the correct page count.",
        book.id)
      );

  Ok((book, edition_id, pages))
}

pub async fn update_or_insert_user_book(
  isbn: Vec<String>,
  book_id: i64,
  object: update_user_book::UserBookUpdateInput,
) -> Result<UserBookResult> {
  let (book, edition_id, pages) = get_book(isbn, book_id).await?;

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

  Ok(UserBookResult {
    book_id: book.id,
    edition_id,
    pages,
    user_book_id,
    user_read_id,
    started_at,
  })
}
