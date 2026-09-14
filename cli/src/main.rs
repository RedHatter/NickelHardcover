use std::env;
use std::panic;

use crate::commands::oauthset;
use crate::commands::{
  getuser, getuserbook, insertjournal, listbookmarks, listeditions, listjournal, oauthrequest, search, setuserbook,
  update, updatejournal,
};
use crate::config::CONFIG;
use crate::messages::Error;
use crate::messages::Messages;
use crate::utils::send_msg;
use crate::utils::{VERSION, write_logfile};

mod commands;
mod config;
mod database;
mod epub;
mod hardcover;
mod messages;
mod rate_limit;
mod utils;

use argh::FromArgs;
use itertools::Itertools;

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
  OAuthCheck(oauthset::OAuthSet),
  Search(search::Search),
  SetUserBook(setuserbook::SetUserBook),
  Update(update::Update),
  UpdateJournal(updatejournal::UpdateJournal),
}

fn main() {
  if env::var("RUST_BACKTRACE").is_err() {
    panic::set_hook(Box::new(|info| {
      if let Err(e) = send_msg(&Messages::Error(Error {
        error_code: "UNEXPECTED".into(),
        message: info.payload_as_str().unwrap_or("An unknown error occurred").into(),
      })) {
        eprintln!("{e}");
      }

      if let Err(e) = write_logfile() {
        eprintln!("{e}");
      }
    }));
  }

  let args: Arguments = argh::from_env();

  if args.version {
    println!("{}", &*VERSION);
    return;
  }

  let res = match args
    .command
    .expect("A subcommands must be present. Run with --help for more information.")
  {
    Commands::GetUser(args) => getuser::run(&args),
    Commands::GetUserBook(args) => getuserbook::run(&args),
    Commands::InsertJournal(args) => insertjournal::run(args),
    Commands::ListBookmarks(args) => listbookmarks::run(&args),
    Commands::ListEditions(args) => listeditions::run(args),
    Commands::ListJournal(args) => listjournal::run(&args),
    Commands::OAuthRequest(args) => oauthrequest::run(&args),
    Commands::OAuthCheck(args) => oauthset::run(&args),
    Commands::Search(args) => search::run(args),
    Commands::SetUserBook(args) => setuserbook::run(args),
    Commands::Update(args) => update::run(&args),
    Commands::UpdateJournal(args) => updatejournal::run(&args),
  };

  if let Err(e) = res {
    panic!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }

  if CONFIG.debug
    && let Err(e) = write_logfile()
  {
    panic!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }
}
