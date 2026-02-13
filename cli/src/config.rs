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
      s => Err(de::Error::custom(format!("{s} is not a valid value"))),
    }
  }
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(default)]
pub struct Config {
  pub authorization: String,
  pub sqlite_path: String,
  pub sync_bookmarks: SyncBookmarks,
  pub frequency: i32,
}

impl Default for Config {
  fn default() -> Self {
    Self {
      authorization: "".into(),
      sqlite_path: "/mnt/onboard/.kobo/KoboReader.sqlite".into(),
      sync_bookmarks: SyncBookmarks::Always,
      frequency: 10,
    }
  }
}

pub static CONFIG: LazyLock<Config> = LazyLock::new(|| {
  let current_exe = std::env::current_exe().expect("Failed to get current exe path");
  let exe_dir = current_exe
    .as_path()
    .parent()
    .expect("Failed to get current exe directory");
  let config_path = exe_dir.join("config.ini");

  let config = if config_path.exists() {
    let content = fs::read_to_string(config_path).expect("Failed to read config file");
    let config: Config = serini::from_str(&content).expect("Failed to parse config file");

    config
  } else {
    let config = Config::default();
    let ini = serini::to_string(&config).expect("Failed to parse default config");
    fs::write(config_path, ini).expect("Failed to write default config");

    config
  };

  Config {
    authorization: config.authorization,
    sqlite_path: exe_dir
      .join(config.sqlite_path)
      .to_str()
      .expect("Failed to get SQLite path")
      .to_string(),
    sync_bookmarks: config.sync_bookmarks,
    frequency: config.frequency,
  }
});
