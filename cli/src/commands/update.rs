use anyhow::Result;
use argh::FromArgs;
use chrono::Local;
use graphql_client::GraphQLQuery;

use macros::AggregateErrors;

use crate::commands::getuserbook::get_book;
use crate::commands::setuserbook::{update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::commands::updatejournal::update_journal;
use crate::config::{CONFIG, SyncBookmarks};
use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION, normalize_identifiers};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updateread.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,AggregateErrors",
  variables_derives = "Debug"
)]
struct UpdateRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertread.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
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

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,

  /// read percentage
  #[argh(option)]
  value: i64,
}

pub fn run(args: Update) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (linked_id, isbn) = normalize_identifiers(args.linked_id, Some(&args.content_id));
  let (book, edition_id, pages) = get_book(isbn, linked_id)?;
  let book_id = book.id;
  let (user_book_id, user_read_id, started_at) = update_or_insert_user_book(
    book,
    edition_id,
    UserBookUpdateInput {
      status_id: Some(2),
      ..UserBookUpdateInput::default()
    },
  )?;
  let started_at = started_at.unwrap_or(Local::now().format("%Y-%m-%d").to_string());

  let progress_pages = (pages * args.value) / 100;

  if let Some(user_read_id) = user_read_id {
    log!("Update read `{user_read_id}` for edition `{edition_id}` to page `{progress_pages}`");

    UpdateRead::send_request(update_read::Variables {
      id: user_read_id,
      progress_pages,
      edition_id,
      started_at,
    })?;
  } else {
    log!("Insert new read for edition `{edition_id}` at page `{progress_pages}`");

    InsertRead::send_request(insert_read::Variables {
      user_book_id,
      progress_pages,
      edition_id,
      started_at,
    })?;
  }

  if CONFIG.sync_bookmarks == SyncBookmarks::Never
    || (CONFIG.sync_bookmarks == SyncBookmarks::Finished && args.value != 100)
  {
    return Ok(());
  }

  update_journal(&args.content_id, book_id, edition_id, pages)?;

  Ok(())
}
