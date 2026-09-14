use anyhow::{Context, Result};
use argh::FromArgs;

use crate::appcontext::AppContext;
use crate::config::{JournalPrivacy, VERSION};
use crate::log;
use crate::messages::{Messages, User};
use crate::utils::send_msg;

/// Retrieve authenticated user.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user")]
pub struct GetUser {}

pub fn run(context: &mut AppContext, args: &GetUser) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  send_msg(&Messages::User(User {
    id: context.user.id,
    username: context.user.username.clone(),
    account_privacy_setting_id: context.user.account_privacy_setting_id,
    account_privacy_setting: JournalPrivacy::try_from(context.user.account_privacy_setting_id)
      .context("Failed to parse <i>account_privacy_setting_id</i>")?
      .to_string(),
  }))
}
