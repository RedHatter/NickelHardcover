use argh::FromArgs;
use chrono::Local;
use graphql_client::GraphQLQuery;

use crate::bookmarks::get_bookmarks;
use crate::hardcover::{date, send_request, update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::isbn::get_isbn;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
struct UpdateRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutation.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
struct InsertJournal;

/// Update read percentage and create journal entries for bookmarks.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update")]
pub struct Update {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: String,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// read percentage
  #[argh(option)]
  value: i64,

  /// process bookmarks created after this ISO 8601 date
  #[argh(option)]
  after: Option<String>,
}

pub async fn run(args: Update) {
  let isbn = if args.book_id.is_none() {
    get_isbn(&args.content_id)
  } else {
    Vec::new()
  };

  let (book_id, edition_id, possible, maybe_id) = update_or_insert_user_book(
    isbn,
    args.book_id.unwrap_or(0),
    UserBookUpdateInput {
      status_id: Some(2),
      ..UserBookUpdateInput::default()
    },
  )
  .await;

  let user_read_id = maybe_id.expect("Failed to find user read after updating or inserting user book");
  let progress_pages = (possible * args.value) / 100;

  println!("Update read `{user_read_id}` for edition `{edition_id}` to page `{progress_pages}`");

  let res = send_request::<update_read::Variables, update_read::ResponseData>(UpdateRead::build_query(
    update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id,
      started_at: Local::now().format("%Y-%m-%d").to_string(),
    },
  ))
  .await;

  if let Some(error) = res.update_user_book_read.and_then(|res| res.error) {
    panic!("{error}");
  }

  let bookmarks = get_bookmarks(args.content_id, args.after);

  for bookmark in bookmarks.iter() {
    let page = (possible as f64 * bookmark.location).round() as i64;
    let percent = bookmark.location * 100.0;

    insert_journal(insert_journal::Variables {
      book_id,
      edition_id,
      event: "quote".into(),
      entry: bookmark.text.clone(),
      action_at: bookmark.date_created.clone(),
      page,
      possible,
      percent,
    })
    .await;

    if !bookmark.annotation.is_empty() {
      insert_journal(insert_journal::Variables {
        book_id,
        edition_id,
        event: "note".into(),
        entry: bookmark.annotation.clone(),
        action_at: bookmark.date_created.clone(),
        page,
        possible,
        percent,
      })
      .await;
    }
  }
}

pub async fn insert_journal(entry: insert_journal::Variables) {
  println!(
    "insert `{}` for book `{}` and edition `{}` at `{}`",
    entry.event, entry.book_id, entry.edition_id, entry.action_at
  );

  let res =
    send_request::<insert_journal::Variables, insert_journal::ResponseData>(InsertJournal::build_query(entry)).await;

  if let Some(errors) = res.insert_reading_journal.and_then(|res| res.errors) {
    panic!("{:#?}", errors);
  }
}
