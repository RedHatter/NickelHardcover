use anyhow::Result;
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use jiff::Zoned;

use crate::appcontext::AppContext;
use crate::commands::getuserbook::get_book;
use crate::commands::setuserbook::{update_or_insert_user_book, update_user_book::UserBookUpdateInput};
use crate::commands::updatejournal::update_journal;
use crate::config::VERSION;
use crate::log;
use crate::utils::{GraphQLQueryExt, Percentage, normalize_identifiers};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/updateread.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
  variables_derives = "Debug"
)]
struct UpdateRead;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/mutations/insertread.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug",
  variables_derives = "Debug"
)]
struct InsertRead;

/// Update read percentage and create journal entries for annotations.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update")]
pub struct Update {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: String,

  /// hardcover.app book or edition id
  #[argh(option)]
  linked_id: Option<i64>,

  /// read percentage (0-100)
  #[argh(option)]
  value: Percentage,
}

pub fn run(context: &mut AppContext, args: &Update) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  let (linked_id, isbn) = normalize_identifiers(context, args.linked_id, Some(&args.content_id));
  let book = get_book(context, isbn, linked_id)?;
  let (user_book_id, user_read_id, started_at) = update_or_insert_user_book(
    context,
    &book,
    UserBookUpdateInput {
      status_id: Some(2),
      ..UserBookUpdateInput::default()
    },
  )?;
  let started_at = started_at.unwrap_or(Zoned::now().strftime("%F").to_string());

  let progress_pages = (book.pages as f64 * (args.value.0 / 100.0)).round() as i64;

  if let Some(user_read_id) = user_read_id {
    log!(
      "Update read `{user_read_id}` for edition `{}` to page `{progress_pages}`",
      book.edition_id
    )?;

    UpdateRead::send_request(
      context,
      update_read::Variables {
        id: user_read_id,
        progress_pages,
        edition_id: book.edition_id,
        started_at,
      },
    )?;
  } else {
    log!(
      "Insert new read for edition `{}` at page `{progress_pages}`",
      book.edition_id
    )?;

    InsertRead::send_request(
      context,
      insert_read::Variables {
        user_book_id,
        edition_id: book.edition_id,
        progress_pages,
        started_at,
      },
    )?;
  }

  if context.config.sync_annotations {
    update_journal(context, &args.content_id, &book)?;
  }

  Ok(())
}
