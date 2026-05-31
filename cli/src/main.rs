use std::env;
use std::panic;

use crate::commands::updatejournal;
use crate::commands::{getuser, getuserbook, insertjournal, listjournal, search, setuserbook, update};
use crate::config::CONFIG;
use crate::utils::{VERSION, write_logfile};

mod commands;
mod hardcover;

mod bookmarks;
mod config;
mod isbn;
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
  ListJournal(listjournal::ListJournal),
  Search(search::Search),
  SetUserBook(setuserbook::SetUserBook),
  Update(update::Update),
  UpdateJournal(updatejournal::UpdateJournal),
}

#[tokio::main]
async fn main() {
  if env::var("RUST_BACKTRACE").is_err() {
    panic::set_hook(Box::new(|info| {
      let msg = info.payload_as_str().unwrap_or("An unknown error occurred");
      eprintln!("{}", msg);
      debug_log!("{}", msg);
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
    Commands::GetUser(args) => getuser::run(args).await,
    Commands::GetUserBook(args) => getuserbook::run(args).await,
    Commands::InsertJournal(args) => insertjournal::run(args).await,
    Commands::ListJournal(args) => listjournal::run(args).await,
    Commands::Search(args) => search::run(args).await,
    Commands::SetUserBook(args) => setuserbook::run(args).await,
    Commands::Update(args) => update::run(args).await,
    Commands::UpdateJournal(args) => updatejournal::run(args).await,
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
