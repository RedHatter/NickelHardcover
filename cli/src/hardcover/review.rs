use chrono::Local;
use graphql_client::GraphQLQuery;
use serde_json::Value;

use super::utils::{date, jsonb, numeric, send_request, timestamp};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/update.graphql",
  response_derives = "Serialize,Debug,Clone",
  variables_derives = "Deserialize"
)]
pub struct GetUserId;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/review.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct GetUserBookReview;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/review.graphql",
  response_derives = "Serialize,Debug",
  variables_derives = "Deserialize"
)]
pub struct UpdateReview;

#[derive(serde::Serialize)]
pub struct Review {
  pub user_book_id: i64,
  pub rating: Option<f32>,
  pub review_has_spoilers: bool,
  pub review_text: String,
  pub reviewed_at: Option<String>,
  pub sponsored_review: bool,
}

pub async fn get_review(isbn: Vec<String>, book_id: i64) -> Review {
  let user_id = send_request::<get_user_id::Variables, get_user_id::ResponseData>(GetUserId::build_query(
    get_user_id::Variables {},
  ))
  .await
  .me
  .first()
  .expect("Failed to find Hardcover.app user")
  .id;

  let all_isbns = isbn.join(", ");

  let res = send_request::<get_user_book_review::Variables, get_user_book_review::ResponseData>(
    GetUserBookReview::build_query(get_user_book_review::Variables { user_id, isbn, book_id }),
  )
  .await;
  let user_book = res
    .editions
    .first()
    .expect(&format!(
      "Failed to find edition with ASIN/ISBN `{all_isbns}` or book_id `{book_id}`",
    ))
    .book
    .user_books
    .first()
    .expect(&format!(
      "Failed to find user book with ASIN/ISBN `{all_isbns}` or book_id `{book_id}`",
    ));

  Review {
    user_book_id: user_book.id,
    rating: user_book.rating,
    review_has_spoilers: user_book.review_has_spoilers,
    review_text: user_book
      .review_slate
      .get("document")
      .map(reduce_slate)
      .unwrap_or(String::new())
      .trim()
      .to_string(),
    reviewed_at: user_book.reviewed_at.clone(),
    sponsored_review: user_book.sponsored_review,
  }
}

pub async fn update_review(
  rating: f32,
  text: String,
  review_has_spoilers: bool,
  sponsored_review: bool,
  user_book_id: i64,
) {
  let paragraphs: Vec<Value> = text
    .split('\n')
    .filter(|str| !str.is_empty())
    .map(|str| {
      serde_json::json!({
        "data": {},
        "type": "paragraph",
        "object": "block",
        "children": [
          {
            "text": str,
            "object": "text"
          }
        ]
      })
    })
    .collect();

  let res = send_request::<update_review::Variables, update_review::ResponseData>(UpdateReview::build_query(
    update_review::Variables {
      rating: if rating == 0.0 { None } else { Some(rating) },
      review_slate: serde_json::json!({
        "document": {
          "object": "document",
          "children": paragraphs
        }
      })
      .as_object()
      .expect("Failed to generate slate json")
      .clone(),
      sponsored_review,
      reviewed_at: Local::now().format("%Y-%m-%d").to_string(),
      review_has_spoilers,
      user_book_id,
    },
  ))
  .await;

  if let Some(error) = res.update_user_book.and_then(|res| res.error) {
    panic!("{error}");
  }
}

fn reduce_slate(data: &Value) -> String {
  return match data {
    Value::Array(array) => array.iter().map(reduce_slate).collect::<Vec<String>>().join(""),
    Value::Object(map) => {
      let mut str = match map.get("type").and_then(Value::as_str) {
        Some("paragraph") => "\n\n".into(),
        _ => String::new(),
      };

      let value = match map.get("object").and_then(Value::as_str) {
        Some("text") => map.get("text").and_then(Value::as_str).unwrap_or("").into(),
        _ => map
          .iter()
          .map(|(_, value)| reduce_slate(value))
          .collect::<Vec<String>>()
          .join(""),
      };

      str.push_str(&value);
      str
    }
    _ => String::new(),
  };
}
