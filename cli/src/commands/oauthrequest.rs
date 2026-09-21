use anyhow::{Context, Result};
use argh::FromArgs;
use itertools::Itertools;
use retry::{
  delay::{Exponential, jitter},
  retry,
};

use crate::config::{BASE_URL, CLIENT_ID, VERSION};
use crate::hardcover::handle_response;
use crate::messages::Messages;
use crate::utils::send_msg;
use crate::{appcontext::AppContext, utils::ExpectedError};

/// Begin OAuth login flow
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "oauth-request")]
pub struct OAuthRequest {}

pub fn run(context: &mut AppContext, args: &OAuthRequest) -> Result<()> {
  log::info!("{VERSION} {args:?}");

  if let Err(err) = request_device_code(context) {
    return Err(ExpectedError::OAuth(format!("{:#}", err.chain().join("<br>> "))).into());
  }

  Ok(())
}

fn request_device_code(context: &mut AppContext) -> Result<()> {
  let json = retry(Exponential::from_millis(10).map(jitter).take(3), || {
    let res = context
      .agent
      .post(format!("{}{}", BASE_URL, "/oauth2/device"))
      .send_form([
        ("client_id", CLIENT_ID),
        (
          "scope",
          "read:catalog read:library read:journal read:me:content write:library write:reviews",
        ),
      ])
      .context("Failed to send OAuth device request")?;

    handle_response(context, res)
  })
  .map_err(|err| err.error)
  .context("OAuth device request failed")?;

  send_msg!(&Messages::OAuthDevice(
    serde_json::from_value(json).context("Failed to deserialize OAuth device response")?,
  ))
}
