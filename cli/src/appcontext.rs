use anyhow::{Context, Result};
use itertools::Itertools;
use ureq::Agent;

use crate::{
  commands::getuser::get_me,
  config::{Config, JournalPrivacy, VERSION},
  utils::ExpectedError,
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

    let config = match Config::read() {
      Ok(Some(config)) => config,
      Ok(None) => {
        let config = Config {
          auto_sync_default: false,
          journal_privacy: JournalPrivacy::Account,
          retry_on_network: true,
          sync_annotations: true,
          sync_on_close: 1,
          sync_on_read: 20,
          sync_on_schedule: -1,
          ..Config::default()
        };
        config.write()?;
        config
      }
      Err(e) => return Err(ExpectedError::InvalidConfig(format!(
        "Your settings couldn't be read. The file may be corrupted or from a version of NickelHardcover prior to 1.0.<br><br>Resetting will restore the default settings.<br><br>{:#}",
        e.chain().join("<br>> ")
      )).into()),
    };

    Ok(AppContext {
      agent: Agent::config_builder()
        .user_agent(format!("{}/{}", env!("CARGO_PKG_NAME"), VERSION))
        .http_status_as_error(false)
        .build()
        .into(),
      config: Config {
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
