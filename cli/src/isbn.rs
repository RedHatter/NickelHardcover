use std::collections::HashMap;
use std::io::prelude::*;
use std::{fs::File, path::Path};

use quick_xml::Reader;
use quick_xml::events::Event;
use quick_xml::name::QName;
use rusqlite::{Connection, OpenFlags};
use zip::ZipArchive;

use crate::config::CONFIG;

fn get_oebps_path(manifest: &str) -> Option<String> {
  let mut reader = Reader::from_str(manifest);

  loop {
    match reader.read_event() {
      Err(e) => panic!(
        "Error reading container manifest at position {}: {:?}",
        reader.error_position(),
        e
      ),
      Ok(Event::Eof) => break,
      Ok(Event::Empty(e)) => {
        if e.name().as_ref() == b"rootfile" {
          let attrs: HashMap<_, _> = e
            .attributes()
            .map(|a| {
              let attr = a.unwrap();
              (attr.key, attr.unescape_value())
            })
            .collect();

          if attrs[&QName(b"media-type")]
            .as_ref()
            .expect("Failed to decode attribute value")
            == "application/oebps-package+xml"
          {
            return Some(
              attrs[&QName(b"full-path")]
                .as_ref()
                .expect("Failed to decode attribute value")
                .to_string(),
            );
          }
        }
      }
      _ => (),
    }
  }

  None
}

fn get_identifiers(oebps: &str) -> Vec<String> {
  let mut reader = Reader::from_str(oebps);

  let mut vec: Vec<String> = Vec::new();
  let mut metadata_open = false;
  let mut identifier_open = false;

  loop {
    match reader.read_event() {
      Err(e) => panic!(
        "Error reading oebps file at position {}: {:?}",
        reader.error_position(),
        e
      ),
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

  vec
}

pub fn get_isbn(content_id: String) -> Vec<String> {
  let isbn = if content_id.starts_with("file://") {
    let file = File::open(Path::new(&content_id[7..]))
      .expect(&format!("Failed to open book file `{content_id}`"));
    let mut archive =
      ZipArchive::new(file).expect("Failed to open book file `{content_id}` as archive");

    let mut manifest = String::new();
    archive
      .by_name("META-INF/container.xml")
      .expect(&format!(
        "Failed to read container manifest in `{content_id}`"
      ))
      .read_to_string(&mut manifest)
      .unwrap();
    let oebps_path =
      get_oebps_path(&manifest).expect(&format!("Failed to find oebps rootfile in `{content_id}`"));

    let mut oebps = String::new();
    archive
      .by_name(&oebps_path)
      .expect(&format!("Failed to read oebps file in `{content_id}`"))
      .read_to_string(&mut oebps)
      .unwrap();

    get_identifiers(&oebps)
  } else {
    let isbn = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
      .expect(&format!(
        "Failed to connect to SQLite data base `{}`",
        &CONFIG.sqlite_path
      ))
      .prepare("SELECT ISBN FROM content WHERE BookTitle is null AND ContentId is (?1) LIMIT 1;")
      .expect("Failed to parpare SQLite query")
      .query_map([&content_id], |row| row.get(0))
      .expect("Failed to query rows from SQLite database")
      .next()
      .expect(&format!(
        "Failed to find content id `{content_id}` in SQLite database",
      ))
      .expect("Failed to find ISBN in row with content id `{content_id}` in SQLite database");

    vec![isbn]
  };

  if isbn.is_empty() {
    panic!("Failed to find ISBN for book `{content_id}`");
  }

  isbn
}
