#![allow(non_camel_case_types)]

use graphql_client::{QueryBody, Response};
use reqwest::Client;
use serde::{Deserialize, Serialize};

use crate::config::CONFIG;

pub type date = String;
pub type jsonb = serde_json::Map<String, serde_json::Value>;
pub type numeric = f32;
pub type timestamp = String;

static USER_AGENT: &str = concat!(env!("CARGO_PKG_NAME"), "/", env!("CARGO_PKG_VERSION"),);

pub async fn send_request<T: Serialize, R: for<'a> Deserialize<'a> + std::fmt::Debug>(
  request_body: QueryBody<T>,
) -> R {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in `.adds/nickelpagesync/config.ini`"
  );

  let client = Client::builder()
    .user_agent(USER_AGENT)
    .build()
    .expect("Failed to construct http client");
  let res: Response<R> = client
    .post("https://api.hardcover.app/v1/graphql")
    .header("authorization", &CONFIG.authorization)
    .json(&request_body)
    .send()
    .await
    .expect("Failed to send Hardcover.app request")
    .json()
    .await
    .expect("Failed to parse Hardcover.app response");

  if let Some(errors) = res.errors {
    let messages: Vec<String> = errors.iter().map(|err| err.message.clone()).collect();
    panic!(
      "Hardcover.app request failed with the following message:\n  {}",
      messages.join("\n  ")
    );
  }

  res
    .data
    .expect("Missing field `data` on Hardcover.app response")
}
