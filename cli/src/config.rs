use core::fmt;
use std::fs;
use std::path::PathBuf;
use std::str::FromStr;

use anyhow::{Context, Result, anyhow};
use jiff::Timestamp;
use serde::Serialize;
use serde::{Deserialize, Deserializer, de};

use crate::appcontext::AppContext;
use crate::commands::getuser::get_user;

// pub static CLIENT_ID: &str = "f74912d3-4275-4935-803f-6b900042d63c";
// pub static BASE_URL: &str = "https://staging-api.hardcover.app";

pub static CLIENT_ID: &str = "2ec8855f-400e-4bb9-a2ed-5b628afa4a17";
pub static BASE_URL: &str = "https://api.hardcover.app";
pub static VERSION: &str = option_env!("VERSION").unwrap();

#[derive(Clone, Copy, Serialize, PartialEq, Debug)]
#[serde(rename_all = "lowercase")]
pub enum JournalPrivacy {
  Account,
  Public,
  Follows,
  Private,
}

impl JournalPrivacy {
  pub fn get_value(self, context: &mut AppContext) -> Result<i64> {
    match self {
      JournalPrivacy::Account => Ok(get_user(context)?.account_privacy_setting_id),
      _ => Ok(self as i64),
    }
  }
}

impl TryFrom<i64> for JournalPrivacy {
  type Error = anyhow::Error;

  fn try_from(value: i64) -> Result<Self> {
    match value {
      0 => Ok(JournalPrivacy::Account),
      1 => Ok(JournalPrivacy::Public),
      2 => Ok(JournalPrivacy::Follows),
      3 => Ok(JournalPrivacy::Private),
      _ => Err(anyhow!("<i>{value}</i> is not a valid <i>journal_privacy</i> value")),
    }
  }
}

impl FromStr for JournalPrivacy {
  type Err = String;

  fn from_str(value: &str) -> Result<Self, Self::Err> {
    match value {
      s if s.eq_ignore_ascii_case("Account") => Ok(JournalPrivacy::Account),
      s if s.eq_ignore_ascii_case("Public") => Ok(JournalPrivacy::Public),
      s if s.eq_ignore_ascii_case("Follows") => Ok(JournalPrivacy::Follows),
      s if s.eq_ignore_ascii_case("Private") => Ok(JournalPrivacy::Private),
      s => Err(format!("<i>{s}</i> is not a valid <i>journal_privacy</i> value")),
    }
  }
}

impl<'de> Deserialize<'de> for JournalPrivacy {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    Self::from_str(&String::deserialize(deserializer)?).map_err(de::Error::custom)
  }
}

impl fmt::Display for JournalPrivacy {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    let s = match self {
      JournalPrivacy::Account => "account",
      JournalPrivacy::Public => "public",
      JournalPrivacy::Follows => "follows",
      JournalPrivacy::Private => "private",
    };

    write!(f, "{s}")
  }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(default)]
pub struct Config {
  pub access_token: String,
  pub auto_sync_default: bool,
  pub debug: bool,
  pub journal_privacy: JournalPrivacy,
  pub refresh_token: String,
  pub retry_on_network: bool,
  pub sqlite_path: String,
  pub sync_annotations: bool,
  pub sync_on_close: i8,
  pub sync_on_read: i8,
  pub sync_on_schedule: i8,
  pub token_expires_at: Option<Timestamp>,
}

impl Default for Config {
  fn default() -> Self {
    Self {
      access_token: String::new(),
      auto_sync_default: false,
      debug: false,
      journal_privacy: JournalPrivacy::Account,
      refresh_token: String::new(),
      retry_on_network: false,
      sqlite_path: "/mnt/onboard/.kobo/KoboReader.sqlite".into(),
      sync_annotations: false,
      sync_on_close: -1,
      sync_on_read: -1,
      sync_on_schedule: -1,
      token_expires_at: None,
    }
  }
}

impl Config {
  pub fn get_dir() -> Result<PathBuf> {
    let current_exe = std::env::current_exe().context("Failed to get current binary path")?;
    Ok(
      current_exe
        .parent()
        .context("Failed to get current binary directory")?
        .into(),
    )
  }

  pub fn get_path() -> Result<PathBuf> {
    Ok(Config::get_dir()?.join("config.ini"))
  }

  pub fn read() -> Result<Option<Config>> {
    let config_path = Config::get_path()?;

    if config_path.exists() {
      let content = fs::read_to_string(config_path)
        .context("Failed to read config file")?
        .replace("[General]", "");
      Ok(Some(serini::from_str(&content).context("Failed to parse config file")?))
    } else {
      Ok(None)
    }
  }

  pub fn write(&self) -> Result<()> {
    let ini = serini::to_string(&self).context("Failed to serialize default config")?;
    fs::write(Config::get_path()?, ini).context("Failed to write default config")
  }
}
