use std::thread::sleep;
use std::{fmt::Debug, sync::LazyLock};

use anyhow::{Context, Result, bail};
use itertools::Itertools;
use retry::{
  delay::{Exponential, jitter},
  retry,
};
use serde::{Serialize, de::DeserializeOwned};
use ureq::{
  Agent, Body,
  http::{Response, StatusCode},
};

use crate::config::CONFIG;
use crate::utils::{AggregateErrors, VERSION};
use crate::{debug_log, log};

pub mod scalars {
  #![allow(non_camel_case_types)]

  use chrono::{DateTime, Utc};

  pub type date = String;
  pub type citext = String;
  pub type jsonb = serde_json::Value;
  pub type json = serde_json::Value;
  pub type numeric = f32;
  pub type float8 = f32;
  pub type bigint = i64;
  pub type smallint = i16;
  pub type timestamp = String;
  pub type timestamptz = DateTime<Utc>;
}

fn try_request<T: Serialize>(request_body: &T) -> Result<Response<Body>> {
  static CLIENT: LazyLock<Agent> = LazyLock::new(|| {
    Agent::config_builder()
      .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), &*VERSION))
      .http_status_as_error(false)
      .build()
      .into()
  });

  let res = CLIENT
    .post("https://api.hardcover.app/v1/graphql")
    .header("authorization", &CONFIG.authorization)
    .send_json(request_body)
    .context("Failed to send request")?;

  match res.status() {
    StatusCode::TOO_MANY_REQUESTS => {
      let timestamp = res
        .headers()
        .get("ratelimit-reset")
        .context("Failed to get <i>ratelimit-reset</i> header from HTTP 429")?
        .to_str()
        .context("Failed to get <i>ratelimit-reset</i> header value")?
        .parse::<i64>()
        .context("Failed to parse <i>ratelimit-reset</i> header")?;
      let duration = chrono::DateTime::from_timestamp(timestamp, 0)
        .context(format!("Failed to create DateTime from timestamp <i>{timestamp}</i>"))?
        .signed_duration_since(chrono::Utc::now())
        .to_std()
        .context("Timestamp is in the past")?;
      log!("Encountered http 429 sleeping for {}", duration.as_secs())?;
      sleep(duration);
      bail!("Rate limited, retrying");
    }
    StatusCode::UNAUTHORIZED => {
      panic!(
        "Authorization token is invalid. Please set a valid Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>."
      );
    }
    code if !code.is_success() => {
      bail!("Request failed with status code <i>{code}</i>");
    }
    _ => {}
  }

  Ok(res)
}

pub fn send_request<T: Serialize, R: DeserializeOwned + Debug + AggregateErrors>(
  operation_name: &str,
  request_body: T,
) -> Result<R> {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>."
  );

  let data = retry(Exponential::from_millis(10).map(jitter).take(3), || {
    try_request(&request_body)
  })
  .map_err(|err| err.error)
  .context(format!("<i>{operation_name}</i> request failed"))?
  .body_mut()
  .read_json::<R>()
  .context(format!("Failed to parse <i>{operation_name}</i> response"))?;

  debug_log!("{:?}", data)?;

  let errors = data.errors().join("<br>>");
  if !errors.is_empty() {
    bail!("{operation_name} has errors<br>{errors}");
  }

  Ok(data)
}
