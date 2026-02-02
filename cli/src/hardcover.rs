#![allow(non_camel_case_types)]

use chrono::Local;
use graphql_client::GraphQLQuery;
use graphql_client::{QueryBody, Response};
use reqwest::Client;
use serde::{Deserialize, Serialize};
use serde_json::{Map, Value};

use crate::config::CONFIG;

pub type date = String;
pub type jsonb = serde_json::Map<String, serde_json::Value>;

static USER_AGENT: &str = concat!(env!("CARGO_PKG_NAME"), "/", env!("CARGO_PKG_VERSION"),);

pub async fn send_request<T: Serialize, R: for<'a> Deserialize<'a> + std::fmt::Debug>(
  request_body: QueryBody<T>,
) -> R {
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
    let messages: Vec<String> = errors.iter().map(|err| err.message.clone()).collect();
    panic!(
      "Hardcover.app request failed with the following message:\n  {}",
      messages.join("\n  ")
    );
  }

  res
    .data
    .expect("Missing field `data` on Hardcover.app response")
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
pub struct InsertUserBook;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct InsertRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct UpdateRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct GetUserBookReadByIsnb;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct GetBook;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct GetUserBookReadById;

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
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct UpdateUserBookStatus;

pub async fn update_or_insert_user_book(
  id: Option<i64>,
  book_id: i64,
  edition_id: i64,
  progress_pages: i64,
) {
  let user_read_id = match id {
    Some(id) => {
      // User book exists set to "currently reading"
      println!("Update user book `{}` to currently reading", id);

      let res =
        send_request::<update_user_book_status::Variables, update_user_book_status::ResponseData>(
          UpdateUserBookStatus::build_query(update_user_book_status::Variables { id }),
        )
        .await;

      res
        .update_user_book
        .and_then(|update| update.user_book)
        .and_then(|user_book| user_book.user_book_reads.first().map(|read| read.id))
        .expect(&format!(
          "Failed to find user read after setting to currently reading on book {book_id}",
        ))
    }
    None => {
      // User book not found, insert one
      println!("Insert user book for book `{book_id}` and edition `{edition_id}`",);

      let res = send_request::<insert_user_book::Variables, insert_user_book::ResponseData>(
        InsertUserBook::build_query(insert_user_book::Variables {
          book_id,
          edition_id,
          status_id: 2,
        }),
      )
      .await;

      res
        .insert_user_book
        .and_then(|insert| insert.user_book)
        .and_then(|user_book| user_book.user_book_reads.first().map(|read| read.id))
        .expect(&format!(
          "Failed to insert user book with book {book_id}, edition {edition_id}, and status 2",
        ))
    }
  };

  let today = Local::now().format("%Y-%m-%d").to_string();

  println!(
    "Update read `{}` started at `{today}` for book `{book_id}` and eddition `{edition_id}` to page `{progress_pages}`",
    user_read_id,
  );

  send_request::<update_read::Variables, update_read::ResponseData>(UpdateRead::build_query(
    update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id,
      started_at: today,
    },
  ))
  .await;
}

pub async fn update_or_insert_read_by_isbn(percent: i64, isbn: Vec<String>) {
  let res = send_request::<get_user_id::Variables, get_user_id::ResponseData>(
    GetUserId::build_query(get_user_id::Variables {}),
  )
  .await;

  let user_id = res
    .me
    .first()
    .expect("Failed to find Hardcover.app user")
    .id;

  println!("Found user id {user_id}");

  // Attempt to get existing user book read
  let maybe_user_read = send_request::<
    get_user_book_read_by_isnb::Variables,
    get_user_book_read_by_isnb::ResponseData,
  >(GetUserBookReadByIsnb::build_query(
    get_user_book_read_by_isnb::Variables {
      isbn: isbn.clone(),
      user_id,
    },
  ))
  .await
  .user_book_reads
  .pop();

  let today = Local::now().format("%Y-%m-%d").to_string();

  if let Some(user_read) = maybe_user_read {
    // Existing user book read found, update progress
    let edition = user_read.edition.expect(&format!(
      "Failed to find book edition with ISBN/ASIN `{}`",
      isbn.join(", ")
    ));
    let pages = edition.pages.or(edition.book.pages).expect(&format!(
      "Failed to find page count for edition `{}` with ISBN `{}`",
      edition.id,
      isbn.join(", ")
    ));
    let progress_pages = (pages * percent) / 100;
    let started_at = user_read.started_at.unwrap_or(today);

    println!(
      "Update read `{}` started at `{started_at}` for eddition `{}` with isbn `{}` to page `{progress_pages}`",
      user_read.id,
      edition.id,
      isbn.join(", ")
    );

    send_request::<update_read::Variables, update_read::ResponseData>(UpdateRead::build_query(
      update_read::Variables {
        id: user_read.id,
        progress_pages,
        edition_id: edition.id,
        started_at,
      },
    ))
    .await;
  } else {
    // Retrieve book edition and maybe user book
    let edition = send_request::<get_edition::Variables, get_edition::ResponseData>(
      GetEdition::build_query(get_edition::Variables {
        isbn: isbn.clone(),
        user_id,
      }),
    )
    .await
    .editions
    .pop()
    .expect(&format!(
      "Failed to find book edition with ISBN/ASIN `{}`",
      isbn.join(", ")
    ));

    let pages = edition.pages.or(edition.book.pages).expect(&format!(
      "Failed to find page count for edition `{}` with ISBN `{}`",
      edition.id,
      isbn.join(", ")
    ));

    update_or_insert_user_book(
      edition
        .book
        .user_books
        .first()
        .map(|user_book| user_book.id),
      edition.book.id,
      edition.id,
      (pages * percent) / 100,
    )
    .await;
  }
}

pub async fn update_or_insert_read_by_id(percent: i64, book_id: i64) {
  let res = send_request::<get_user_id::Variables, get_user_id::ResponseData>(
    GetUserId::build_query(get_user_id::Variables {}),
  )
  .await;

  let user_id = res
    .me
    .first()
    .expect("Failed to find Hardcover.app user")
    .id;

  println!("Found user id {user_id}");

  // Attempt to get existing user book read
  let maybe_user_read =
    send_request::<get_user_book_read_by_id::Variables, get_user_book_read_by_id::ResponseData>(
      GetUserBookReadById::build_query(get_user_book_read_by_id::Variables { book_id, user_id }),
    )
    .await
    .user_book_reads
    .pop();

  let today = Local::now().format("%Y-%m-%d").to_string();

  if let Some(user_read) = maybe_user_read {
    // Existing user book read found, update progress
    let edition = user_read.edition.expect(&format!(
      "Failed to find edition of book with ID `{}`",
      book_id
    ));
    let pages = edition.pages.or(edition.book.pages).expect(&format!(
      "Failed to find page count for edition `{}` of book `{}`",
      edition.id, book_id
    ));
    let progress_pages = (pages * percent) / 100;
    let started_at = user_read.started_at.unwrap_or(today);

    println!(
      "Update read `{}` started at `{started_at}` for book `{book_id}` and eddition `{}` to page `{progress_pages}`",
      user_read.id, edition.id,
    );

    send_request::<update_read::Variables, update_read::ResponseData>(UpdateRead::build_query(
      update_read::Variables {
        id: user_read.id,
        progress_pages,
        edition_id: edition.id,
        started_at,
      },
    ))
    .await;
  } else {
    // Retrieve book, edition, and maybe user book
    let book = send_request::<get_book::Variables, get_book::ResponseData>(GetBook::build_query(
      get_book::Variables { book_id, user_id },
    ))
    .await
    .books
    .pop()
    .expect(&format!("Failed to find book `{book_id}`"));

    let edition_id = book
      .user_books
      .first()
      .and_then(|user_book| user_book.edition.as_ref().map(|edition| edition.id))
      .or(
        book
          .default_ebook_edition
          .as_ref()
          .map(|edition| edition.id),
      )
      .or(
        book
          .default_cover_edition
          .as_ref()
          .map(|edition| edition.id),
      )
      .expect(&format!("Failed to find edition for book `{book_id}`"));

    let pages = book
      .user_books
      .first()
      .and_then(|user_book| user_book.edition.as_ref().and_then(|edition| edition.pages))
      .or(
        book
          .default_ebook_edition
          .as_ref()
          .and_then(|ebook| ebook.pages),
      )
      .or(
        book
          .default_cover_edition
          .as_ref()
          .and_then(|ebook| ebook.pages),
      )
      .or(book.pages)
      .expect(&format!("Failed to find page count for book `{book_id}`"));

    update_or_insert_user_book(
      book.user_books.first().map(|user_book| user_book.id),
      book_id,
      edition_id,
      (pages * percent) / 100,
    )
    .await;
  }
}

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct SearchBooks;

pub async fn search_books(query: String, limit: i64, page: i64) -> Map<String, Value> {
  return send_request::<search_books::Variables, search_books::ResponseData>(
    SearchBooks::build_query(search_books::Variables { query, limit, page }),
  )
  .await
  .search
  .expect("Failed to find field `search` in Hardcover.app results")
  .results
  .expect("Failed to find field `results` in Hardcover.app results");
}
