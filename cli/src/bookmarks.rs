use anyhow::{Context, Result};
use chrono::{DateTime, Utc};
use rusqlite::{Connection, OpenFlags};

use crate::config::CONFIG;

#[derive(Debug)]
pub struct Bookmark {
  pub id: String,
  pub text: String,
  pub annotation: Option<String>,
  pub date_created: DateTime<Utc>,
  pub location: Option<f64>,
}

pub fn get_bookmarks(content_id: String) -> Result<Vec<Bookmark>> {
  let connection = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY).context(
    format!("Failed to connect to the database <i>{}</i>", &CONFIG.sqlite_path),
  )?;

  let total_word_count: Option<f64> = connection
    .prepare("SELECT SUM(WordCount) FROM content WHERE BookId = (?1) AND WordCount > 0")
    .context("Failed to prepare total word count query")?
    .query_map([&content_id], |row| row.get(0))
    .context("Failed to run total word count query")?
    .next()
    .context("Total word count query returned no results")?
    .context("Failed to map total word count query result")?;

  connection
    .prepare(
      "SELECT
        BookmarkID,
        Text,
        Annotation,
        Bookmark.DateCreated,
        ChapterProgress,
        chapter.WordCount,
        SUM(before.WordCount)
      FROM Bookmark
      LEFT JOIN content AS chapter
        ON chapter.ContentId = Bookmark.ContentId
        AND chapter.WordCount > 0
      LEFT JOIN content AS before
        ON before.BookId = VolumeID
        AND before.WordCount > 0
        AND before.VolumeIndex < chapter.VolumeIndex
      WHERE VolumeID = (?1)
      AND Hidden = 'false'
      AND bookmark.Text != ''
      GROUP BY Bookmark.BookmarkID;",
    )
    .context("Failed to prepare bookmark query")?
    .query_map([&content_id], |row| {
      let chapter_progress: Option<f64> = row.get(4)?;
      let chapter_word_count: Option<f64> = row.get(5)?;
      let bookmark_word_count: Option<f64> = row.get(6)?;
      Ok(Bookmark {
        id: row.get(0)?,
        text: row.get(1)?,
        annotation: row.get(2)?,
        date_created: row.get(3)?,
        location: if let Some(chapter_progress) = chapter_progress
          && let Some(chapter_word_count) = chapter_word_count
          && let Some(total_word_count) = total_word_count
          && total_word_count > 0.0
        {
          Some((bookmark_word_count.unwrap_or(0.0) + chapter_word_count * chapter_progress) / total_word_count)
        } else {
          None
        },
      })
    })
    .context("Failed to run bookmark query")?
    .collect::<Result<Vec<_>, _>>()
    .context("Failed to map bookmark query result")
}
