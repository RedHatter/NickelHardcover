use std::sync::OnceLock;

use argh::FromArgs;
use graphql_client::GraphQLQuery;
use serde_json::json;

use crate::config::{VERSION, log};
use crate::hardcover::{citext, send_request};

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/query.graphql",
  response_derives = "Serialize,Debug,Clone",
  variables_derives = "Deserialize,Debug"
)]
pub struct GetUserId;

pub async fn get_user() -> Result<&'static get_user_id::GetUserIdMe, String> {
  static USER: OnceLock<get_user_id::GetUserIdMe> = OnceLock::new();

  if let Some(user) = USER.get() {
    return Ok(user);
  }

  let res = send_request::<get_user_id::Variables, get_user_id::ResponseData>(GetUserId::build_query(
    get_user_id::Variables {},
  ))
  .await?;

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
