use std::env;
use std::panic;

use crate::appcontext::AppContext;
use crate::commands::oauthset;
use crate::commands::{
  getuser, getuserbook, insertjournal, listbookmarks, listeditions, listjournal, oauthrequest, search, setuserbook,
  update, updatejournal,
};
use crate::config::VERSION;
use crate::messages::Error;
use crate::messages::Messages;
use crate::utils::send_msg;
use crate::utils::write_logfile;

mod appcontext;
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
    println!("{}", VERSION);
    return;
  }

  let mut context = AppContext::new().unwrap_or_else(|e| {
    panic!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    )
  });

  let res = match args
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
    Commands::OAuthCheck(args) => oauthset::run(&mut context, &args),
    Commands::Search(args) => search::run(&mut context, args),
    Commands::SetUserBook(args) => setuserbook::run(&mut context, args),
    Commands::Update(args) => update::run(&mut context, &args),
    Commands::UpdateJournal(args) => updatejournal::run(&mut context, &args),
  };

  if let Err(e) = res {
    panic!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }

  if context.config.debug
    && let Err(e) = write_logfile()
  {
    panic!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }
}
