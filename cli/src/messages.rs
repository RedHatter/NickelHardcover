use jiff::Timestamp;
use serde::{Deserialize, Serialize};

#[derive(Serialize)]
pub struct Log {
  pub message: String,
}

#[derive(Serialize)]
pub struct Error {
  pub error_code: String,
  pub message: String,
}

#[derive(Serialize)]
pub struct User {
  pub id: i64,
  pub username: Option<String>,
  pub account_privacy_setting_id: i64,
  pub account_privacy_setting: String,
}

#[derive(Serialize)]
pub struct UserBook {
  pub user_book_id: i64,
  pub status_id: i64,
  pub rating: Option<f64>,
  pub review_has_spoilers: bool,
  pub review_raw: Option<String>,
  pub reviewed_at: Option<String>,
  pub sponsored_review: bool,
}

#[derive(Serialize)]
pub struct Annotation {
  pub title: String,
  pub attribution: String,
  pub volume_id: String,
  pub count: u32,
  pub last_modified: String,
}

#[derive(Serialize)]
pub struct AnnotationList {
  pub annotations: Vec<Annotation>,
}

#[derive(Serialize)]
pub struct Edition {
  pub asin: Option<String>,
  pub contributions: Vec<String>,
  pub country: Option<String>,
  pub edition_format: Option<String>,
  pub edition_information: Option<String>,
  pub id: i64,
  pub image: Option<String>,
  pub isbn_10: Option<String>,
  pub isbn_13: Option<String>,
  pub language: Option<String>,
  pub pages: Option<i64>,
  pub publisher: Option<String>,
  pub reading_format: String,
  pub release_date: Option<String>,
  pub score: i64,
  pub title: Option<String>,
  pub users_count: i64,
}

#[derive(Serialize)]
pub struct EditionList {
  pub languages: Vec<String>,
  pub editions: Vec<Edition>,
}

/// Union of all metadata fields we make use of
#[derive(Serialize)]
pub struct Metadata {
  pub list_name: Option<String>,
  pub progress: Option<u64>,
  pub progress_was: Option<u64>,
  pub prompt: Option<String>,
  pub rating: Option<f64>,
  pub review: Option<String>,
}

#[derive(Serialize)]
pub struct Journal {
  pub id: i64,
  pub event: Option<String>,
  pub entry: Option<String>,
  pub action_at: Timestamp,
  pub metadata: Metadata,
}

#[derive(Serialize)]
pub struct JournalList {
  pub reading_journals: Vec<Journal>,
}

#[derive(Serialize)]
pub struct Series {
  pub name: Option<String>,
  pub position: Option<u64>,
  pub primary_books_count: Option<u64>,
}

#[derive(Serialize)]
pub struct SearchResult {
  pub authors: Vec<String>,
  pub id: Option<String>,
  pub image: Option<String>,
  pub rating: Option<u64>,
  pub release_year: Option<i64>,
  pub series: Option<Series>,
  pub title: Option<String>,
  pub users_count: Option<u64>,
}

#[derive(Serialize)]
pub struct SearchPages {
  pub results: Vec<SearchResult>,
  pub page: u64,
  pub total: u64,
}

#[derive(Serialize, Deserialize)]
pub struct OAuthDevice {
  pub device_code: String,
  pub user_code: String,
  pub verification_uri: String,
  pub verification_uri_complete: String,
  pub expires_in: u64,
  pub interval: u64,
}

#[derive(Serialize)]
#[serde(tag = "kind")]
pub enum Messages {
  Log(Log),
  Error(Error),
  User(User),
  UserBook(UserBook),
  AnnotationList(AnnotationList),
  EditionList(EditionList),
  JournalList(JournalList),
  SearchPages(SearchPages),
  OAuthDevice(OAuthDevice),
}
