use std::sync::OnceLock;

use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use macros::{AggregateErrors, SendRequest};

use crate::hardcover::citext;
use crate::utils::{VERSION, log};

#[derive(GraphQLQuery, SendRequest)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Debug,AggregateErrors,Clone,Serialize",
  variables_derives = "Debug"
)]
pub struct GetUserId;

pub async fn get_user() -> Result<&'static get_user_id::GetUserIdMe, String> {
  static USER: OnceLock<get_user_id::GetUserIdMe> = OnceLock::new();

  if let Some(user) = USER.get() {
    return Ok(user);
  }

  let res = GetUserId::send_request(get_user_id::Variables {}).await?;
  let user = res.me.first().ok_or("Failed to find Hardcover.app user")?;

  log(format!("user {}", user.id))?;

  Ok(USER.get_or_init(|| user.to_owned()))
}

/// Retrieve authenticated user.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user")]
pub struct GetUser {}

pub async fn run(args: GetUser) -> Result<(), String> {
  log(format!("{} {:?}", &*VERSION, args))?;
  log(format!("BEGIN_JSON\n{}", json!(get_user().await?)))?;

  Ok(())
}
