use std::sync::atomic::{AtomicUsize, Ordering};
use std::thread::sleep;
use std::time::Duration;
use std::{fmt::Debug, sync::LazyLock};

use anyhow::{Context, Result, bail};
use itertools::Itertools;
use retry::{
  delay::{Exponential, jitter},
  retry,
};
use serde::{Serialize, de::DeserializeOwned};
use serde_json::Value;
use ureq::{
  Agent, Body,
  http::{Response, StatusCode},
};

use crate::config::CONFIG;
use crate::rate_limit::RateLimit;
use crate::utils::{AggregateErrors, VERSION};
use crate::{debug_log, log};

pub mod scalars {
  #![allow(non_camel_case_types)]

  use jiff::Timestamp;

  pub type date = String;
  pub type citext = String;
  pub type jsonb = serde_json::Value;
  pub type json = serde_json::Value;
  pub type numeric = f64;
  pub type float8 = f64;
  pub type bigint = i64;
  pub type smallint = i16;
  pub type timestamp = String;
  pub type timestamptz = Timestamp;
}

struct HardcoverContext {
  agent: Agent,
  rate_limit: AtomicUsize,
}

impl HardcoverContext {
  fn rate_limit(&self) -> usize {
    self.rate_limit.load(Ordering::Relaxed)
  }

  fn set_rate_limit(&self, val: usize) {
    self.rate_limit.store(val.max(1), Ordering::Relaxed);
  }
}

static HARDCOVER_AGENT: LazyLock<HardcoverContext> = LazyLock::new(|| HardcoverContext {
  agent: Agent::config_builder()
    .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), &*VERSION))
    .http_status_as_error(false)
    .build()
    .into(),
  rate_limit: AtomicUsize::new(0),
});

fn try_request<T: Serialize>(request_body: &T) -> Result<Response<Body>> {
  let res = HARDCOVER_AGENT
    .agent
    .post(&CONFIG.hardcover_endpoint)
    .header("authorization", &CONFIG.authorization)
    .send_json(request_body)
    .context("Failed to send request")?;

  let code = res.status();

  if code == StatusCode::TOO_MANY_REQUESTS
    && let Some(retry_after) = res.headers().get("Retry-After")
  {
    let retry_after = retry_after
      .to_str()
      .context("Failed to get <i>Retry-After</i> header value")?
      .parse::<u64>()
      .context("Failed to parse <i>Retry-After</i> header")?;
    let duration = Duration::from_secs(retry_after);
    log!("Encountered Retry-After header sleeping for {}", duration.as_secs())?;
    sleep(duration);
    bail!("Rate limited, retrying");
  }

  let (daily, rate_limit): (Vec<_>, Vec<_>) = RateLimit::from_headers(res.headers())
    .context("Failed to parse rate limit")?
    .into_iter()
    .partition(|limit| limit.name().eq_ignore_ascii_case("daily"));

  if let Some(daily) = daily.first()
    && daily.remaining() == 0
  {
    panic!("Exceeded daily rate limit. Please try again tomorrow.");
  }

  if let Some(rate_limit) = rate_limit.first() {
    HARDCOVER_AGENT.set_rate_limit(rate_limit.remaining() as usize);

    if rate_limit.remaining() == 0 {
      let duration = rate_limit.reset().unwrap_or(Duration::from_secs(1));
      log!("Reached rate limit sleeping for {}", duration.as_secs())?;
      sleep(duration);
    }
  }

  if !code.is_success() {
    let body = res.into_body().read_to_string()?;

    let msg = if let Some(json) = serde_json::from_str::<Value>(&body).ok()
      && let Some(error) = json.get("error").and_then(Value::as_str)
    {
      json
        .get("error_description")
        .or(json.get("message"))
        .and_then(Value::as_str)
        .map_or_else(|| error.to_string(), |desc| format!("{error} — {desc}"))
    } else {
      body
    };
    let msg = format!("Request failed <i>{code}: {msg}</i>");
    log!("{msg}")?;

    if code == StatusCode::UNAUTHORIZED {
      panic!(
        "Please set a valid Hardcover.app authorization token in <i>.adds/NickelHardcover/config.ini</i>.<br>>{msg}"
      )
    } else {
      bail!(msg);
    }
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

pub fn batch_requests<T: Serialize, I: Iterator<Item = T>, R: DeserializeOwned + Debug + AggregateErrors>(
  operation_name: &str,
  request_bodies: I,
) -> Result<Vec<R>> {
  request_bodies
    .batching(|it| {
      let chunk = it.take(HARDCOVER_AGENT.rate_limit()).collect::<Vec<_>>();
      if chunk.is_empty() {
        None
      } else {
        Some(send_request::<_, Vec<R>>(operation_name, chunk))
      }
    })
    .flatten_ok()
    .collect()
}
