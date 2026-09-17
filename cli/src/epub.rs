use std::io::prelude::*;
use std::sync::LazyLock;
use std::{fs::File, path::Path};

use anyhow::{Context, Result, bail};
use quick_xml::events::Event;
use quick_xml::{Reader, XmlVersion};
use regex::Regex;
use zip::ZipArchive;

use crate::isbn::normalize_isbn;

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
      Ok(Event::Empty(e) | Event::Start(e)) if e.name().as_ref() == b"rootfile" => {
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
      _ => (),
    }
  }

  bail!("Failed to find OEBPS root file path")
}

fn read_opf(opf: &str) -> Result<(Vec<String>, Vec<String>)> {
  #[derive(Debug)]
  enum State {
    Start,
    Metadata,
    Identifier(String),
    Manifest,
  }

  let mut reader = Reader::from_str(opf);

  let mut xml_version = XmlVersion::Implicit1_0;
  let mut isbns = Vec::<String>::new();
  let mut items = Vec::<String>::new();

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
        let id = s.rfind(':').map_or(s.as_str(), |i| &s[i + 1..]);
        if let Some(normalized) = normalize_isbn(id) {
          isbns.extend(normalized);
        }

        State::Metadata
      }
      (State::Metadata, Event::End(e)) if e.local_name().as_ref() == b"metadata" => State::Start,
      (State::Identifier(s), Event::Text(e)) => {
        State::Identifier(s + e.decode().context("Failed to decode identifier text")?.as_ref())
      }
      (State::Start, Event::Start(e)) if e.local_name().as_ref() == b"manifest" => State::Manifest,
      (State::Manifest, Event::Empty(e) | Event::Start(e)) if e.local_name().as_ref() == b"item" => {
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
      (state, Event::Eof) => bail!("Unexpected end of file in {state:?}"),
      (state, _) => state,
    };
  }

  Ok((isbns, items))
}

fn read_item(item: &str) -> Result<Vec<String>> {
  enum State {
    Start,
    Body,
    Style,
    Script,
  }

  static RE: LazyLock<Result<Regex, regex::Error>> = LazyLock::new(|| {
    Regex::new(
      r"(?:-10 |-13 )?((?:[\u2010-\u2015\-\.\^ \t\u00a0\u00ad\u2212]?[0-9]){13}|(?:[\u2010-\u2015\-\.\^ \t\u00a0\u00ad\u2212]?[0-9]){9}[\u2010-\u2015\-\.\^ \t\u00a0\u00ad\u2212]]?[0-9xX])",
    )
  });

  let mut reader = Reader::from_str(item);
  let mut text = String::new();
  let mut state = State::Start;

  loop {
    let event = reader.read_event().context(format!(
      "Error reading OEBPS file at position {}",
      reader.error_position()
    ))?;
    state = match (state, event) {
      (State::Start, Event::Start(e)) if e.local_name().as_ref() == b"body" => State::Body,
      (State::Body, Event::End(e)) if e.local_name().as_ref() == b"body" => break,
      (State::Body, Event::Start(e)) if e.local_name().as_ref() == b"style" => State::Style,
      (State::Style, Event::End(e)) if e.local_name().as_ref() == b"style" => State::Body,
      (State::Body, Event::Start(e)) if e.local_name().as_ref() == b"script" => State::Script,
      (State::Script, Event::End(e)) if e.local_name().as_ref() == b"script" => State::Body,
      (State::Body, Event::Text(e)) => {
        text += e.decode().context("Failed to decode text")?.as_ref();
        State::Body
      }
      (_, Event::Eof) => break,
      (state, _) => state,
    };
  }

  Ok(
    RE.as_ref()?
      .captures_iter(&text)
      .filter_map(|c| c.get(0))
      .filter_map(|c| normalize_isbn(c.as_str()))
      .flatten()
      .collect(),
  )
}

pub fn read_epub_isbn(content_id: &str) -> Result<Vec<String>> {
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

    for n in 0..items.len() {
      let i = if n < 3 {
        n
      } else if n < 6 {
        items.len() - 1 - (n - 3)
      } else {
        n - 3
      };

      buf.clear();
      archive
        .by_path(opf_dir.join(&items[i]))
        .context(format!("Failed to open OEBPS root file <i>{}</i>", items[i]))?
        .read_to_string(&mut buf)
        .context("Failed to read OEBPS root file")?;
      isbn = read_item(&buf)?;

      if !isbn.is_empty() {
        break;
      }
    }
  }

  if isbn.is_empty() {
    bail!("No ISBN found in the EPUB metadata or content");
  }

  isbn.sort();
  isbn.dedup();

  Ok(isbn)
}

#[cfg(test)]
mod tests {
  use std::io::{Cursor, Write};
  use std::path::PathBuf;
  use std::sync::atomic::{AtomicU64, Ordering};

  use zip::write::{SimpleFileOptions, ZipWriter};

  use super::*;

  #[test]
  fn get_opf_path_open_close() {
    let xml = r#"<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"></rootfile>
  </rootfiles>
</container>"#;
    assert_eq!(get_opf_path(xml).unwrap(), "OEBPS/content.opf");
  }

  #[test]
  fn get_opf_path_wrong_media_type() {
    let xml = r#"<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="META-INF/other.xml" media-type="text/xml"/>
    <rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>"#;
    assert_eq!(get_opf_path(xml).unwrap(), "OEBPS/content.opf");
  }

  #[test]
  fn get_opf_path_missing() {
    let xml = r#"<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles></rootfiles>
</container>"#;
    assert!(get_opf_path(xml).is_err());
  }

  #[test]
  fn read_opf_manifest() {
    let opf = r#"<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf" xmlns:dc="http://purl.org/dc/elements/1.1/" version="2.0">
  <metadata>
    <dc:identifier opf:scheme="ISBN">urn:isbn:9780306406157</dc:identifier>
  </metadata>
  <manifest>
    <item href="style.css" media-type="text/css"/>
    <item href="cover.jpg" media-type="image/jpeg"/>
    <item href="toc.ncx" media-type="application/x-dtbncx+xml"/>
    <item id="chap1" href="chap1.xhtml" media-type="application/xhtml+xml"/>
    <item id="chap2" href="chap2.xhtml" media-type="application/xhtml+xml"></item>
    <item href="chap3.dtbook" media-type="application/x-dtbook+xml"/>
  </manifest>
</package>"#;

    let (isbns, items) = read_opf(opf).unwrap();
    assert_eq!(isbns, vec!["9780306406157".to_string(), "0306406152".to_string()]);
    assert_eq!(
      items,
      vec![
        "chap1.xhtml".to_string(),
        "chap2.xhtml".to_string(),
        "chap3.dtbook".to_string()
      ]
    );
  }

  #[test]
  fn read_opf_empty() {
    let opf = r#"<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf" version="2.0">
  <spine></spine>
</package>"#;

    let (isbns, items) = read_opf(opf).unwrap();
    assert!(isbns.is_empty());
    assert!(items.is_empty());
  }

  #[test]
  fn read_item_finds_isbn() {
    let xhtml = r#"<?xml version="1.0"?>
<html xmlns="http://www.w3.org/1999/xhtml">
  <body>
    <style>/* 978-0-13-468599-1 hidden in css, should be ignored */</style>
    <script>var isbn = "978-1-59420-171-4";</script>
    <p>Copyright page. ISBN 978-0-306-40615-7</p>
  </body>
</html>"#;

    let isbns = read_item(xhtml).unwrap();
    assert_eq!(isbns, vec!["9780306406157".to_string(), "0306406152".to_string()]);
  }

  static TEMP_COUNTER: AtomicU64 = AtomicU64::new(0);
  struct TempFile(PathBuf);

  impl TempFile {
    fn new(bytes: &[u8]) -> Self {
      let id = TEMP_COUNTER.fetch_add(1, Ordering::Relaxed);
      let path = std::env::temp_dir().join(format!("nickelhardcover-epub-test-{}-{id}.epub", std::process::id()));
      std::fs::write(&path, bytes).unwrap();
      TempFile(path)
    }

    fn content_id(&self) -> String {
      format!("file://{}", self.0.display())
    }
  }

  impl Drop for TempFile {
    fn drop(&mut self) {
      let _ = std::fs::remove_file(&self.0);
    }
  }

  fn build_epub(opf_path: &str, opf_contents: &str, items: &[(&str, &str)]) -> Vec<u8> {
    let container = format!(
      r#"<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="{opf_path}" media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>"#
    );

    let options = SimpleFileOptions::default();
    let mut writer = ZipWriter::new(Cursor::new(Vec::new()));

    writer.start_file("META-INF/container.xml", options).unwrap();
    writer.write_all(container.as_bytes()).unwrap();

    writer.start_file(opf_path, options).unwrap();
    writer.write_all(opf_contents.as_bytes()).unwrap();

    for (name, contents) in items {
      writer.start_file(*name, options).unwrap();
      writer.write_all(contents.as_bytes()).unwrap();
    }

    writer.finish().unwrap().into_inner()
  }

  #[test]
  fn read_epub_isbn_variants() {
    let isbn_9780306406157 = vec!["0306406152".to_string(), "9780306406157".to_string()];

    let opf = r#"<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf" xmlns:dc="http://purl.org/dc/elements/1.1/" version="2.0">
  <metadata>
    <dc:identifier opf:scheme="ISBN">urn:isbn:9780306406157</dc:identifier>
  </metadata>
  <manifest>
    <item href="chap1.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
</package>"#;
    let bytes = build_epub(
      "OEBPS/content.opf",
      opf,
      &[("OEBPS/chap1.xhtml", "<html><body><p>Hello</p></body></html>")],
    );
    let file = TempFile::new(&bytes);
    assert_eq!(read_epub_isbn(&file.content_id()).unwrap(), isbn_9780306406157);

    let opf = r#"<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf" version="2.0">
  <manifest>
    <item href="chap1.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
</package>"#;
    let chapter = r#"<html xmlns="http://www.w3.org/1999/xhtml">
  <body>
    <style>/* 979-10-30-015935 should be ignored */</style>
    <script>var isbn = "0-8044-2957-X"; // should be ignored</script>
    <p>Copyright page. ISBN 978-0-306-40615-7</p>
  </body>
</html>"#;
    let bytes = build_epub("OEBPS/content.opf", opf, &[("OEBPS/chap1.xhtml", chapter)]);
    let file = TempFile::new(&bytes);
    assert_eq!(read_epub_isbn(&file.content_id()).unwrap(), isbn_9780306406157);
  }

  #[test]
  fn read_epub_isbn_not_found() {
    let opf = r#"<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf" version="2.0">
  <manifest>
    <item href="chap1.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
</package>"#;

    let bytes = build_epub(
      "OEBPS/content.opf",
      opf,
      &[("OEBPS/chap1.xhtml", "<html><body><p>No numbers here.</p></body></html>")],
    );
    let file = TempFile::new(&bytes);

    assert!(read_epub_isbn(&file.content_id()).is_err());
  }
}
