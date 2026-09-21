#include <QDateTime>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../files.h"
#include "../settings.h"
#include "../widgets/elidedlabel.h"
#include "journalentry.h"

JournalEntry::JournalEntry(const Journal &journal, QWidget *parent) : QFrame(parent) {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] JournalEntry {
      padding: 12px 0;
    }
    [qApp_deviceIsPhoenix=true] JournalEntry {
      padding: 16px 0;
    }
    [qApp_deviceIsDragon=true] JournalEntry {
      padding: 22px 0;
    }
    [qApp_deviceIsStorm=true] JournalEntry {
      padding: 25px 0;
    }
    [qApp_deviceIsDaylight=true] JournalEntry {
      padding: 28px 0;
    }

    JournalEntry {
      border-top: 1px solid #666666;
    }

    JournalEntry[noBorder=true] {
      border-top-width: 0;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout *line = new QHBoxLayout();
  layout->addLayout(line);
  QLabel *icon = new QLabel(this);
  line->addWidget(icon);
  Label *label = new Label(Label::Small, "");
  line->addWidget(label, 1);

  if (!journal.event) {
    return;
  }

  QString event = *journal.event;

  if (event == "status_want_to_read") {
    icon->setPixmap(QPixmap(Files::status_want_to_read));
    label->setText("Saved as Want To Read");
  } else if (event == "status_read") {
    icon->setPixmap(QPixmap(Files::status_read));
    label->setText("Marked as Read");
  } else if (event == "user_book_read_started" || event == "status_currently_reading") {
    icon->setPixmap(QPixmap(Files::user_book_read_started));
    label->setText("Started reading");
  } else if (event == "user_book_read_finished") {
    icon->setPixmap(QPixmap(Files::user_book_read_finished));
    label->setText("Finished reading");
  } else if (event == "status_stopped") {
    icon->setPixmap(QPixmap(Files::status_stopped));
    label->setText("Stopped reading");
  } else if (event == "status_paused") {
    icon->setPixmap(QPixmap(Files::status_paused));
    label->setText("Paused reading");
  } else if (event == "progress_updated") {
    icon->setPixmap(QPixmap(Files::progress_updated));
    Metadata metadata = journal.metadata;
    label->setText(QString("Updated progress from %1% → %2%")
                       .arg(metadata.progress_was.value_or_default())
                       .arg(metadata.progress.value_or_default()));
  } else if (event == "rated") {
    icon->setPixmap(QPixmap(Files::rated));
    Metadata metadata = journal.metadata;
    label->setText(QString("Rated %1").arg(metadata.rating.value_or_default(), 0, 'f', 1));
  } else if (event == "list_book") {
    icon->setPixmap(QPixmap(Files::list_book));
    label->setText(QString("Added to list <i>%1</i>").arg(journal.metadata.list_name.value_or("Unknown")));
  } else if (event == "prompt_book") {
    icon->setPixmap(QPixmap(Files::prompt_book));
    label->setText(QString("Answered prompt <i>%1</i>").arg(journal.metadata.prompt.value_or("Unknown")));
  } else if (event == "note") {
    icon->setPixmap(QPixmap(Files::note));
    label->setText("Saved a Note");

    layout->addWidget(new ElidedLabel(Label::Medium, journal.entry.value_or("Content missing"), 5, this));
  } else if (event == "quote") {
    icon->setPixmap(QPixmap(Files::quote));
    label->setText("Saved a Quote");

    layout->addWidget(new ElidedLabel(Label::Medium, journal.entry.value_or("Content missing"), 5, this));
  } else if (event == "reviewed") {
    icon->setPixmap(QPixmap(Files::reviewed));
    label->setText("Reviewed");

    layout->addWidget(new ElidedLabel(Label::Medium, journal.metadata.review.value_or("Content missing"), 5, this));
  } else {
    label->setText("Unknown journal type " + event);
  }

  QString fmt = locale().dateTimeFormat(QLocale::ShortFormat);

  if (Settings::getInstance()->is24HourClock()) {
    fmt = fmt.remove("ap", Qt::CaseInsensitive).remove("a", Qt::CaseInsensitive).replace('h', 'H');
  } else {
    fmt = fmt.replace('H', 'h');

    if (!fmt.contains('a', Qt::CaseInsensitive)) {
      fmt += " AP";
    }
  }

  QString actionAt = QDateTime::fromString(journal.action_at, Qt::ISODate).toLocalTime().toString(fmt);
  Label *actionAtLabel = new Label(Label::ExtraSmall, actionAt);
  layout->addWidget(actionAtLabel, 0, Qt::AlignRight);
  actionAtLabel->lower();
}
