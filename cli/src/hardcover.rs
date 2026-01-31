#![allow(non_camel_case_types)]

use chrono::Local;
use graphql_client::GraphQLQuery;
use graphql_client::{QueryBody, Response};
use reqwest::Client;
use serde::{Deserialize, Serialize};

use crate::config::CONFIG;

pub type date = String;

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
pub struct GetUserBookRead;

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

pub async fn update_or_insert_read(isbn: Vec<String>, percent: i64) {
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
    send_request::<get_user_book_read::Variables, get_user_book_read::ResponseData>(
      GetUserBookRead::build_query(get_user_book_read::Variables {
        isbn: isbn.clone(),
        user_id,
      }),
    )
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
    let pages = edition.pages.expect(&format!(
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

    let user_book_id = match edition.book.user_books.first() {
      Some(user_book) => {
        // User book exists set to "currently reading" if needed
        if user_book.status_id != 2 {
          println!("Update user book `{}` to currently reading", user_book.id);

          send_request::<update_user_book_status::Variables, update_user_book_status::ResponseData>(
            UpdateUserBookStatus::build_query(update_user_book_status::Variables {
              id: user_book.id,
            }),
          )
          .await;
        }

        user_book.id
      }
      None => {
        // User book not found, insert one
        println!(
          "Insert user book for book `{}` and edition `{}`",
          edition.book.id, edition.id
        );

        let res = send_request::<insert_user_book::Variables, insert_user_book::ResponseData>(
          InsertUserBook::build_query(insert_user_book::Variables {
            book_id: edition.book.id,
            edition_id: edition.id,
            status_id: 2,
          }),
        )
        .await;

        res
          .insert_user_book
          .expect(&format!(
            "Failed to insert user book with book {}, edition {}, and status 2",
            edition.book.id, edition.id
          ))
          .user_book
          .expect(&format!(
            "Failed to insert user book with book {}, edition {}, and status 2",
            edition.book.id, edition.id
          ))
          .id
      }
    };

    // Insert new user book read
    let pages = edition.pages.expect(&format!(
      "Failed to find page count for edition `{}` with ISBN `{}`",
      edition.id,
      isbn.join(", ")
    ));
    let progress_pages = (pages * percent) / 100;

    println!(
      "Insert read for user book `{user_book_id}`, eddition `{}`, and isbn `{}` at page `{progress_pages}`",
      edition.id,
      isbn.join(", ")
    );

    send_request::<insert_read::Variables, insert_read::ResponseData>(InsertRead::build_query(
      insert_read::Variables {
        user_book_id,
        edition_id: edition.id,
        started_at: today,
        progress_pages,
      },
    ))
    .await;
  }
}
