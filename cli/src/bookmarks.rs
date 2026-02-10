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

pub fn get_bookmarks(content_id: String, after_datetime: Option<String>) -> Vec<Bookmark> {
  let connection = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY).expect(&format!(
    "Failed to connect to SQLite data base `{}`",
    &CONFIG.sqlite_path
  ));

  let total_word_count: f64 = connection
    .prepare(
      "SELECT SUM(WordCount)
      FROM content
      WHERE BookId = (?1)",
    )
    .expect("Failed to parpare SQLite total word count query")
    .query_map([&content_id], |row| row.get(0))
    .expect("Failed to query total word count from SQLite database")
    .next()
    .expect("Failed to map total word count from SQLite database")
    .expect("Failed to find total word count in SQLite database");

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
      AND Bookmark.DateCreated > (?2)
      AND Hidden = 'false'
      AND bookmark.Text != ''
      AND EXISTS (
        SELECT 1
        FROM content
        WHERE content.___UserId = Bookmark.UserId
      )
      GROUP BY Bookmark.BookmarkID;",
    )
    .expect("Failed to parpare SQLite bookmark query")
    .query_map(
      [
        &content_id,
        &after_datetime.unwrap_or(
          NaiveDate::from_yo_opt(2000, 1)
            .expect("Failed to construct NaiveDate")
            .and_hms_opt(1, 0, 0)
            .expect("Failed to construct NaiveDateTime")
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
    .expect("Failed to query bookmarks from SQLite database")
    .map(|row| row.expect("Failed to map bookmark in SQLite response"))
    .collect()
}
