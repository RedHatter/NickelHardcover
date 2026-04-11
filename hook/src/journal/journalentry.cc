#include <QDateTime>
#include <QLabel>
#include <QVBoxLayout>

#include "../files.h"
#include "../widgets/elidedlabel.h"
#include "journalentry.h"

JournalEntry::JournalEntry(QJsonObject doc, QWidget *parent) : QFrame(parent) {
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

    [qApp_deviceIsTrilogy=true] QLabel#actionAt {
      font-size: 14px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#actionAt {
      font-size: 18px;
    }
    [qApp_deviceIsDragon=true] QLabel#actionAt {
      font-size: 21px;
    }
    [qApp_deviceIsAlyssum=true] QLabel#actionAt,
    [qApp_deviceIsNova=true] QLabel#actionAt,
    [qApp_deviceIsStorm=true] QLabel#actionAt {
      font-size: 25px;
    }
    [qApp_deviceIsDaylight=true] QLabel#actionAt {
      font-size: 28px;
    }

    JournalEntry {
      border-top: 1px solid #666666;
    }

    JournalEntry#first {
      border-top-width: 0;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  QString event = doc.value("event").toString();

  QHBoxLayout *line = new QHBoxLayout();
  layout->addLayout(line);
  QLabel *icon = new QLabel(this);
  line->addWidget(icon);
  QLabel *label = new QLabel(this);
  label->setObjectName("small");
  line->addWidget(label, 1);

  if (event == "status_want_to_read") {
    icon->setPixmap(QPixmap(Files::status_want_to_read));
    label->setText("Saved as Want To Read");
  } else if (event == "status_read") {
    icon->setPixmap(QPixmap(Files::status_read));
    label->setText("Marked as Read");
  } else if (event == "user_book_read_started") {
    icon->setPixmap(QPixmap(Files::user_book_read_started));
    label->setText("Started reading");
  } else if (event == "user_book_read_finished") {
    icon->setPixmap(QPixmap(Files::user_book_read_finished));
    label->setText("Finished reading");
  } else if (event == "status_stopped") {
    icon->setPixmap(QPixmap(Files::status_stopped));
    label->setText("Stopped reading");
  } else if (event == "progress_updated") {
    icon->setPixmap(QPixmap(Files::progress_updated));
    QJsonObject metadata = doc.value("metadata").toObject();
    label->setText(QString("Updated progress from %1% → %2%")
                       .arg(metadata.value("progress_was").toInt())
                       .arg(metadata.value("progress").toInt()));
  } else if (event == "rated") {
    icon->setPixmap(QPixmap(Files::rated));
    QJsonObject metadata = doc.value("metadata").toObject();
    label->setText("Rated " + metadata.value("rating").toString());
  } else if (event == "list_book") {
    icon->setPixmap(QPixmap(Files::list_book));
    QJsonObject metadata = doc.value("metadata").toObject();
    label->setText(QString("Added to list <i>%1</i>").arg(metadata.value("list_name").toString()));
  } else if (event == "prompt_book") {
    icon->setPixmap(QPixmap(Files::prompt_book));
    QJsonObject metadata = doc.value("metadata").toObject();
    label->setText(QString("Answered prompt <i>%1</i>").arg(metadata.value("prompt").toString()));
  } else if (event == "note") {
    icon->setPixmap(QPixmap(Files::note));
    label->setText("Saved a Note");

    ElidedLabel *text = new ElidedLabel(doc.value("entry").toString(), 5, this);
    text->setObjectName("regular");
    layout->addWidget(text);
  } else if (event == "quote") {
    icon->setPixmap(QPixmap(Files::quote));
    label->setText("Saved a Quote");

    ElidedLabel *text = new ElidedLabel(doc.value("entry").toString(), 5, this);
    text->setObjectName("regular");
    layout->addWidget(text);
  } else if (event == "reviewed") {
    icon->setPixmap(QPixmap(Files::reviewed));
    label->setText("Reviewed");

    QJsonObject metadata = doc.value("metadata").toObject();
    ElidedLabel *text = new ElidedLabel(metadata.value("review").toString(), 5, this);
    text->setObjectName("regular");
    layout->addWidget(text);
  } else {
    label->setText("Unknown journal type " + event);
  }

  QString actionAt = QDateTime::fromString(doc.value("action_at").toString(), Qt::ISODate)
                         .toLocalTime()
                         .toString(Qt::SystemLocaleShortDate);
  QLabel *actionAtLabel = new QLabel(actionAt, this);
  actionAtLabel->setObjectName("actionAt");
  layout->addWidget(actionAtLabel, 0, Qt::AlignRight);
  actionAtLabel->lower();
}
