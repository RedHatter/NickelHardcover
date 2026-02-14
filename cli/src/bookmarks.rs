use chrono::NaiveDate;
use rusqlite::{Connection, OpenFlags};

use crate::config::CONFIG;

#[derive(Debug)]
pub struct Bookmark {
  pub text: String,
  pub annotation: String,
  pub date_created: String,
  pub location: f64,
}

pub fn get_bookmarks(content_id: String, after_datetime: Option<&String>) -> Result<Vec<Bookmark>, String> {
  let connection = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
    .map_err(|e| format!("Failed to connect to the database <i>{}</i>: {e}", &CONFIG.sqlite_path))?;

  let total_word_count: f64 = connection
    .prepare(
      "SELECT SUM(WordCount)
      FROM content
      WHERE BookId = (?1)",
    )
    .map_err(|e| format!("Failed to parpare total word count query: {e}"))?
    .query_map([&content_id], |row| row.get(0))
    .map_err(|e| format!("Failed to run total word count query: {e}"))?
    .next()
    .ok_or("Total word count query returned no results")?
    .map_err(|e| format!("Failed to map total word count query result: {e}"))?;

  connection
    .prepare(
      "SELECT
        Text,
        Annotation,
        Bookmark.DateCreated,
        ChapterProgress,
        chapter.WordCount,
        SUM(before.WordCount)
      FROM Bookmark
      JOIN content AS chapter
        ON chapter.ContentId = Bookmark.ContentId
      JOIN content AS before
        ON before.BookId = VolumeID
        AND before.ContentType = 9
        AND before.VolumeIndex < chapter.VolumeIndex
      WHERE VolumeID = (?1)
      AND Bookmark.DateModified > (?2)
      AND Hidden = 'false'
      AND bookmark.Text != ''
      AND EXISTS (
        SELECT 1
        FROM content
        WHERE content.___UserId = Bookmark.UserId
      )
      GROUP BY Bookmark.BookmarkID;",
    )
    .map_err(|e| format!("Failed to parpare bookmark query: {e}"))?
    .query_map(
      [
        &content_id,
        after_datetime.unwrap_or(
          &NaiveDate::from_yo_opt(2000, 1)
            .ok_or("Failed to construct NaiveDate")?
            .and_hms_opt(1, 0, 0)
            .ok_or("Failed to construct NaiveDateTime")?
            .and_utc()
            .to_rfc3339(),
        ),
      ],
      |row| {
        let chapter_progress: f64 = row.get(3)?;
        let chapter_word_count: f64 = row.get(4)?;
        let bookmark_word_count: f64 = row.get(5)?;
        Ok(Bookmark {
          text: row.get(0)?,
          annotation: row.get(1)?,
          date_created: row.get(2)?,
          location: (bookmark_word_count + chapter_word_count * chapter_progress) / total_word_count,
        })
      },
    )
    .map_err(|e| format!("Failed to run bookmark query: {e}"))?
    .map(|row| row.map_err(|e| format!("Failed to map bookmark query result: {e}")))
    .collect()
}
