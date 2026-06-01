use std::io::prelude::*;
use std::sync::LazyLock;
use std::{fs::File, path::Path};

use anyhow::{Context, Result, bail};
use itertools::Itertools;
use quick_xml::events::Event;
use quick_xml::{Reader, XmlVersion};
use regex::Regex;
use rusqlite::{Connection, OpenFlags};
use zip::ZipArchive;

use crate::config::CONFIG;
use crate::log;
use crate::utils::book_not_found;

fn isbn_13_check_digit(digits: &[u32]) -> u32 {
  let mut sum = 0;
  for i in 0..6 {
    sum += digits[i * 2] + 3 * digits[i * 2 + 1];
  }
  let sum_m = sum % 10;
  if sum_m == 0 { 0 } else { 10 - sum_m }
}

fn isbn_10_check_digit(digits: &[u32]) -> u32 {
  let mut sum = 0;
  for i in 0..9 {
    sum += digits[i] * (10 - i as u32);
  }

  let sum_m = sum % 11;
  if sum_m == 0 { 0 } else { 11 - sum_m }
}

fn normalize_isbn(isbn: &str) -> Option<Vec<String>> {
  let mut isbn = isbn.to_ascii_uppercase();
  isbn.retain(char::is_alphanumeric);

  if isbn.len() == 10 && isbn.starts_with("B") {
    return Some(vec![isbn]);
  }

  let digits = isbn
    .chars()
    .filter_map(|c| if c == 'X' { Some(10) } else { c.to_digit(10) })
    .collect::<Vec<_>>();

  if digits.len() == 13
    && (digits[..3] == [9, 7, 8] || digits[..3] == [9, 7, 9])
    && isbn_13_check_digit(&digits) == digits[12]
  {
    if digits[..3] == [9, 7, 8] {
      let mut isbn_10 = [0; 10];
      isbn_10[..9].clone_from_slice(&digits[3..12]);
      isbn_10[9] = isbn_10_check_digit(&isbn_10);

      Some(vec![
        isbn,
        isbn_10
          .iter()
          .map(|d| {
            if *d == 10 {
              'X'
            } else {
              char::from_digit(*d, 10).unwrap()
            }
          })
          .collect::<String>(),
      ])
    } else {
      Some(vec![isbn])
    }
  } else if digits.len() == 10 && isbn_10_check_digit(&digits) == digits[9] {
    let mut isbn_13 = [0; 13];
    isbn_13[0] = 9;
    isbn_13[1] = 7;
    isbn_13[2] = 8;
    isbn_13[3..12].clone_from_slice(&digits[..9]);
    isbn_13[12] = isbn_13_check_digit(&isbn_13);

    Some(vec![
      isbn,
      isbn_13
        .iter()
        .map(|d| char::from_digit(*d, 10).unwrap())
        .collect::<String>(),
    ])
  } else {
    None
  }
}

fn get_opf_path(manifest: &str) -> Result<String> {
  let mut reader = Reader::from_str(manifest);
  let mut xml_version = XmlVersion::Implicit1_0;

  loop {
    match reader.read_event() {
      Err(e) => {
        Err(e).context(format!(
          "Error reading container manifest at position {}",
          reader.error_position()
        ))?;
      }
      Ok(Event::Eof) => break,
      Ok(Event::Decl(e)) => {
        if let Ok(version) = e.xml_version() {
          xml_version = version;
        }
      }
      Ok(Event::Empty(e)) => {
        if e.name().as_ref() == b"rootfile" {
          let media_type = e
            .try_get_attribute("media-type")
            .context("Failed to decode <i>media-type</i> attribute")?;
          if let Some(media_type) = media_type
            && media_type
              .normalized_value(xml_version)
              .context("Failed to decode <i>media-type</i> attribute value")?
              == "application/oebps-package+xml"
          {
            return Ok(
              e.try_get_attribute("full-path")
                .context("Failed to decode <i>full-path</i> attribute")?
                .context("Failed to find <i>full-path</i> attribute")?
                .normalized_value(xml_version)
                .context("Failed to decode <i>full-path</i> attribute value")?
                .to_string(),
            );
          }
        }
      }
      _ => (),
    }
  }

  bail!("Failed to find OEBPS root file path")
}

fn read_opf(opf: &str) -> Result<(Vec<String>, Vec<String>)> {
  let mut reader = Reader::from_str(opf);

  let mut xml_version = XmlVersion::Implicit1_0;
  let mut isbns: Vec<String> = Vec::new();
  let mut items: Vec<String> = Vec::new();

  #[derive(Debug)]
  enum State {
    Start,
    Metadata,
    Identifier(String),
    Manifest,
  }
  let mut state = State::Start;

  loop {
    let event = reader.read_event().context(format!(
      "Error reading OEBPS file at position {}",
      reader.error_position()
    ))?;
    state = match (state, event) {
      (State::Start, Event::Decl(e)) => {
        if let Ok(version) = e.xml_version() {
          xml_version = version;
        }

        State::Start
      }
      (State::Start, Event::Start(e)) if e.local_name().as_ref() == b"metadata" => State::Metadata,
      (State::Metadata, Event::Start(e)) if e.local_name().as_ref() == b"identifier" => {
        State::Identifier(String::new())
      }
      (State::Identifier(s), Event::End(e)) if e.local_name().as_ref() == b"identifier" => {
        let i = s.rfind(':').unwrap_or(0);
        if let Some(normalized) = normalize_isbn(&s[i..]) {
          isbns.extend(normalized);
        }

        State::Metadata
      }
      (State::Metadata, Event::End(e)) if e.local_name().as_ref() == b"metadata" => State::Start,
      (State::Identifier(s), Event::Text(e)) => {
        State::Identifier(s + e.decode().context("Failed to decode identifier text")?.as_ref())
      }
      (State::Start, Event::Start(e)) if e.local_name().as_ref() == b"manifest" => State::Manifest,
      (State::Manifest, Event::Empty(e)) if e.local_name().as_ref() == b"item" => {
        if let Some(media_type) = e
          .try_get_attribute("media-type")
          .context("Failed to decode <i>media-type</i> attribute")?
        {
          let media_type = media_type
            .normalized_value(xml_version)
            .context("Failed to decode <i>href</i> attribute value")?
            .to_string();

          if (media_type == "application/xhtml+xml" || media_type == "application/x-dtbook+xml")
            && let Some(href) = e
              .try_get_attribute("href")
              .context("Failed to decode <i>href</i> attribute")?
          {
            items.push(
              href
                .normalized_value(xml_version)
                .context("Failed to decode <i>href</i> attribute value")?
                .to_string(),
            );
          }
        }

        State::Manifest
      }
      (State::Manifest, Event::End(e)) if e.local_name().as_ref() == b"manifest" => State::Start,
      (State::Start, Event::Eof) => break,
      (state, Event::Eof) => bail!("Unexpected end of file in {:?}", state),
      (state, _) => state,
    };
  }

  Ok((isbns, items))
}

fn read_item(item: &str) -> Result<Vec<String>> {
  static RE: LazyLock<Result<Regex, regex::Error>> = LazyLock::new(|| Regex::new(r"\s*([0-9\-\.–­―—\^ ]{9,22}[0-9xX])"));

  let mut reader = Reader::from_str(item);

  let mut text = String::new();
  let mut inside_body = false;

  loop {
    let event = reader.read_event().context(format!(
      "Error reading OEBPS file at position {}",
      reader.error_position()
    ))?;
    match event {
      Event::Start(e) if e.local_name().as_ref() == b"body" => inside_body = true,
      Event::End(e) if e.local_name().as_ref() == b"body" => break,
      Event::Text(e) if inside_body => {
        text += e.decode().context("Failed to decode text")?.as_ref();
      }
      Event::Eof => break,
      _ => {}
    };
  }

  Ok(
    RE.as_ref()?
      .captures_iter(&text)
      .filter_map(|c| c.get(0))
      .filter_map(|c| {
        let str = c.as_str();
        let s = if str.starts_with("-10 ") || str.starts_with("-13 ") {
          &str[4..]
        } else {
          str
        };
        normalize_isbn(&s)
      })
      .flatten()
      .collect::<Vec<_>>(),
  )
}

fn read_epub_isbn(content_id: &str) -> Result<Vec<String>> {
  let file = File::open(Path::new(&content_id[7..])).context("Failed to open file")?;
  let mut archive = ZipArchive::new(file).context("Failed to parse file as archive")?;

  let mut buf = String::new();
  archive
    .by_name("META-INF/container.xml")
    .context("Failed to open container manifest")?
    .read_to_string(&mut buf)
    .context("Failed to read container manifest")?;
  let opf_path = get_opf_path(&buf)?;

  buf.clear();
  archive
    .by_name(&opf_path)
    .context(format!("Failed to open OEBPS root file <i>{opf_path}</i>"))?
    .read_to_string(&mut buf)
    .context("Failed to read OEBPS root file")?;

  let (mut isbn, items) = read_opf(&buf)?;

  if isbn.is_empty() {
    let opf_dir = Path::new(&opf_path).parent().unwrap_or(Path::new("/"));
    for href in items {
      buf.clear();
      archive
        .by_path(opf_dir.join(&href))
        .context(format!("Failed to open OEBPS root file <i>{href}</i>"))?
        .read_to_string(&mut buf)
        .context("Failed to read OEBPS root file")?;
      isbn.extend(read_item(&buf)?);
    }
  }

  if isbn.is_empty() {
    book_not_found("Couldn't find an ISBN in the EPUB metadata. Please link book manually.")
  }

  isbn.sort();
  isbn.dedup();

  log!("ISBN from EPUB `{}`", isbn.join(", "));

  Ok(isbn)
}

fn read_sqlite_isbn(content_id: &str) -> Result<Vec<String>> {
  let isbn = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
    .context(format!(
      "Failed to connect to the database <i>{}</i>",
      &CONFIG.sqlite_path
    ))?
    .prepare(
      "SELECT ISBN
      FROM content
      WHERE BookTitle is null
      AND ContentId is (?1)
      LIMIT 1;",
    )
    .context("Failed to prepare query")?
    .query_map([&content_id], |row| row.get::<_, Option<String>>(0))
    .context("Failed to run query")?
    .next()
    .context("Query returned no results")?
    .context("Failed to map query result")?
    .and_then(|isbn| normalize_isbn(&isbn));

  match isbn {
    Some(isbn) => {
      log!("ISBN from database `{}`", isbn.join(", "));
      Ok(isbn)
    }
    None => book_not_found("Couldn't find an ISBN in the database. Please link book manually."),
  }
}

pub fn get_isbn(content_id: &str) -> Vec<String> {
  let res = if content_id.starts_with("file://") {
    read_epub_isbn(content_id)
  } else {
    read_sqlite_isbn(content_id)
  };

  match res {
    Ok(isbn) => isbn,
    Err(e) => book_not_found(&format!(
      "Encountered an unexpected error while finding ISBN. Please link book manually.<br><br>{:#}",
      e.chain().join("<br>> ")
    )),
  }
}
