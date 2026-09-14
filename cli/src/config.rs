use core::fmt;
use std::fs;
use std::path::PathBuf;
use std::str::FromStr;
use std::sync::LazyLock;

use anyhow::{Context, Result, anyhow};
use itertools::Itertools;
use jiff::Timestamp;
use serde::Serialize;
use serde::{Deserialize, Deserializer, de};

use crate::commands::getuser::get_user;

pub static CLIENT_ID: &str = "f74912d3-4275-4935-803f-6b900042d63c";

#[derive(Serialize, PartialEq, Debug, Clone)]
#[serde(rename_all = "lowercase")]
pub enum SyncBookmarks {
  Always,
  Never,
  Finished,
}

impl<'de> Deserialize<'de> for SyncBookmarks {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    match String::deserialize(deserializer)? {
      s if s.eq_ignore_ascii_case("Always") => Ok(SyncBookmarks::Always),
      s if s.eq_ignore_ascii_case("Never") => Ok(SyncBookmarks::Never),
      s if s.eq_ignore_ascii_case("Finished") => Ok(SyncBookmarks::Finished),
      s => Err(de::Error::custom(format!(
        "<i>{s}</i> is not a valid <i>sync_bookmarks</i> value"
      ))),
    }
  }
}

#[derive(Clone, Copy, Serialize, PartialEq, Debug)]
#[serde(rename_all = "lowercase")]
pub enum JournalPrivacy {
  Account,
  Public,
  Follows,
  Private,
}

impl JournalPrivacy {
  pub fn get_value(self) -> Result<i64> {
    match self {
      JournalPrivacy::Account => Ok(get_user()?.account_privacy_setting_id),
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

#[derive(Serialize, PartialEq, Debug, Clone)]
#[serde(rename_all = "lowercase")]
pub enum SyncOnClose {
  Always,
  Never,
  Number(u8),
}

impl<'de> Deserialize<'de> for SyncOnClose {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    let s = String::deserialize(deserializer)?;

    if s.eq_ignore_ascii_case("Always") {
      Ok(SyncOnClose::Always)
    } else if s.eq_ignore_ascii_case("Never") {
      Ok(SyncOnClose::Never)
    } else if let Ok(n) = s.parse::<u8>()
      && n > 1
      && n <= 100
    {
      Ok(SyncOnClose::Number(n))
    } else {
      Err(de::Error::custom(format!(
        "<i>{s}</i> is not a valid <i>sync_on_close</i> value"
      )))
    }
  }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(default)]
pub struct Config {
  pub authorization: String,
  pub auto_sync_default: bool,
  pub debug: bool,
  pub hardcover_endpoint: String,
  pub journal_privacy: JournalPrivacy,
  pub refresh_token: String,
  pub retry_on_network: bool,
  pub sqlite_path: String,
  pub sync_bookmarks: SyncBookmarks,
  pub sync_daily: i8,
  pub sync_on_close: SyncOnClose,
  pub threshold: u8,
  pub token_expires_at: Option<Timestamp>,
}

impl Default for Config {
  fn default() -> Self {
    Self {
      authorization: String::new(),
      auto_sync_default: false,
      debug: false,
      hardcover_endpoint: "https://api.hardcover.app".into(),
      journal_privacy: JournalPrivacy::Account,
      refresh_token: String::new(),
      retry_on_network: false,
      sqlite_path: "/mnt/onboard/.kobo/KoboReader.sqlite".into(),
      sync_bookmarks: SyncBookmarks::Never,
      sync_daily: -1,
      sync_on_close: SyncOnClose::Never,
      threshold: 0,
      token_expires_at: None,
    }
  }
}

impl Config {
  fn get_dir() -> Result<PathBuf> {
    let current_exe = std::env::current_exe().context("Failed to get current binary path")?;
    Ok(
      current_exe
        .parent()
        .context("Failed to get current binary directory")?
        .into(),
    )
  }

  fn get_path() -> Result<PathBuf> {
    Ok(Config::get_dir()?.join("config.ini"))
  }

  fn read() -> Result<Option<Config>> {
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

pub static CONFIG: LazyLock<Config> = LazyLock::new(|| {
  let config = || -> Result<Config> {
    let config_dir = Config::get_dir()?;

    let config = if let Some(config) = Config::read()? {
      config
    } else {
      let config = Config::default();
      config.write()?;
      config
    };

    Ok(Config {
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
    })
  }();

  match config {
    Ok(config) => config,
    Err(e) => {
      panic!(
        "Encountered an unexpected error. Please report this.<br><br>{:#}",
        e.chain().join("<br>> ")
      );
    }
  }
});
