use anyhow::Result;
use argh::FromArgs;
use chrono::Local;
use graphql_client::GraphQLQuery;

use macros::{AggregateErrors, SendRequest};

use crate::commands::updatejournal::update_journal;
use crate::config::{CONFIG, SyncBookmarks};
use crate::hardcover::{date, update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::log;
use crate::utils::{VERSION, normalize_identifiers};

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updateread.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateRead;

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertread.graphql",
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct InsertRead;

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
}

pub async fn run(args: Update) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (book_id, isbn) = normalize_identifiers(args.book_id, Some(&args.content_id));

  let result = update_or_insert_user_book(
    isbn,
    book_id,
    UserBookUpdateInput {
      status_id: Some(2),
      ..UserBookUpdateInput::default()
    },
  )
  .await?;

  let progress_pages = (result.pages * args.value) / 100;

  if let Some(user_read_id) = result.user_read_id {
    log!(
      "Update read `{user_read_id}` for edition `{}` to page `{progress_pages}`",
      result.edition_id
    );

    UpdateRead::send_request(update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id: result.edition_id,
      started_at: result.started_at.unwrap_or(Local::now().format("%Y-%m-%d").to_string()),
    })
    .await?;
  } else {
    log!(
      "Insert new read for edition `{}` at page `{progress_pages}`",
      result.edition_id
    );

    InsertRead::send_request(insert_read::Variables {
      user_book_id: result.user_book_id,
      progress_pages,
      edition_id: result.edition_id,
      started_at: result.started_at.unwrap_or(Local::now().format("%Y-%m-%d").to_string()),
    })
    .await?;
  }

  if CONFIG.sync_bookmarks == SyncBookmarks::Never
    || (CONFIG.sync_bookmarks == SyncBookmarks::Finished && args.value != 100)
  {
    return Ok(());
  }

  update_journal(&args.content_id, result.book_id, result.edition_id, result.pages).await?;

  Ok(())
}
