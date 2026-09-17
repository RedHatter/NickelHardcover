//! Adapted from https://docs.rs/trillium-ratelimit/latest/src/trillium_ratelimit/headers/rate_limit.rs.html

#![allow(dead_code)]

use sfparse::{Parser, Value};
use std::{
  borrow::Cow,
  fmt::{self, Display},
  time::Duration,
};
use ureq::http::HeaderMap;

/// The error returned when a `RateLimit` / `RateLimit-Policy` header value is not well-formed as
/// an RFC 9651 Structured Field. Per the RateLimit draft, a caller that receives this should
/// treat the field as carrying no rate-limit information.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ParseError;

impl fmt::Display for ParseError {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    f.write_str("malformed RateLimit structured field")
  }
}

impl std::error::Error for ParseError {}

const RATE_LIMIT_HEADER: &str = "RateLimit";

mod sf {
  //! Small shared helpers for parsing and emitting RFC 9651 Structured Field items.
  //!
  //! `sfparse` hands back byte ranges into the input plus an `escape` flag rather than decoded
  //! values (that's what makes it zero-allocation); these helpers turn those ranges into `Cow`s,
  //! write the quoting/escaping back out, and skip over list items we don't model.

  use super::ParseError;
  use sfparse::{Parser, Value};
  use std::{
    borrow::Cow,
    fmt::{self, Write},
    ops::Range,
  };

  /// Materialize an SF String from its source `range` and `escape` flag: borrow when there are no
  /// escape sequences, otherwise allocate and strip the backslashes.
  pub(crate) fn string_value(input: &str, range: Range<usize>, escape: bool) -> Cow<'_, str> {
    let raw = &input[range];
    if escape {
      Cow::Owned(unescape(raw))
    } else {
      Cow::Borrowed(raw)
    }
  }

  fn unescape(raw: &str) -> String {
    let mut out = String::with_capacity(raw.len());
    let mut chars = raw.chars();
    while let Some(c) = chars.next() {
      if c == '\\' {
        if let Some(escaped) = chars.next() {
          out.push(escaped);
        }
      } else {
        out.push(c);
      }
    }
    out
  }

  /// Write `value` as an SF String — wrapped in double quotes with `\` and `"` backslash-escaped.
  pub(crate) fn write_sf_string(f: &mut fmt::Formatter<'_>, value: &str) -> fmt::Result {
    f.write_char('"')?;
    for c in value.chars() {
      if matches!(c, '\\' | '"') {
        f.write_char('\\')?;
      }
      f.write_char(c)?;
    }
    f.write_char('"')
  }

  /// Drain and discard the remaining parameters of the current item.
  pub(crate) fn drain_params(parser: &mut Parser<'_>) -> Result<(), ParseError> {
    while parser.parse_param().map_err(|_| ParseError)?.is_some() {}
    Ok(())
  }

  /// Skip a list item we don't model: drain any inner-list members, then its parameters.
  pub(crate) fn skip_item(parser: &mut Parser<'_>, value: &Value) -> Result<(), ParseError> {
    if matches!(value, Value::InnerList) {
      while parser.parse_inner_list().map_err(|_| ParseError)?.is_some() {}
    }
    drain_params(parser)
  }
}

///
/// A single service limit — one item of the `RateLimit` header field's Structured Field List.
///
/// Where [`RateLimitPolicy`] advertises a server's *standing* allocation, `RateLimit` reports the
/// *current* state for a particular partition: the available quota `r` (remaining, required), an
/// optional effective window `t` (the [`reset`] delay until quota replenishes, in seconds, rather
/// than an absolute timestamp), and an optional partition key `pk`. The `name` ties it to the
/// [`RateLimitPolicy`] of the same name.
///
/// Like [`RateLimitPolicy`], this type is unopinionated and represents the full range the field
/// can express. Parse with [`from_headers`] / [`parse_list`]; format one item via [`Display`].
///
/// [`RateLimitPolicy`]: crate::headers::RateLimitPolicy
/// [`reset`]: RateLimit::reset
/// [`from_headers`]: RateLimit::from_headers
/// [`parse_list`]: RateLimit::parse_list
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RateLimit<'a> {
  name: Cow<'a, str>,
  remaining: u64,
  reset: Option<Duration>,
  partition_key: Option<Cow<'a, str>>,
}

impl<'a> RateLimit<'a> {
  /// Builds a service limit named `name` reporting `remaining` available units, with no reset
  /// window or partition key.
  pub fn new(name: impl Into<Cow<'a, str>>, remaining: u64) -> Self {
    Self {
      name: name.into(),
      remaining,
      reset: None,
      partition_key: None,
    }
  }

  /// Sets the effective window — the delay until the available quota is replenished (the `t`
  /// parameter).
  pub fn with_reset(mut self, reset: Duration) -> Self {
    self.reset = Some(reset);
    self
  }

  /// Sets the partition key (the `pk` parameter), as its base64 wire form.
  pub fn with_partition_key(mut self, partition_key: impl Into<Cow<'a, str>>) -> Self {
    self.partition_key = Some(partition_key.into());
    self
  }

  /// The name of the policy this service limit reports on.
  pub fn name(&self) -> &str {
    &self.name
  }

  /// The available quota under the named policy (the `r` parameter).
  pub fn remaining(&self) -> u64 {
    self.remaining
  }

  /// The effective window — the delay until quota is replenished (the `t` parameter), if
  /// advertised.
  pub fn reset(&self) -> Option<Duration> {
    self.reset
  }

  /// The partition key (the `pk` parameter), as its base64-encoded wire form, if present.
  pub fn partition_key(&self) -> Option<&str> {
    self.partition_key.as_deref()
  }

  /// Parses all service limits from a single `RateLimit` field value (a Structured Field List of
  /// service-limit items). Returns [`ParseError`] if the value is not well-formed.
  pub fn parse_list(input: &'a str) -> Result<Vec<Self>, ParseError> {
    let mut parser = Parser::new(input.as_bytes());
    let mut limits = Vec::new();

    while let Some(value) = parser.parse_list().map_err(|_| ParseError)? {
      let name = match value {
        Value::String { range, escape } => sf::string_value(input, range, escape),
        other => {
          sf::skip_item(&mut parser, &other)?;
          continue;
        }
      };

      let mut remaining = None;
      let mut reset = None;
      let mut partition_key = None;

      while let Some((key, param)) = parser.parse_param().map_err(|_| ParseError)? {
        match (key, param) {
          ("r", Value::Integer(value)) if value >= 0 => remaining = Some(value.cast_unsigned()),
          ("t", Value::Integer(value)) if value >= 0 => {
            reset = Some(Duration::from_secs(value.cast_unsigned()));
          }
          ("pk", Value::ByteSeq(range)) => {
            partition_key = Some(Cow::Borrowed(&input[range]));
          }
          _ => {}
        }
      }

      // The remaining `r` parameter is required.
      if let Some(remaining) = remaining {
        limits.push(Self {
          name,
          remaining,
          reset,
          partition_key,
        });
      }
    }

    Ok(limits)
  }

  /// Parses every service limit across all `RateLimit` field lines in `headers`.
  pub fn from_headers(headers: &'a HeaderMap) -> Result<Vec<Self>, ParseError> {
    let mut limits = Vec::new();
    for value in headers.get_all(RATE_LIMIT_HEADER) {
      limits.extend(Self::parse_list(value.to_str().map_err(|_| ParseError)?)?);
    }
    Ok(limits)
  }

  /// Converts a borrowed service limit into an owned `RateLimit<'static>`.
  pub fn into_owned(self) -> RateLimit<'static> {
    RateLimit {
      name: Cow::Owned(self.name.into_owned()),
      remaining: self.remaining,
      reset: self.reset,
      partition_key: self.partition_key.map(|pk| Cow::Owned(pk.into_owned())),
    }
  }
}

impl Display for RateLimit<'_> {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    sf::write_sf_string(f, &self.name)?;
    write!(f, ";r={}", self.remaining)?;
    if let Some(reset) = self.reset {
      write!(f, ";t={}", reset.as_secs())?;
    }
    if let Some(partition_key) = &self.partition_key {
      write!(f, ";pk=:{partition_key}:")?;
    }
    Ok(())
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  fn one(input: &str) -> RateLimit<'_> {
    let mut limits = RateLimit::parse_list(input).unwrap();
    assert_eq!(limits.len(), 1, "expected exactly one limit in {input:?}");
    limits.pop().unwrap()
  }

  #[test]
  fn basic() {
    let limit = one(r#""default";r=50;t=30"#);
    assert_eq!(limit.name(), "default");
    assert_eq!(limit.remaining(), 50);
    assert_eq!(limit.reset(), Some(Duration::from_secs(30)));
    assert_eq!(limit.partition_key(), None);
  }

  #[test]
  fn throttled_zero_remaining() {
    let limit = one(r#""default";r=0;t=5"#);
    assert_eq!(limit.remaining(), 0);
    assert_eq!(limit.reset(), Some(Duration::from_secs(5)));
  }

  #[test]
  fn reset_is_optional() {
    let limit = one(r#""default";r=999;pk=:dHJpYWwxMjEzMjM=:"#);
    assert_eq!(limit.reset(), None);
    assert_eq!(limit.partition_key(), Some("dHJpYWwxMjEzMjM="));
  }

  #[test]
  fn multiple_limits() {
    let limits = RateLimit::parse_list(r#""a";r=1;t=2,"b";r=3"#).unwrap();
    assert_eq!(limits.len(), 2);
    assert_eq!(limits[1].name(), "b");
    assert_eq!(limits[1].remaining(), 3);
    assert_eq!(limits[1].reset(), None);
  }

  #[test]
  fn item_without_remaining_is_skipped() {
    assert!(RateLimit::parse_list(r#""x";t=5"#).unwrap().is_empty());
  }

  #[test]
  fn display_round_trips() {
    for input in [
      r#""default";r=50;t=30"#,
      r#""default";r=0;t=5"#,
      r#""default";r=999;pk=:dHJpYWwxMjEzMjM=:"#,
    ] {
      let limit = one(input);
      let formatted = limit.to_string();
      assert_eq!(limit, one(&formatted), "round trip of {input:?} via {formatted:?}");
    }
  }

  #[test]
  fn builder_formats() {
    let limit = RateLimit::new("default", 50).with_reset(Duration::from_secs(30));
    assert_eq!(limit.to_string(), r#""default";r=50;t=30"#);
  }
}
