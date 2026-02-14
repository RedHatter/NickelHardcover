use std::collections::HashMap;
use std::io::prelude::*;
use std::{fs::File, path::Path};

use quick_xml::Reader;
use quick_xml::events::Event;
use quick_xml::name::QName;
use rusqlite::{Connection, OpenFlags};
use zip::ZipArchive;

use crate::config::CONFIG;

fn get_oebps_path(manifest: &str) -> Result<String, String> {
  let mut reader = Reader::from_str(manifest);

  loop {
    match reader.read_event() {
      Err(e) => {
        return Err(format!(
          "Error reading container manifest at position {}: {:?}",
          reader.error_position(),
          e
        ));
      }
      Ok(Event::Eof) => break,
      Ok(Event::Empty(e)) => {
        if e.name().as_ref() == b"rootfile" {
          let attrs: HashMap<_, _> = e
            .attributes()
            .filter_map(|attr| attr.ok().map(|attr| (attr.key, attr.unescape_value())))
            .collect();

          if attrs[&QName(b"media-type")]
            .as_ref()
            .map_err(|e| format!("Failed to decode `media-type` attribute value: {e}"))?
            == "application/oebps-package+xml"
          {
            return Ok(
              attrs[&QName(b"full-path")]
                .as_ref()
                .map_err(|e| format!("Failed to decode `full-path` attribute value: {e}"))?
                .to_string(),
            );
          }
        }
      }
      _ => (),
    }
  }

  Err("Failed to find oebps root file path".into())
}

fn get_identifiers(oebps: &str) -> Result<Vec<String>, String> {
  let mut reader = Reader::from_str(oebps);

  let mut vec: Vec<String> = Vec::new();
  let mut metadata_open = false;
  let mut identifier_open = false;

  loop {
    match reader.read_event() {
      Err(e) => {
        return Err(format!(
          "Error reading oebps file at position {}: {:?}",
          reader.error_position(),
          e
        ));
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
        if identifier_open && let Ok(content) = e.decode() {
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

fn read_epub_isbn(content_id: &str) -> Result<Vec<String>, String> {
  let file = File::open(Path::new(&content_id[7..])).map_err(|e| format!("Failed to open file: {e}"))?;
  let mut archive = ZipArchive::new(file).map_err(|e| format!("Failed to parse file as archive: {e}"))?;

  let mut manifest = String::new();
  archive
    .by_name("META-INF/container.xml")
    .map_err(|e| format!("Failed to open container manifest: {e}"))?
    .read_to_string(&mut manifest)
    .map_err(|e| format!("Failed to read container manifest: {e}"))?;
  let oebps_path = get_oebps_path(&manifest)?;

  let mut oebps = String::new();
  archive
    .by_name(&oebps_path)
    .map_err(|e| (format!("Failed to open oebps root file <i>{oebps_path}</i>: {e}")))?
    .read_to_string(&mut oebps)
    .map_err(|e| format!("Failed to read oebps root file: {e}"))?;

  let isbn = get_identifiers(&oebps)?;

  if isbn.is_empty() {
    panic!("Couldn't find an ISBN in the epub metadata. Please link book manually.");
  }

  println!("ISBN from epub `{}`", isbn.join(", "));

  Ok(isbn)
}

fn read_sqlite_isbn(content_id: &str) -> Result<String, String> {
  let isbn = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
    .map_err(|e| format!("Failed to connect to the database <i>{}</i>: {e}", &CONFIG.sqlite_path))?
    .prepare(
      "SELECT ISBN
      FROM content
      WHERE BookTitle is null
      AND ContentId is (?1)
      LIMIT 1;",
    )
    .map_err(|e| format!("Failed to parpare query: {e}"))?
    .query_map([&content_id], |row| row.get(0))
    .map_err(|e| format!("Failed to run query: {e}"))?
    .next()
    .ok_or("Query returned no results")?
    .map_err(|e| format!("Failed to map query result: {e}"))?;

  println!("ISBN from database `{isbn}`");

  Ok(isbn)
}

pub fn get_isbn(content_id: &str) -> Vec<String> {
  if content_id.starts_with("file://") {
    match read_epub_isbn(content_id) {
      Err(msg) => {
        panic!("Encountered an unexpeced error while parsing epub metadata. Please link book manually.<br><br>{msg}");
      }
      Ok(isbn) => isbn,
    }
  } else {
    match read_sqlite_isbn(content_id) {
      Err(msg) => {
        panic!(
          "Encountered an unexpeced error while fetching ISBN from the database. Please link book manually.<br><br>{msg}"
        );
      }
      Ok(isbn) => vec![isbn],
    }
  }
}
