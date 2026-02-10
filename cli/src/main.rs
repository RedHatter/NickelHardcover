use std::env;
use std::panic;

use serde_json::{Value, json};

use crate::{
  hardcover::{
    journal::insert_journal,
    review::{get_review, update_review},
    search::search_books,
    update::update_or_insert_read,
  },
  isbn::get_isbn,
};

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
  Search(Search),
  Update(Update),
  Review(Review),
  GetReview(GetReview),
}

/// Search for books.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "search")]
struct Search {
  /// how many results per page
  #[argh(option)]
  limit: i64,

  /// which page
  #[argh(option)]
  page: i64,

  /// search query
  #[argh(option)]
  query: String,
}

/// Update read percentage and create journal entries for bookmarks.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update")]
struct Update {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: String,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// read percentage
  #[argh(option)]
  value: i64,

  /// process bookmarks created after this ISO 8601 date
  #[argh(option)]
  after: Option<String>,
}

/// Update user book review.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "review")]
struct Review {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,

  /// book rating
  #[argh(option)]
  rating: f32,

  /// review body text
  #[argh(option)]
  text: String,

  /// sponsored or ARC Review
  #[argh(switch)]
  sponsored: bool,

  /// review contains spoilers
  #[argh(switch)]
  spoilers: bool,
}

/// Retrieve user book review.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-review")]
struct GetReview {
  /// kobo book id or epub file path
  #[argh(option)]
  content_id: Option<String>,

  /// hardcover.app book id
  #[argh(option)]
  book_id: Option<i64>,
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
    Commands::Search(args) => {
      let results = search_books(args.query, args.limit, args.page).await;
      let hits = results
        .get("hits")
        .and_then(Value::as_array)
        .expect("Failed to find field `hits` in Hardcover.app results")
        .iter()
        .map(|hit| {
          hit.get("document").map(|doc| {
            json!({
              "id": doc.get("id"),
              "title": doc.get("title"),
              "release_year": doc.get("release_year"),
              "users_count": doc.get("users_count"),
              "rating": doc.get("rating"),
              "authors": doc.get("contributions").and_then(Value::as_array).map(|contributions|
                contributions.iter()
                  .filter_map(|item| match item.get("contribution") {
                    Some(Value::Null) | None => item.get("author").and_then(|author| author.get("name")),
                    _ => None
                  })
                  .collect::<Vec<_>>()
              ),
              "image": doc.get("image").and_then(|image| image.get("url")),
              "series": doc.get("featured_series").map(|featured_series| json!({
                "name": featured_series.get("series").map(|s| s.get("name")),
                "position": featured_series.get("position"),
                "primary_books_count": featured_series.get("series").and_then(|series| series.get("primary_books_count"))
              }))
            })
          })
        })
        .collect::<Vec<_>>();

      let json = json!({
        "results": hits,
        "page": results.get("page"),
        "total": results.get("found").and_then(Value::as_i64).unwrap_or(args.limit) / args.limit
      });

      println!("{}", json.to_string());
    }
    Commands::Update(args) => {
      let isbn = if args.book_id.is_none() {
        get_isbn(&args.content_id)
      } else {
        Vec::new()
      };

      let (book_id, edition_id, possible) = update_or_insert_read(isbn, args.book_id.unwrap_or(0), args.value).await;

      let bookmarks = bookmarks::get_bookmarks(args.content_id, args.after);

      for bookmark in bookmarks.iter() {
        let page = (possible as f64 * bookmark.location).round() as i64;
        let percent = bookmark.location * 100.0;

        insert_journal(insert_journal::Variables {
          book_id,
          edition_id,
          event: "quote".into(),
          entry: bookmark.text.clone(),
          action_at: bookmark.date_created.clone(),
          page,
          possible,
          percent,
        })
        .await;

        if !bookmark.annotation.is_empty() {
          insert_journal(insert_journal::Variables {
            book_id,
            edition_id,
            event: "note".into(),
            entry: bookmark.annotation.clone(),
            action_at: bookmark.date_created.clone(),
            page,
            possible,
            percent,
          })
          .await;
        }
      }
    }
    Commands::Review(args) => {
      if args.content_id.is_none() && args.book_id.is_none() {
        panic!("One of --content-id or --book-id is required");
      }

      let review = get_review(
        args.content_id.map(|id| get_isbn(&id)).unwrap_or(Vec::new()),
        args.book_id.unwrap_or(0),
      )
      .await;

      println!("Found user book `{}`", review.user_book_id);

      update_review(
        args.rating,
        args.text,
        args.spoilers,
        args.sponsored,
        review.user_book_id,
      )
      .await;
    }
    Commands::GetReview(args) => {
      if args.content_id.is_none() && args.book_id.is_none() {
        panic!("One of --content-id or --book-id is required");
      }

      let review = get_review(
        args.content_id.map(|id| get_isbn(&id)).unwrap_or(Vec::new()),
        args.book_id.unwrap_or(0),
      )
      .await;

      println!("{}", json!(review));
    }
  }
}
