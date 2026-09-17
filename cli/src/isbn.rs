fn isbn_13_check_digit(digits: &[u32]) -> u32 {
  let mut sum = 0;
  for i in 0..6 {
    sum += digits[i * 2] + 3 * digits[i * 2 + 1];
  }
  let sum_m = sum % 10;
  if sum_m == 0 { 0 } else { 10 - sum_m }
}

fn isbn_10_check_digit(digits: &[u32]) -> u32 {
  let sum = digits
    .iter()
    .enumerate()
    .take(9)
    .map(|(i, digit)| digit * (10 - i as u32))
    .sum::<u32>();
  let sum_m = sum % 11;
  if sum_m == 0 { 0 } else { 11 - sum_m }
}

fn digits_to_string(digits: &[u32]) -> String {
  digits
    .iter()
    .map(|d| {
      if *d == 10 {
        'X'
      } else {
        char::from_digit(*d, 10).unwrap()
      }
    })
    .collect()
}

fn string_to_digits(str: &str) -> Vec<u32> {
  str
    .chars()
    .filter_map(|c| if c == 'X' || c == 'x' { Some(10) } else { c.to_digit(10) })
    .collect()
}

pub fn normalize_isbn(isbn: &str) -> Option<Vec<String>> {
  let mut isbn = isbn.to_ascii_uppercase();
  isbn.retain(char::is_alphanumeric);

  if isbn.len() == 10 && isbn.starts_with('B') {
    return Some(vec![isbn]);
  }

  let digits = string_to_digits(&isbn);

  if digits.len() == 13
    && (digits[..3] == [9, 7, 8] || digits[..3] == [9, 7, 9])
    && isbn_13_check_digit(&digits) == digits[12]
  {
    if digits[..3] == [9, 7, 8] {
      let mut isbn_10 = [0; 10];
      isbn_10[..9].clone_from_slice(&digits[3..12]);
      isbn_10[9] = isbn_10_check_digit(&isbn_10);

      Some(vec![isbn, digits_to_string(&isbn_10)])
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

    Some(vec![isbn, digits_to_string(&isbn_13)])
  } else {
    None
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  #[test]
  fn normalize_isbn_valid() {
    // 978-prefixed ISBN-13 has an ISBN-10 equivalent
    assert_eq!(
      normalize_isbn("9780306406157"),
      Some(vec!["9780306406157".to_string(), "0306406152".to_string()])
    );

    // 979-prefixed ISBNs have no ISBN-10 equivalent
    assert_eq!(normalize_isbn("9791030015935"), Some(vec!["9791030015935".to_string()]));

    // ISBN-10 converts to its ISBN-13 equivalent
    assert_eq!(
      normalize_isbn("0306406152"),
      Some(vec!["0306406152".to_string(), "9780306406157".to_string()])
    );

    // ISBN-10 with an 'X' check digit
    assert_eq!(
      normalize_isbn("080442957X"),
      Some(vec!["080442957X".to_string(), "9780804429573".to_string()])
    );

    // ASIN passes through unchanged, uppercased
    assert_eq!(normalize_isbn("B0123456AB"), Some(vec!["B0123456AB".to_string()]));
    assert_eq!(normalize_isbn("b0123456ab"), Some(vec!["B0123456AB".to_string()]));
  }

  #[test]
  fn normalize_isbn_strips_separators() {
    assert_eq!(
      normalize_isbn("978-0-306-40615-7"),
      Some(vec!["9780306406157".to_string(), "0306406152".to_string()])
    );
    assert_eq!(
      normalize_isbn("0-8044-2957-x"),
      Some(vec!["080442957X".to_string(), "9780804429573".to_string()])
    );
  }

  #[test]
  fn normalize_isbn_invalid() {
    // Invalid checksums
    assert_eq!(normalize_isbn("9780306406158"), None);
    assert_eq!(normalize_isbn("0306406153"), None);

    // Wrong length
    assert_eq!(normalize_isbn("12345"), None);
    assert_eq!(normalize_isbn(""), None);
  }

  #[test]
  fn string_to_digits_basic() {
    assert_eq!(string_to_digits("0123456789X"), vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    assert_eq!(string_to_digits("0123456789x"), vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    assert_eq!(string_to_digits("12-34"), vec![1, 2, 3, 4]);
  }

  #[test]
  fn digits_to_string_roundtrip() {
    let digits = vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    let s = digits_to_string(&digits);
    assert_eq!(s, "0123456789X");
    assert_eq!(string_to_digits(&s), digits);
  }

  #[test]
  fn isbn_10_check_digit_ok() {
    assert_eq!(isbn_10_check_digit(&string_to_digits("030640615")), 2);
    assert_eq!(isbn_10_check_digit(&string_to_digits("080442957")), 10);
  }

  #[test]
  fn isbn_13_check_digit_ok() {
    assert_eq!(isbn_13_check_digit(&string_to_digits("978030640615")), 7);
    assert_eq!(isbn_13_check_digit(&string_to_digits("979103001593")), 5);
  }
}
