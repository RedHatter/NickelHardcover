use anyhow::{Context, Result};
use ureq::Agent;

use crate::{
  commands::getuser::get_me,
  config::{Config, VERSION},
};

pub struct AppContext {
  pub agent: Agent,
  pub config: Config,
  pub rate_limit: usize,
  pub user: Option<get_me::GetMeMe>,
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

    Ok(AppContext {
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
      user: None,
    })
  }
}
