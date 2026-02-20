use std::fs;
use std::sync::LazyLock;

use serde::Serialize;
use serde::{Deserialize, Deserializer, de};

#[derive(Serialize, PartialEq, Debug)]
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
      s => Err(de::Error::custom(format!("{s} is not a valid sync_bookmarks value"))),
    }
  }
}

#[derive(Serialize, PartialEq, Debug)]
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
    } else if let Ok(n) = s.parse::<u8>() {
      Ok(SyncOnClose::Number(n))
    } else {
      Err(de::Error::custom(format!("{s} is not a valid sync_on_close value")))
    }
  }
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(default)]
pub struct Config {
  pub authorization: String,
  pub auto_sync_default: bool,
  pub sqlite_path: String,
  pub sync_bookmarks: SyncBookmarks,
  pub sync_on_close: SyncOnClose,
  pub threshold: u8,
}

impl Default for Config {
  fn default() -> Self {
    Self {
      authorization: "".into(),
      auto_sync_default: false,
      sqlite_path: "/mnt/onboard/.kobo/KoboReader.sqlite".into(),
      sync_bookmarks: SyncBookmarks::Always,
      sync_on_close: SyncOnClose::Always,
      threshold: 20,
    }
  }
}

pub static CONFIG: LazyLock<Config> = LazyLock::new(|| {
  let current_exe = std::env::current_exe()
    .map_err(|e| panic!("Failed to get current exe path: {e}"))
    .unwrap();
  let exe_dir = current_exe
    .as_path()
    .parent()
    .expect("Failed to get current exe directory");
  let config_path = exe_dir.join("config.ini");

  let config = if config_path.exists() {
    let content = fs::read_to_string(config_path)
      .map_err(|e| panic!("Failed to read config file: {e}"))
      .unwrap();
    let config: Config = serini::from_str(&content)
      .map_err(|e| panic!("Failed to parse config file: {e}"))
      .unwrap();

    config
  } else {
    let config = Config::default();
    let ini = serini::to_string(&config)
      .map_err(|e| panic!("Failed to parse default config: {e}"))
      .unwrap();
    fs::write(config_path, ini)
      .map_err(|e| panic!("Failed to write default config: {e}"))
      .unwrap();

    config
  };

  Config {
    authorization: if config.authorization.is_empty() || config.authorization.starts_with("Bearer") {
      config.authorization
    } else {
      "Bearer ".to_string() + &config.authorization
    },
    auto_sync_default: config.auto_sync_default,
    sqlite_path: exe_dir
      .join(config.sqlite_path)
      .to_str()
      .expect("Failed to get SQLite path")
      .to_string(),
    sync_bookmarks: config.sync_bookmarks,
    sync_on_close: config.sync_on_close,
    threshold: config.threshold,
  }
});
