use std::fmt::Debug;
use std::fs::write;
use std::sync::{LazyLock, Mutex};

use anyhow::{Context, Result};
use chrono::Local;
use graphql_client::{GraphQLQuery, Response};
use itertools::Itertools;
use serde_json::json;

use crate::config::CONFIG;
use crate::hardcover::send_request;
use crate::isbn::get_isbn;

#[allow(clippy::crate_in_macro_def)]
#[macro_export]
macro_rules! debug_log {
  ($($t:tt)*) => {{
    crate::utils::debug_log(&format!($($t)*));
  }};
}

#[allow(clippy::crate_in_macro_def)]
#[macro_export]
macro_rules! log {
  ($($t:tt)*) => {{
    let msg = format!($($t)*);
    crate::utils::debug_log(&msg);
    println!("{}", msg);
  }};
}

pub static VERSION: LazyLock<&str> = LazyLock::new(|| option_env!("VERSION").unwrap_or(env!("CARGO_PKG_VERSION")));

static LOG: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::new()));

pub fn debug_log(msg: &str) {
  LOG
    .lock()
    .unwrap()
    .push_str(&format!("{} {msg}\n", Local::now().format("%c")));
}

pub fn write_logfile() {
  let res = || -> Result<()> {
    write(
      std::env::current_exe()
        .context("Failed to get current binary path")?
        .as_path()
        .parent()
        .context("Failed to get current binary directory")?
        .join(Local::now().format("nickelhardcover_%Y-%m-%d_%H-%M-%S.log").to_string()),
      LOG.lock().unwrap().as_str(),
    )
    .context("Failed to write log file")
  }();

  if let Err(e) = res {
    eprintln!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }
}

pub fn normalize_identifiers(linked_id: Option<i64>, content_id: Option<&str>) -> (i64, Vec<String>) {
  match (linked_id, content_id) {
    (Some(linked_id), _) => (linked_id, Vec::new()),
    (_, Some(content_id)) => (0, get_isbn(&content_id)),
    (None, None) => panic!("One of --content-id or --linked-id is required"),
  }
}

pub fn book_not_found(msg: &str) -> ! {
  log!(
    "BEGIN_JSON\n{}",
    json!({ "error_code": "BOOK_NOT_FOUND", "message": msg })
  );

  if CONFIG.debug {
    write_logfile();
  }

  std::process::exit(0);
}

pub trait AggregateErrors {
  fn errors<'a>(&'a self) -> impl Iterator<Item = &'a str>;
}

impl<Data: AggregateErrors> AggregateErrors for graphql_client::Response<Data> {
  fn errors<'a>(&'a self) -> impl Iterator<Item = &'a str> {
    self
      .errors
      .iter()
      .flatten()
      .map(|e| e.message.as_str())
      .chain(self.data.iter().flat_map(AggregateErrors::errors))
  }
}

impl<T: AggregateErrors> AggregateErrors for Vec<T> {
  fn errors<'a>(&'a self) -> impl Iterator<Item = &'a str> {
    self.iter().flat_map(AggregateErrors::errors)
  }
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
  <T as GraphQLQuery>::ResponseData: Debug + AggregateErrors,
{
  fn send_request(variables: Self::Variables) -> Result<Self::ResponseData> {
    let body = Self::build_query(variables);
    debug_log!("{}, {:?}", body.operation_name, body.variables);
    send_request::<_, Response<Self::ResponseData>>(body.operation_name, &body)?
      .data
      .context(format!("{} response is None", body.operation_name))
  }
}
