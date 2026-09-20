use anyhow::{Context, Result};
use argh::FromArgs;
use graphql_client::GraphQLQuery;

use crate::appcontext::AppContext;
use crate::config::{JournalPrivacy, VERSION};
use crate::messages::{Messages, User};
use crate::utils::{GraphQLQueryExt, send_msg};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getme.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,Serialize,Default",
  variables_derives = "Debug"
)]
pub struct GetMe;

/// Retrieve authenticated user.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user")]
pub struct GetUser {}

pub fn get_user(context: &mut AppContext) -> Result<&get_me::GetMeMe> {
  if context.user.is_none() {
    let user = GetMe::send_request(context, get_me::Variables {})?
      .me
      .into_iter()
      .next()
      .context("Failed to find Hardcover.app user")?;
    log::info!("user {}", user.id);
    context.user = Some(user);
  }

  Ok(context.user.as_ref().unwrap())
}

pub fn run(context: &mut AppContext, args: &GetUser) -> Result<()> {
  log::info!("{VERSION} {args:?}");

  let user = get_user(context)?;

  send_msg!(&Messages::User(User {
    id: user.id,
    username: user.username.clone(),
    account_privacy_setting_id: user.account_privacy_setting_id,
    account_privacy_setting: JournalPrivacy::try_from(user.account_privacy_setting_id)
      .context("Failed to parse <i>account_privacy_setting_id</i>")?
      .to_string(),
  }))
}
