use std::error::Error;
use std::fmt::Write as _;
use std::fs::write;
use std::sync::{LazyLock, Mutex};

use chrono::Local;

pub static VERSION: LazyLock<&str> = LazyLock::new(|| option_env!("VERSION").unwrap_or(env!("CARGO_PKG_VERSION")));

static LOG: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::new()));

pub fn debug_log(msg: &str) -> Result<(), String> {
  LOG
    .lock()
    .map_err(report("Failed to lock log"))?
    .push_str(&format!("{} {}\n", Local::now().format("%c"), msg));

  Ok(())
}

pub fn log(msg: String) -> Result<(), String> {
  debug_log(&msg)?;
  println!("{}", msg);

  Ok(())
}

pub fn write_logfile(include_time: bool) -> Result<(), String> {
  let current_exe = std::env::current_exe()
    .map_err(report("Failed to get current exe path"))
    .unwrap();
  let file = current_exe
    .as_path()
    .parent()
    .expect("Failed to get current exe directory")
    .join(if include_time {
      Local::now().format("nickelhardcover_%Y-%m-%d_%H-%M-%S.log").to_string()
    } else {
      "nickelhardcover.log".into()
    });

  write(file, LOG.lock().map_err(report("Failed to lock log"))?.as_str())
    .map_err(report("Failed to write log file"))?;

  Ok(())
}

pub fn report<E: Error>(msg: &str) -> impl FnOnce(E) -> String {
  move |e| {
    let mut err: &dyn Error = &e;
    let mut s = format!("{msg}<br>> {err}");
    while let Some(src) = err.source() {
      write!(s, "<br>> {src}").unwrap();
      err = src;
    }
    s
  }
}
