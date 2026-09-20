use std::fmt::{Debug, Write};
use std::fs::write;
use std::str::FromStr;
use std::sync::Mutex;

use anyhow::{Context, Result, bail};
use graphql_client::{GraphQLQuery, Response};
use itertools::Itertools;
use jiff::Zoned;
use log::{Level, Log, Metadata, Record};
use strum_macros::AsRefStr;
use thiserror::Error;

use crate::appcontext::AppContext;
use crate::config::Config;
use crate::database::get_sqlite_isbn;
use crate::epub::read_epub_isbn;
use crate::hardcover::send_request;
use crate::messages::{Log as LogMessage, Messages};

macro_rules! send_msg {
  ($value:expr) => {{
    |value| -> anyhow::Result<()> {
      let message = anyhow::Context::context(serde_json::to_string(value), "Failed to serialize message")?;

      match value {
        Messages::Log(_) => {} // Already recorded by `BufferedLogger::log`
        Messages::Error(err) => log::error!("{}", err.message),
        _ => log::debug!("{message}"),
      }

      println!("{message}");

      Ok(())
    }($value)
  }};
}

pub(crate) use send_msg;

#[derive(Error, Debug, AsRefStr)]
pub enum ExpectedError {
  #[error("{0}")]
  BookInfo(String),
  #[error("{0}")]
  BookNotFound(String),
  #[error("Exceeded daily rate limit. Please try again tomorrow.")]
  DailyRateLimit,
  #[error("Sign-in failed, please try again.<br><br><i>{0}</i>")]
  OAuth(String),
  #[error("Unauthorized")]
  Unauthorized,
  #[error("{0}")]
  InvalidConfig(String),
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Percentage(pub f64);

impl FromStr for Percentage {
  type Err = String;

  fn from_str(s: &str) -> Result<Self, Self::Err> {
    let value: f64 = s.parse().map_err(|_| format!("<i>{s}</i> is not a valid number"))?;

    if (0.0..=100.0).contains(&value) {
      Ok(Percentage(value))
    } else {
      Err(format!(
        "<i>{value}</i> is not a valid percentage; expected a value between 0 and 100"
      ))
    }
  }
}

pub struct BufferedLogger {
  buffer: Mutex<String>,
}

impl BufferedLogger {
  const fn new() -> Self {
    BufferedLogger {
      buffer: Mutex::new(String::new()),
    }
  }

  pub fn write_to_disk(&self) -> Result<()> {
    write(
      Config::get_dir()?.join(Zoned::now().strftime("nickelhardcover_%F_%H-%M-%S.log").to_string()),
      self.buffer.lock().unwrap().as_str(),
    )
    .context("Failed to write log file")
  }
}

impl Log for BufferedLogger {
  fn enabled(&self, _metadata: &Metadata) -> bool {
    true
  }

  fn log(&self, record: &Record) {
    if !self.enabled(record.metadata()) {
      return;
    }

    {
      let mut buffer = self.buffer.lock().unwrap();
      let _ = writeln!(
        buffer,
        "[{} {:<5} {}] {}",
        Zoned::now().strftime("%Y-%m-%dT%H:%M:%S"),
        record.level(),
        record.target(),
        record.args()
      );
    }

    if record.level() == Level::Info {
      let _ = send_msg!(&Messages::Log(LogMessage {
        message: record.args().to_string(),
      }));
    }
  }

  fn flush(&self) {}
}

pub static LOGGER: BufferedLogger = BufferedLogger::new();

pub fn normalize_identifiers(
  context: &mut AppContext,
  linked_id: Option<i64>,
  content_id: Option<&str>,
) -> Result<(i64, Vec<String>)> {
  match (linked_id, content_id) {
    (Some(linked_id), _) if linked_id != 0 => Ok((linked_id, Vec::new())),
    (_, Some(content_id)) => {
      let isbn = if content_id.starts_with("file://") {
        read_epub_isbn(content_id)
      } else {
        get_sqlite_isbn(context, content_id)
      };

      match isbn {
        Ok(isbn) => Ok((0, isbn)),
        Err(e) => Err(
          ExpectedError::BookNotFound(format!(
            "Failed to find an ISBN. Please link book manually.<br><br>{:#}",
            e.chain().join("<br>> ")
          ))
          .into(),
        ),
      }
    }
    (_, None) => bail!("One of --content-id or --linked-id is required"),
  }
}

pub trait GraphQLQueryExt
where
  Self: GraphQLQuery,
{
  fn send_request(context: &mut AppContext, variables: Self::Variables) -> Result<Self::ResponseData>;
}

impl<T: GraphQLQuery> GraphQLQueryExt for T
where
  <T as GraphQLQuery>::Variables: Debug,
  <T as GraphQLQuery>::ResponseData: Debug,
{
  fn send_request(context: &mut AppContext, variables: Self::Variables) -> Result<Self::ResponseData> {
    let body = Self::build_query(variables);
    log::debug!("{}, {:?}", body.operation_name, body.variables);
    send_request::<_, Response<Self::ResponseData>>(context, body.operation_name, &body)?
      .data
      .context(format!("{} response is None", body.operation_name))
  }
}
