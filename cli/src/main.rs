use std::env;
use std::panic;

use crate::commands::listbookmarks;
use crate::commands::listeditions;
use crate::commands::updatejournal;
use crate::commands::{getuser, getuserbook, insertjournal, listjournal, search, setuserbook, update};
use crate::config::CONFIG;
use crate::utils::{VERSION, write_logfile};

mod commands;
mod hardcover;

mod config;
mod database;
mod epub;
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
  Search(search::Search),
  SetUserBook(setuserbook::SetUserBook),
  Update(update::Update),
  UpdateJournal(updatejournal::UpdateJournal),
}

fn main() {
  if env::var("RUST_BACKTRACE").is_err() {
    panic::set_hook(Box::new(|info| {
      let msg = info.payload_as_str().unwrap_or("An unknown error occurred");
      eprintln!("{msg}");
      if let Err(e) = debug_log!("{msg}") {
        eprintln!("{e}");
      }
      write_logfile();
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

  if CONFIG.debug {
    write_logfile();
  }
}
