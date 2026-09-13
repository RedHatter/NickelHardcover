use std::fmt::{Debug, Write};
use std::fs::write;
use std::sync::{LazyLock, Mutex};

use anyhow::{Context, Result};
use graphql_client::{GraphQLQuery, Response};
use itertools::Itertools;
use jiff::Zoned;

use crate::config::CONFIG;
use crate::database::get_sqlite_isbn;
use crate::epub::read_epub_isbn;
use crate::hardcover::send_request;
use crate::messages::{Error, Messages};

#[allow(clippy::crate_in_macro_def)]
#[macro_export]
macro_rules! debug_log {
  ($($t:tt)*) => {
    crate::utils::debug_log(&format!($($t)*))
  };
}

#[allow(clippy::crate_in_macro_def)]
#[macro_export]
macro_rules! log {
  ($($t:tt)*) => {{
    crate::utils::send_msg(&crate::messages::Messages::Log(crate::messages::Log {
      message: format!($($t)*),
    }))
  }};
}

pub static VERSION: LazyLock<&str> = LazyLock::new(|| option_env!("VERSION").unwrap_or(env!("CARGO_PKG_VERSION")));

static LOG: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::new()));

pub fn send_msg(value: &Messages) -> Result<()> {
  let message = serde_json::to_string(value).context("Failed to serialize message")?;

  debug_log(match value {
    Messages::Log(log) => &log.message,
    Messages::Error(err) => &err.message,
    _ => &message,
  })?;

  println!("{message}");

  Ok(())
}

pub fn debug_log(msg: &str) -> Result<()> {
  writeln!(LOG.lock().unwrap(), "{} {msg}", Zoned::now().strftime("%a %b %e %T %Y")).context("Failed to write to log")
}

pub fn write_logfile() -> Result<()> {
  write(
    std::env::current_exe()
      .context("Failed to get current binary path")?
      .as_path()
      .parent()
      .context("Failed to get current binary directory")?
      .join(Zoned::now().strftime("nickelhardcover_%F_%H-%M-%S.log").to_string()),
    LOG.lock().unwrap().as_str(),
  )
  .context("Failed to write log file")
}

pub fn normalize_identifiers(linked_id: Option<i64>, content_id: Option<&str>) -> (i64, Vec<String>) {
  match (linked_id, content_id) {
    (Some(linked_id), _) => (linked_id, Vec::new()),
    (_, Some(content_id)) => {
      let isbn = if content_id.starts_with("file://") {
        read_epub_isbn(content_id)
      } else {
        get_sqlite_isbn(content_id)
      };

      match isbn {
        Ok(isbn) => (0, isbn),
        Err(e) => book_not_found(&format!(
          "Failed to find an ISBN. Please link book manually.<br><br>{:#}",
          e.chain().join("<br>> ")
        )),
      }
    }
    (None, None) => panic!("One of --content-id or --linked-id is required"),
  }
}

pub fn book_not_found(message: &str) -> ! {
  send_msg(&Messages::Error(Error {
    error_code: "BOOK_NOT_FOUND".to_string(),
    message: message.to_string(),
  }))
  .expect("Failed to log `BOOK_NOT_FOUND` error");

  if CONFIG.debug
    && let Err(e) = write_logfile()
  {
    panic!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }

  std::process::exit(0);
}

pub trait GraphQLQueryExt
where
  Self: GraphQLQuery,
{
  fn send_request(variables: Self::Variables) -> Result<Self::ResponseData>;
}

impl<T: GraphQLQuery> GraphQLQueryExt for T
where
  <T as GraphQLQuery>::Variables: Debug,
  <T as GraphQLQuery>::ResponseData: Debug,
{
  fn send_request(variables: Self::Variables) -> Result<Self::ResponseData> {
    let body = Self::build_query(variables);
    debug_log!("{}, {:?}", body.operation_name, body.variables)?;
    send_request::<_, Response<Self::ResponseData>>(body.operation_name, &body)?
      .data
      .context(format!("{} response is None", body.operation_name))
  }
}
