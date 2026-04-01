use std::env;
use std::panic;
use std::sync::LazyLock;

use crate::commands::{getuser, getuserbook, insertjournal, listjournal, search, setuserbook, update};
use crate::config::CONFIG;
use crate::config::VERSION;
use crate::config::debug_log;

mod commands;
mod hardcover;

mod bookmarks;
mod config;
mod isbn;

use argh::FromArgs;

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
}

#[tokio::main]
async fn main() {
  if env::var("RUST_BACKTRACE").is_err() {
    panic::set_hook(Box::new(|info| {
      let msg = if let Some(s) = info.payload().downcast_ref::<&str>() {
        s
      } else if let Some(s) = info.payload().downcast_ref::<String>() {
        s
      } else {
        "An unknown error occurred"
      };

      eprintln!("{}", msg);
      if LazyLock::get(&CONFIG).is_some() {
        debug_log(msg).unwrap();
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
    Commands::GetUser(args) => getuser::run(args).await,
    Commands::GetUserBook(args) => getuserbook::run(args).await,
    Commands::InsertJournal(args) => insertjournal::run(args).await,
    Commands::ListJournal(args) => listjournal::run(args).await,
    Commands::Search(args) => search::run(args).await,
    Commands::SetUserBook(args) => setuserbook::run(args).await,
    Commands::Update(args) => update::run(args).await,
  };

  if let Err(e) = res {
    panic!("Encountered an unexpeced error. Please report this.<br><br>{e}");
  }
}
