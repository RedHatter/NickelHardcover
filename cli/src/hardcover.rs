#![allow(non_camel_case_types)]

use std::{fmt::Debug, sync::LazyLock};

use anyhow::{Context, Result, bail};
use itertools::Itertools;
use reqwest::{Certificate, Client, StatusCode};
use serde::{Serialize, de::DeserializeOwned};
use tokio::time::sleep;
use tokio_retry::{Retry, strategy::ExponentialBackoff};

use crate::config::CONFIG;
use crate::utils::{AggregateErrors, VERSION};
use crate::{debug_log, log};

pub mod scalars {
  use chrono::{DateTime, Utc};

  pub type date = String;
  pub type citext = String;
  pub type jsonb = serde_json::Value;
  pub type json = serde_json::Value;
  pub type numeric = f32;
  pub type bigint = i64;
  pub type float8 = f32;
  pub type smallint = i8;
  pub type timestamp = String;
  pub type timestamptz = DateTime<Utc>;
}

async fn try_request<T: Serialize>(request_body: &T) -> Result<reqwest::Response> {
  static CLIENT: LazyLock<Result<Client, reqwest::Error>> = LazyLock::new(|| {
    Client::builder()
      .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), &*VERSION))
      .tls_certs_only(
        webpki_root_certs::TLS_SERVER_ROOT_CERTS
          .iter()
          .map(|cert| Certificate::from_der(cert))
          .collect::<Result<Vec<_>, _>>()?,
      )
      .build()
  });

  let res = CLIENT
    .as_ref()
    .context("Failed to construct http client")?
    .post("https://api.hardcover.app/v1/graphql")
    .header("authorization", &CONFIG.authorization)
    .json(&request_body)
    .send()
    .await
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
      log!("Encountered http 429 sleeping for {}", duration.as_secs());
      sleep(duration).await;
    }
    StatusCode::UNAUTHORIZED => {
      panic!(
        "Authorization token is invalid. Please set a valid Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>."
      );
    }
    _ => {}
  }

  Ok(res.error_for_status()?)
}

pub async fn send_request<T: Serialize, R: DeserializeOwned + Debug + AggregateErrors>(
  operation_name: &str,
  request_body: T,
) -> Result<R> {
  assert!(
    !CONFIG.authorization.is_empty(),
    "Please set the Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>."
  );

  let res = Retry::spawn(ExponentialBackoff::from_millis(10).take(3), || {
    try_request(&request_body)
  })
  .await
  .context(format!("<i>{operation_name}</i> request failed"))?;

  let data: R = res
    .json()
    .await
    .context(format!("Failed to parse <i>{operation_name}</i> response"))?;

  debug_log!("{:?}", data);

  let errors = data.errors().join("<br>>");
  if !errors.is_empty() {
    bail!("{operation_name} has errors<br>{errors}");
  }

  Ok(data)
}
