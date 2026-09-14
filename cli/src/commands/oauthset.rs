use anyhow::{Context, Result};
use argh::FromArgs;
use jiff::{SignedDuration, Timestamp};
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::appcontext::AppContext;
use crate::config::{CLIENT_ID, VERSION};
use crate::utils::send_error;
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

pub fn run(context: &mut AppContext, args: &OAuthSet) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  request_token(
    context,
    [
      ("client_id", CLIENT_ID),
      ("grant_type", "urn:ietf:params:oauth:grant-type:device_code"),
      ("device_code", &args.device_code),
    ],
  )
}

pub fn refresh_token(context: &mut AppContext) -> Result<()> {
  request_token(
    context,
    [
      ("client_id", CLIENT_ID),
      ("grant_type", "refresh_token"),
      ("refresh_token", &context.config.refresh_token.clone()),
    ],
  )
}

pub fn request_token<I: IntoIterator<Item = (K, V)>, K: AsRef<str>, V: AsRef<str>>(
  context: &mut AppContext,
  body: I,
) -> Result<()> {
  let json = context
    .agent
    .post(format!("{}{}", context.config.hardcover_endpoint, "/oauth2/token"))
    .send_form(body)
    .context("Failed to send OAuth token request")?
    .body_mut()
    .read_json::<Value>()
    .context("Failed to parse OAuth token response")?;

  debug_log!("{:?}", json)?;

  if let Some(Value::String(error)) = json.get("error") {
    context.config.authorization = String::new();
    context.config.refresh_token = String::new();
    context.config.token_expires_at = None;
    context.config.write()?;
    send_error(
      context,
      "OAUTH",
      format!("Sign-in failed, please try again.<br><br><i>{}</i>", error),
    );
  } else {
    let value =
      serde_json::from_value::<OAuthToken>(json).context(format!("Failed to deserialize OAuth token response"))?;
    context.config.authorization = value.access_token;
    context.config.refresh_token = value.refresh_token;
    context.config.token_expires_at = Some(Timestamp::now() + SignedDuration::from_secs(value.expires_in));
    context.config.write()
  }
}
