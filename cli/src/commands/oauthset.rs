use std::ops::Deref;

use anyhow::{Context, Result};
use argh::FromArgs;
use jiff::{SignedDuration, Timestamp};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::config::{CLIENT_ID, CONFIG};
use crate::hardcover::HARDCOVER_AGENT;
use crate::utils::{VERSION, send_error};
use crate::{debug_log, log};

#[derive(Serialize, Deserialize)]
struct OAuthToken {
  access_token: String,
  expires_in: i64,
  refresh_token: String,
  scope: String,
  token_type: String,
}

/// Get and store OAuth `access_token`, `refresh_token`, and expiration
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "oauth-set")]
pub struct OAuthSet {
  /// the device code returned by `oauth-request`
  #[argh(option)]
  device_code: String,
}

pub fn run(args: &OAuthSet) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  request_token([
    ("client_id", CLIENT_ID),
    ("grant_type", "urn:ietf:params:oauth:grant-type:device_code"),
    ("device_code", &args.device_code),
  ])
}

pub fn refresh_token() -> Result<()> {
  request_token([
    ("client_id", CLIENT_ID),
    ("grant_type", "refresh_token"),
    ("refresh_token", &CONFIG.refresh_token),
  ])
}

pub fn request_token<I: IntoIterator<Item = (K, V)>, K: AsRef<str>, V: AsRef<str>>(body: I) -> Result<()> {
  let json = HARDCOVER_AGENT
    .agent
    .post(format!("{}{}", &CONFIG.hardcover_endpoint, "/oauth2/token"))
    .send_form(body)
    .context("Failed to send OAuth token request")?
    .body_mut()
    .read_json::<Value>()
    .context("Failed to parse OAuth token response")?;

  debug_log!("{:?}", json)?;

  if let Some(Value::String(error)) = json.get("error") {
    let mut config = CONFIG.deref().clone();
    config.authorization = String::new();
    config.refresh_token = String::new();
    config.token_expires_at = None;
    config.write()?;
    send_error(
      "OAUTH",
      format!("Sign-in failed, please try again.<br><br><i>{}</i>", error),
    );
  } else {
    let value =
      serde_json::from_value::<OAuthToken>(json).context(format!("Failed to deserialize OAuth token response"))?;
    let mut config = CONFIG.deref().clone();
    config.authorization = value.access_token;
    config.refresh_token = value.refresh_token;
    config.token_expires_at = Some(Timestamp::now() + SignedDuration::from_secs(value.expires_in));
    config.write()
  }
}
