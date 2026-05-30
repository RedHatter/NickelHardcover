use std::io::prelude::*;
use std::{fs::File, path::Path};

use anyhow::{Context, Result, bail};
use itertools::Itertools;
use quick_xml::Reader;
use quick_xml::events::Event;
use rusqlite::{Connection, OpenFlags};
use zip::ZipArchive;

use crate::config::CONFIG;
use crate::log;

fn get_oebps_path(manifest: &str) -> Result<String> {
  let mut reader = Reader::from_str(manifest);

  loop {
    match reader.read_event() {
      Err(e) => {
        Err(e).context(format!(
          "Error reading container manifest at position {}",
          reader.error_position()
        ))?;
      }
      Ok(Event::Eof) => break,
      Ok(Event::Empty(e)) => {
        if e.name().as_ref() == b"rootfile" {
          let media_type = e
            .try_get_attribute("media-type")
            .context("Failed to decode <i>media-type</i> attribute")?;
          if let Some(media_type) = media_type
            && media_type
              .unescape_value()
              .context("Failed to decode <i>media-type</i> attribute value")?
              == "application/oebps-package+xml"
          {
            return Ok(
              e.try_get_attribute("full-path")
                .context("Failed to decode <i>full-path</i> attribute")?
                .context("Failed to find <i>full-path</i> attribute")?
                .unescape_value()
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

fn get_identifiers(oebps: &str) -> Result<Vec<String>> {
  let mut reader = Reader::from_str(oebps);

  let mut vec: Vec<String> = Vec::new();
  let mut metadata_open = false;
  let mut identifier_open = false;

  loop {
    match reader.read_event() {
      Err(e) => {
        Err(e).context(format!(
          "Error reading OEBPS file at position {}",
          reader.error_position()
        ))?;
      }
      Ok(Event::Eof) => break,
      Ok(Event::Start(e)) => match e.local_name().as_ref() {
        b"metadata" => metadata_open = true,
        b"identifier" => identifier_open = metadata_open,
        _ => (),
      },
      Ok(Event::End(e)) => match e.local_name().as_ref() {
        b"metadata" => break,
        b"identifier" => identifier_open = false,
        _ => (),
      },
      Ok(Event::Text(e)) => {
        if identifier_open {
          let content = e.decode().context("Failed to decode identifier text")?;
          let mut s = match content.rfind(':') {
            Some(i) => content[i..].to_string(),
            None => content.to_string(),
          };
          s.retain(char::is_alphanumeric);
          if (s.len() == 10 || s.len() == 13) && !vec.contains(&s) {
            vec.push(s);
          }
        }
      }
      _ => (),
    }
  }

  Ok(vec)
}

fn read_epub_isbn(content_id: &str) -> Result<Vec<String>> {
  let file = File::open(Path::new(&content_id[7..])).context("Failed to open file")?;
  let mut archive = ZipArchive::new(file).context("Failed to parse file as archive")?;

  let mut manifest = String::new();
  archive
    .by_name("META-INF/container.xml")
    .context("Failed to open container manifest")?
    .read_to_string(&mut manifest)
    .context("Failed to read container manifest")?;
  let oebps_path = get_oebps_path(&manifest)?;

  let mut oebps = String::new();
  archive
    .by_name(&oebps_path)
    .context(format!("Failed to open OEBPS root file <i>{oebps_path}</i>"))?
    .read_to_string(&mut oebps)
    .context("Failed to read OEBPS root file")?;

  let isbn = get_identifiers(&oebps)?;

  if isbn.is_empty() {
    panic!("Couldn't find an ISBN in the EPUB metadata. Please link book manually.");
  }

  log!("ISBN from EPUB `{}`", isbn.join(", "));

  Ok(isbn)
}

fn read_sqlite_isbn(content_id: &str) -> Result<String> {
  let maybe_isbn: Option<String> = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
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
    .query_map([&content_id], |row| row.get(0))
    .context("Failed to run query")?
    .next()
    .context("Query returned no results")?
    .context("Failed to map query result")?;

  let isbn = maybe_isbn.expect("Couldn't find an ISBN in the database. Please link book manually.");
  log!("ISBN from database `{isbn}`");

  Ok(isbn)
}

pub fn get_isbn(content_id: &str) -> Vec<String> {
  let res = if content_id.starts_with("file://") {
    read_epub_isbn(content_id)
  } else {
    read_sqlite_isbn(content_id).map(|isbn| vec![isbn])
  };

  match res {
    Err(e) => panic!(
      "Encountered an unexpected error while finding ISBN. Please link book manually.<br><br>{:#}",
      e.chain().join("<br>> ")
    ),
    Ok(isbn) => isbn,
  }
}
