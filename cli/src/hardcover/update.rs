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
pub struct GetBook;

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

      if let Some(error) = res
        .update_user_book
        .as_ref()
        .and_then(|res| res.error.as_ref())
      {
        panic!("{error}");
      }

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

      if let Some(error) = res
        .insert_user_book
        .as_ref()
        .and_then(|res| res.error.as_ref())
      {
        panic!("{error}");
      }

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

  let res = send_request::<update_read::Variables, update_read::ResponseData>(
    UpdateRead::build_query(update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id,
      started_at: today,
    }),
  )
  .await;

  if let Some(error) = res.update_user_book_read.and_then(|res| res.error) {
    panic!("{error}");
  }
}

pub async fn update_or_insert_read_by_isbn(percent: i64, isbn: Vec<String>) {
  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(
    GetUserId::build_query(get_user_id::Variables {}),
  )
  .await
  .me
  .first()
  .expect("Failed to find Hardcover.app user")
  .id;

  println!("Found user id {user_id}");

  // etrieve book edition and maybe user book and user book read
  let res = send_request::<get_edition::Variables, get_edition::ResponseData>(
    GetEdition::build_query(get_edition::Variables {
      isbn: isbn.clone(),
      user_id,
    }),
  )
  .await;
  let edition = res.editions.first().expect(&format!(
    "Failed to find edition with ISBN/ASIN `{}`",
    isbn.join(", ")
  ));

  let pages = edition.pages.or(edition.book.pages).expect(&format!(
    "Failed to find page count for edition `{}` with ISBN `{}`",
    edition.id,
    isbn.join(", ")
  ));

  if let Some(user_read) = edition
    .book
    .user_books
    .first()
    .and_then(|user_book| user_book.user_book_reads.first())
  {
    let progress_pages = (pages * percent) / 100;
    let started_at = user_read
      .started_at
      .clone()
      .unwrap_or(Local::now().format("%Y-%m-%d").to_string());

    println!(
      "Update read `{}` started at `{started_at}` for eddition `{}` with isbn `{}` to page `{progress_pages}`",
      user_read.id,
      edition.id,
      isbn.join(", ")
    );

    let res = send_request::<update_read::Variables, update_read::ResponseData>(
      UpdateRead::build_query(update_read::Variables {
        id: user_read.id,
        progress_pages,
        edition_id: edition.id,
        started_at,
      }),
    )
    .await;

    if let Some(error) = res.update_user_book_read.and_then(|res| res.error) {
      panic!("{error}");
    }
  } else {
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

pub async fn update_or_insert_read_by_book(percent: i64, book_id: i64) {
  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(
    GetUserId::build_query(get_user_id::Variables {}),
  )
  .await
  .me
  .first()
  .expect("Failed to find Hardcover.app user")
  .id;

  println!("Found user id {user_id}");

  // Retrieve book, edition, and maybe user book and user read
  let res = send_request::<get_book::Variables, get_book::ResponseData>(GetBook::build_query(
    get_book::Variables { book_id, user_id },
  ))
  .await;
  let book = res
    .books
    .first()
    .expect(&format!("Failed to find book with id `{book_id}`"));

  let book_pages = book
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
    .or(book.pages);

  if let Some(user_read) = book
    .user_books
    .first()
    .and_then(|user_book| user_book.user_book_reads.first())
  {
    let edition = user_read.edition.as_ref().expect(&format!(
      "Failed to find edition of user read with ID `{}`",
      user_read.id
    ));
    let pages = edition.pages.or(book_pages).expect(&format!(
      "Failed to find page count for edition `{}` of book `{}`",
      edition.id, book_id
    ));
    let progress_pages = (pages * percent) / 100;
    let started_at = user_read
      .started_at
      .clone()
      .unwrap_or(Local::now().format("%Y-%m-%d").to_string());

    println!(
      "Update read `{}` started at `{started_at}` for book `{book_id}` and eddition `{}` to page `{progress_pages}`",
      user_read.id, edition.id,
    );

    let res = send_request::<update_read::Variables, update_read::ResponseData>(
      UpdateRead::build_query(update_read::Variables {
        id: user_read.id,
        progress_pages,
        edition_id: edition.id,
        started_at,
      }),
    )
    .await;

    if let Some(error) = res.update_user_book_read.and_then(|res| res.error) {
      panic!("{error}");
    }
  } else {
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
    let pages = book_pages.expect(&format!("Failed to find page count for book `{book_id}`"));

    update_or_insert_user_book(
      book.user_books.first().map(|user_book| user_book.id),
      book_id,
      edition_id,
      (pages * percent) / 100,
    )
    .await;
  }
}
