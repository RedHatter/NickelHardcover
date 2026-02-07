use chrono::Local;
use graphql_client::GraphQLQuery;

use super::utils::{date, send_request};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/update.graphql",
  response_derives = "Serialize,Debug,Clone",
  variables_derives = "Deserialize"
)]
pub struct GetUserId;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/update.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct InsertUserBook;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/update.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct UpdateRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/update.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct GetEdition;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/update.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct UpdateUserBookStatus;

pub async fn update_or_insert_read(isbn: Vec<String>, book_id: i64, percentage: i64) {
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

  let maybe_user_book = book.user_books.first();

  let user_read_id = if maybe_user_book.is_some_and(|user_book| user_book.status_id != 2) {
    // Existing user book, set it to "currently reading"
    let user_book_id = maybe_user_book
      .expect(&format!(
        "Failed to find user book for book with ISBN/ASIN `{all_isbns}` or id `{book_id}`"
      ))
      .id;

    println!("Update user book `{}` to currently reading", user_book_id);

    let res = send_request::<update_user_book_status::Variables, update_user_book_status::ResponseData>(
      UpdateUserBookStatus::build_query(update_user_book_status::Variables { id: user_book_id }),
    )
    .await;

    if let Some(error) = res.update_user_book.as_ref().and_then(|res| res.error.as_ref()) {
      panic!("{error}");
    }

    res
      .update_user_book
      .and_then(|update| update.user_book)
      .and_then(|user_book| user_book.user_book_reads.first().map(|read| read.id))
      .expect(&format!(
        "Failed to find user read after setting to currently reading on book {book_id}",
      ))
  } else if let Some(user_read_id) = maybe_user_book
    .and_then(|user_book| user_book.user_book_reads.first())
    .map(|user_read| user_read.id)
  {
    user_read_id // Existing user read
  } else {
    // User read or user book not found, insert one
    println!("Insert user book for book `{book_id}` and edition `{edition_id}`",);

    let res = send_request::<insert_user_book::Variables, insert_user_book::ResponseData>(InsertUserBook::build_query(
      insert_user_book::Variables {
        book_id,
        edition_id,
        status_id: 2,
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
      .expect(&format!(
        "Failed to insert user book with book {book_id}, edition {edition_id}, and status 2",
      ))
  };

  let progress_pages = (pages * percentage) / 100;
  let today = Local::now().format("%Y-%m-%d").to_string();

  println!(
    "Update read `{}` started at `{today}` for book `{book_id}` and eddition `{edition_id}` to page `{progress_pages}`",
    user_read_id,
  );

  let res = send_request::<update_read::Variables, update_read::ResponseData>(UpdateRead::build_query(
    update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id,
      started_at: today,
    },
  ))
  .await;

  if let Some(error) = res.update_user_book_read.and_then(|res| res.error) {
    panic!("{error}");
  }
}
