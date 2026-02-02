use std::env;
use std::panic;

use serde_json::{Value, json};

use crate::{
  hardcover::update_or_insert_read_by_id, hardcover::update_or_insert_read_by_isbn, isbn::get_isbn,
};

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

  let cmd = args.next();

  match cmd.as_deref() {
    Some("search") => {
      let limit = args
        .next()
        .expect("`limit` is a required argument")
        .parse::<i64>()
        .expect("`limit` must be a number");
      let page = args
        .next()
        .expect("`page` is a required argument")
        .parse::<i64>()
        .expect("`page` must be a number");
      let query = args.next().expect("`query` is a required argument");
      let results = hardcover::search_books(query, limit, page).await;
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
        "total": results.get("found").and_then(Value::as_i64).unwrap_or(limit) / limit
      });

      println!("{}", json.to_string());
    }
    Some("update") => {
      let content_id = args.next().expect("`content_id` is a required argument");
      let percent = args
        .next()
        .expect("`percent` is a required argument")
        .parse::<i64>()
        .expect("`percent` must be a number");
      let book_id = args.next();

      match book_id {
        Some(id) => {
          println!(
            "Running cli with content id `{content_id}`, book `{id}` and percent `{percent}`"
          );
          update_or_insert_read_by_id(
            percent,
            id.parse::<i64>().expect("`book_id` must be a number"),
          )
          .await;
        }
        None => {
          println!("Running cli with content id `{content_id}` and percent `{percent}`");
          let isbn = get_isbn(content_id.clone());
          println!(
            "Found ISBN `{}` for content id `{content_id}`",
            isbn.join(", ")
          );
          update_or_insert_read_by_isbn(percent, isbn).await;
        }
      }
    }
    _ => {
      println!("Usage:");
      println!("  cli search <limit> <query>");
      println!("  cli update <content_id> <percent> [book_id]");
    }
  }
}
