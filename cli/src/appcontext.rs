use anyhow::{Context, Result};
use graphql_client::GraphQLQuery;
use ureq::Agent;

use crate::config::{Config, VERSION};
use crate::log;
use crate::utils::GraphQLQueryExt;

#[derive(GraphQLQuery)]
#[graphql(
  schema_path = "src/graphql/schema.graphql",
  query_path = "src/graphql/queries/getme.graphql",
  custom_scalars_module = "crate::hardcover::scalars"
  response_derives = "Debug,Serialize,Default",
  variables_derives = "Debug"
)]
pub struct GetMe;

pub struct AppContext {
  pub agent: Agent,
  pub config: Config,
  pub rate_limit: usize,
  pub user: get_me::GetMeMe,
}

impl AppContext {
  pub fn new() -> Result<AppContext> {
    let config_dir = Config::get_dir()?;

    let config = if let Some(config) = Config::read()? {
      config
    } else {
      let config = Config::default();
      config.write()?;
      config
    };

    let mut context = AppContext {
      agent: Agent::config_builder()
        .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), VERSION))
        .http_status_as_error(false)
        .build()
        .into(),
      config: Config {
        authorization: if let Some(auth) = config.authorization.strip_prefix("Bearer ") {
          auth.to_string()
        } else {
          config.authorization
        },
        sqlite_path: config_dir
          .join(config.sqlite_path)
          .to_str()
          .context("Failed to get SQLite path")?
          .to_string(),
        ..config
      },
      rate_limit: 1,
      user: get_me::GetMeMe::default(),
    };

    context.user = GetMe::send_request(&mut context, get_me::Variables {})?
      .me
      .into_iter()
      .next()
      .context("Failed to find Hardcover.app user")?;

    log!("user {}", context.user.id)?;

    Ok(context)
  }
}
