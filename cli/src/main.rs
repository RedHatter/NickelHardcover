use std::env;
use std::panic;

use itertools::Itertools;

use crate::appcontext::AppContext;
use crate::commands::{
  getuser, getuserbook, insertjournal, listbookmarks, listeditions, listjournal, oauthrequest, oauthset, search,
  setuserbook, update, updatejournal,
};
use crate::config::VERSION;
use crate::messages::{Error, Messages};
use crate::utils::ExpectedError;
use crate::utils::LOGGER;
use crate::utils::send_msg;

mod appcontext;
mod commands;
mod config;
mod database;
mod epub;
mod hardcover;
mod isbn;
mod messages;
mod rate_limit;
mod utils;

use argh::FromArgs;
use log::LevelFilter;

/// The CLI for NickelHardcover.
#[derive(FromArgs, PartialEq, Debug)]
struct Arguments {
  #[argh(subcommand)]
  command: Option<Commands>,

  /// print version number
  #[argh(switch)]
  version: bool,
}

#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum Commands {
  GetUser(getuser::GetUser),
  GetUserBook(getuserbook::GetUserBook),
  InsertJournal(insertjournal::InsertJournal),
  ListBookmarks(listbookmarks::ListBookmarks),
  ListEditions(listeditions::ListEditions),
  ListJournal(listjournal::ListJournal),
  OAuthRequest(oauthrequest::OAuthRequest),
  OAuthSet(oauthset::OAuthSet),
  Search(search::Search),
  SetUserBook(setuserbook::SetUserBook),
  Update(update::Update),
  UpdateJournal(updatejournal::UpdateJournal),
}

fn main() {
  log::set_logger(&LOGGER).expect("Failed to initialize logger");
  log::set_max_level(LevelFilter::Debug);

  if env::var("RUST_BACKTRACE").is_err() {
    panic::set_hook(Box::new(|info| {
      if let Err(e) = send_msg!(&Messages::Error(Error {
        error_code: "Unexpected".into(),
        message: info.payload_as_str().unwrap_or("An unknown error occurred").into(),
      })) {
        eprintln!("{e}");
      }

      if let Err(e) = LOGGER.write_to_disk() {
        eprintln!("{e}");
      }
    }));
  }

  let args: Arguments = argh::from_env();

  if args.version {
    println!("{VERSION}");
    return;
  }

  let res = AppContext::new().and_then(|mut context| {
    match args
      .command
      .expect("A subcommands must be present. Run with --help for more information.")
    {
      Commands::GetUser(args) => getuser::run(&mut context, &args),
      Commands::GetUserBook(args) => getuserbook::run(&mut context, &args),
      Commands::InsertJournal(args) => insertjournal::run(&mut context, args),
      Commands::ListBookmarks(args) => listbookmarks::run(&mut context, &args),
      Commands::ListEditions(args) => listeditions::run(&mut context, args),
      Commands::ListJournal(args) => listjournal::run(&mut context, &args),
      Commands::OAuthRequest(args) => oauthrequest::run(&mut context, &args),
      Commands::OAuthSet(args) => oauthset::run(&mut context, &args),
      Commands::Search(args) => search::run(&mut context, args),
      Commands::SetUserBook(args) => setuserbook::run(&mut context, args),
      Commands::Update(args) => update::run(&mut context, &args),
      Commands::UpdateJournal(args) => updatejournal::run(&mut context, &args),
    }?;

    if context.config.debug {
      LOGGER.write_to_disk()?;
    }

    Ok(())
  });

  if let Err(error) = res {
    if let Some(expected) = error.root_cause().downcast_ref::<ExpectedError>() {
      for cause in error.chain().dropping_back(1) {
        log::error!("{cause}");
      }

      send_msg!(&Messages::Error(Error {
        error_code: match expected {
          ExpectedError::BookInfo(_) => "BookInfo",
          ExpectedError::BookNotFound(_) => "BookNotFound",
          ExpectedError::DailyRateLimit => "DailyRateLimit",
          ExpectedError::OAuth(_) => "OAuth",
          ExpectedError::Unauthorized => "Unauthorized",
        }
        .to_string(),
        message: expected.to_string(),
      }))
      .expect("Failed to log error");

      if let Err(e) = LOGGER.write_to_disk() {
        eprintln!("{e}");
      }
    } else {
      panic!(
        "Encountered an unexpected error. Please report this.<br><br>{:#}",
        error.chain().join("<br>> ")
      );
    }
  }
}
