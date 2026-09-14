use anyhow::{Context, Result};
use argh::FromArgs;
use serde_json::Value;

use crate::appcontext::AppContext;
use crate::config::{BASE_URL, CLIENT_ID, VERSION};
use crate::messages::Messages;
use crate::utils::{send_error, send_msg};
use crate::{debug_log, log};

/// Begin OAuth login flow
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "oauth-request")]
pub struct OAuthRequest {}

pub fn run(context: &mut AppContext, args: &OAuthRequest) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  let json = context
    .agent
    .post(format!("{}{}", BASE_URL, "/oauth2/device"))
    .send_form([
      ("client_id", CLIENT_ID),
      (
        "scope",
        "read:catalog read:library read:journal read:me:content write:library write:reviews",
      ),
    ])
    .context("Failed to send OAuth device request")?
    .body_mut()
    .read_json::<Value>()
    .context("Failed to parse OAuth device response")?;

  debug_log!("{:?}", json)?;

  if let Some(Value::String(error)) = json.get("error") {
    send_error(
      context,
      "OAUTH",
      format!("Sign-in failed, please try again.<br><br><i>{}</i>", error),
    );
  } else {
    send_msg(&Messages::OAuthDevice(
      serde_json::from_value(json).context(format!("Failed to deserialize OAuth device response"))?,
    ))
  }
}
