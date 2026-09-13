use std::sync::OnceLock;

use anyhow::{Context, Result};
use argh::FromArgs;
use graphql_client::GraphQLQuery;

use crate::config::JournalPrivacy;
use crate::log;
use crate::messages::{Messages, User};
use crate::utils::{GraphQLQueryExt, VERSION, send_msg};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getme.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,Serialize",
  variables_derives = "Debug"
)]
pub struct GetMe;

pub fn get_user() -> Result<&'static get_me::GetMeMe> {
  static USER: OnceLock<get_me::GetMeMe> = OnceLock::new();

  if let Some(user) = USER.get() {
    return Ok(user);
  }

  let user = GetMe::send_request(get_me::Variables {})?
    .me
    .into_iter()
    .next()
    .context("Failed to find Hardcover.app user")?;

  log!("user {}", user.id)?;

  Ok(USER.get_or_init(|| user))
}

/// Retrieve authenticated user.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user")]
pub struct GetUser {}

pub fn run(args: &GetUser) -> Result<()> {
  log!("{} {:?}", &*VERSION, args)?;

  let user = get_user()?;

  send_msg(&Messages::User(User {
    id: user.id,
    username: user.username.clone(),
    account_privacy_setting_id: user.account_privacy_setting_id,
    account_privacy_setting: JournalPrivacy::try_from(user.account_privacy_setting_id)
      .context("Failed to parse <i>account_privacy_setting_id</i>")?
      .to_string(),
  }))
}
