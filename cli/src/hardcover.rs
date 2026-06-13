#![allow(non_camel_case_types)]

use std::{fmt::Debug, sync::LazyLock};

use anyhow::{Context, Result, bail};
use chrono::{DateTime, Utc};
use graphql_client::GraphQLQuery;
use itertools::Itertools;
use reqwest::{Certificate, Client, StatusCode};
use serde::{Serialize, de::DeserializeOwned};
use tokio::time::sleep;
use tokio_retry::{Retry, strategy::ExponentialBackoff};

use macros::{AggregateErrors, SendRequest};

use crate::commands::getuser::get_user;
use crate::config::CONFIG;
use crate::utils::{AggregateErrors, VERSION, book_not_found};
use crate::{debug_log, log};

pub type date = String;
pub type citext = String;
pub type jsonb = serde_json::Value;
pub type numeric = f32;
pub type bigint = i64;
pub type timestamp = String;
pub type timestamptz = DateTime<Utc>;

async fn try_request<T: Serialize>(request_body: &T) -> Result<reqwest::Response> {
  static CLIENT: LazyLock<Result<Client, reqwest::Error>> = LazyLock::new(|| {
    Client::builder()
      .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), &*VERSION))
      .tls_certs_only(
        webpki_root_certs::TLS_SERVER_ROOT_CERTS
          .iter()
          .map(|cert| Certificate::from_der(cert))
          .collect::<Result<Vec<_>, _>>()?,
      )
      .build()
  });

  let res = CLIENT
    .as_ref()
    .context("Failed to construct http client")?
    .post("https://api.hardcover.app/v1/graphql")
    .header("authorization", &CONFIG.authorization)
    .json(&request_body)
    .send()
    .await
    .context("Failed to send request")?;

  match res.status() {
    StatusCode::TOO_MANY_REQUESTS => {
      let timestamp = res
        .headers()
        .get("ratelimit-reset")
        .context("Failed to get <i>ratelimit-reset</i> header from HTTP 429")?
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
    StatusCode::UNAUTHORIZED => {
      panic!(
        "Authorization token is invalid. Please set a valid Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>."
      );
    }
    _ => {}
  }

  Ok(res.error_for_status()?)
}

pub async fn send_request<T: Serialize, R: DeserializeOwned + Debug + AggregateErrors>(
  operation_name: &str,
  request_body: T,
) -> Result<R> {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>."
  );

  let res = Retry::spawn(ExponentialBackoff::from_millis(10).take(3), || {
    try_request(&request_body)
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

pub async fn get_book(isbn: Vec<String>, linked_id: i64) -> Result<(get_edition::GetEditionEditionsBook, i64, i64)> {
  let user_id = get_user().await?.id;
  let isbn_display = isbn.join(", ");

  // retrieve book, edition and maybe user book and user book read
  let book = match GetEdition::send_request(get_edition::Variables {
    isbn,
    linked_id,
    user_id,
  })
  .await?
  .editions
  .into_iter()
  .next()
  {
    Some(edition) => edition.book,
    None => book_not_found(&if linked_id != 0 {
      format!(
        "Unable to find book or edition with id <i>{linked_id}</i> on Hardcover.app. Please manually un-link and re-link book."
      )
    } else {
      format!(
        "Unable to find a book edition on Hardcover.app with ISBN/ASIN <i>{isbn_display}</i>. Please manually link book."
      )
    }),
  };

  let edition_id = book
    .user_books
    .first()
    .and_then(|user_book| user_book.user_book_reads.first())
    .and_then(|read| read.edition.as_ref())
    .filter(filter_edition)
    .or(book.id_edition.first().filter(filter_edition))
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
    .or(book.id_edition.first().and_then(map_pages))
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
