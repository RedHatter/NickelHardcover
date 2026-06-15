use std::sync::OnceLock;

use anyhow::{Context, Result};
use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use macros::AggregateErrors;

use crate::config::JournalPrivacy;
use crate::hardcover::citext;
use crate::log;
use crate::utils::{GraphQLQueryExt, VERSION};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getme.graphql",
  response_derives = "Debug,AggregateErrors,Serialize",
  variables_derives = "Debug"
)]
pub struct GetMe;

pub async fn get_user() -> Result<&'static get_me::GetMeMe> {
  static USER: OnceLock<get_me::GetMeMe> = OnceLock::new();

  if let Some(user) = USER.get() {
    return Ok(user);
  }

  let user = GetMe::send_request(get_me::Variables {})
    .await?
    .me
    .into_iter()
    .next()
    .context("Failed to find Hardcover.app user")?;

  log!("user {}", user.id);

  Ok(USER.get_or_init(|| user))
}

/// Retrieve authenticated user.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user")]
pub struct GetUser {}

pub async fn run(args: GetUser) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let user = get_user().await?;

  log!(
    "BEGIN_JSON\n{}",
    json!({
      "id": user.id,
      "username": user.username,
      "account_privacy_setting_id": user.account_privacy_setting_id,
      "account_privacy_setting": JournalPrivacy::try_from(user.account_privacy_setting_id)
        .context("Failed to parse <i>account_privacy_setting_id</i>")?,
    })
  );

  Ok(())
}
