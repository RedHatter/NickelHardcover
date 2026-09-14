use anyhow::{Context, Result};
use argh::FromArgs;
use rusqlite::{Connection, OpenFlags};

use crate::appcontext::AppContext;
use crate::config::VERSION;
use crate::log;
use crate::messages::{Annotation, AnnotationList, Messages};
use crate::utils::send_msg;

/// List aggregated bookmarks.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-bookmarks")]
pub struct ListBookmarks {}

pub fn run(context: &mut AppContext, args: &ListBookmarks) -> Result<()> {
  log!("{} {:?}", VERSION, args)?;

  let annotations = Connection::open_with_flags(&context.config.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
    .context(format!(
      "Failed to connect to the database <i>{}</i>",
      context.config.sqlite_path
    ))?
    .prepare(
      "SELECT
        Title,
        Attribution,
        VolumeID,
        COUNT(DISTINCT BookmarkID),
        MAX(Bookmark.DateModified)
      FROM Bookmark
      LEFT JOIN content
        ON content.ContentId = VolumeID AND BookTitle is null
      WHERE Hidden = 'false'
      AND bookmark.Text != ''
      GROUP BY VolumeID
      ORDER BY Bookmark.DateModified DESC;",
    )
    .context("Failed to prepare list bookmarks query")?
    .query_map([], |row| {
      Ok(Annotation {
        title: row.get::<_, String>(0)?,
        attribution: row.get::<_, String>(1)?,
        volume_id: row.get::<_, String>(2)?,
        count: row.get::<_, u32>(3)?,
        last_modified: row.get::<_, String>(4)?,
      })
    })
    .context("Failed to run list bookmarks query")?
    .collect::<Result<Vec<_>, _>>()
    .context("Failed to map list bookmarks query result")?;

  if annotations.is_empty() {
    return Ok(());
  }

  send_msg(&Messages::AnnotationList(AnnotationList { annotations }))?;

  Ok(())
}
