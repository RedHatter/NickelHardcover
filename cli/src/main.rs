use std::env;
use std::panic;

use crate::commands::{getuserbook, search, setuserbook, update};

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
  command: Commands,
}

#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum Commands {
  Search(search::Search),
  Update(update::Update),
  SetUserBook(setuserbook::SetUserBook),
  GetUserBook(getuserbook::GetUserBook),
}

#[tokio::main]
async fn main() {
  if env::var("RUST_BACKTRACE").is_err() {
    panic::set_hook(Box::new(|info| {
      if let Some(s) = info.payload().downcast_ref::<&str>() {
        eprintln!("{}", s);
      } else if let Some(s) = info.payload().downcast_ref::<String>() {
        eprintln!("{}", s);
      } else {
        eprintln!("An unknown error occurred");
      }
    }));
  }

  let args: Arguments = argh::from_env();
  match args.command {
    Commands::Search(args) => search::run(args).await,
    Commands::Update(args) => update::run(args).await,
    Commands::SetUserBook(args) => setuserbook::run(args).await,
    Commands::GetUserBook(args) => getuserbook::run(args).await,
  }
}
