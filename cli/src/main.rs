use std::env;
use std::panic;

use crate::{hardcover::update_or_insert_read, isbn::get_isbn};

mod config;
mod hardcover;
mod isbn;

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

  let mut args = env::args();
  args.next();

  let content_id = args.next().expect("`content_id` is a required argument");
  let percent = args
    .next()
    .expect("`percent` is a required argument")
    .parse::<i64>()
    .expect("`percent` must be a number");
  println!("Running cli with content id `{content_id}` and percent `{percent}`");
  let isbn = get_isbn(content_id.clone());
  println!(
    "Found ISBN `{}` for content id `{content_id}`",
    isbn.join(", ")
  );
  update_or_insert_read(isbn, percent).await;
}
