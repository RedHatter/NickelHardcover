#include <QDateTime>
#include <QLabel>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../files.h"
#include "../widgets/elidedlabel.h"
#include "journalentry.h"

JournalEntry::JournalEntry(QJsonObject doc, QWidget *parent) : QFrame(parent) {
  setStyleSheet("\
    JournalEntry { \
      border-bottom: 1px solid #111111; \
      padding: 16px; \
    } \
    QLabel#actionAt { font-size: 8pt; } \
    QLabel#label { font-size: 10pt; } \
  ");

  QVBoxLayout *layout = new QVBoxLayout(this);

  QString event = doc.value("event").toString();

  QHBoxLayout *line = new QHBoxLayout();
  layout->addLayout(line);
  QLabel *icon = new QLabel(this);
  line->addWidget(icon);
  QLabel *label = new QLabel(this);
  label->setObjectName("label");
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
    label->setText(QString("Rated %1").arg(metadata.value("rating").toString()));
  } else if (event == "note") {
    icon->setPixmap(QPixmap(Files::note));
    label->setText("Saved a Note");

    ElidedLabel *text = new ElidedLabel(doc.value("entry").toString(), 5, this);
    layout->addWidget(text);
  } else if (event == "quote") {
    icon->setPixmap(QPixmap(Files::quote));
    label->setText("Saved a Quote");

    ElidedLabel *text = new ElidedLabel(doc.value("entry").toString(), 5, this);
    layout->addWidget(text);
  } else if (event == "reviewed") {
    icon->setPixmap(QPixmap(Files::reviewed));
    label->setText("Reviewed");

    QJsonObject metadata = doc.value("metadata").toObject();
    ElidedLabel *text = new ElidedLabel(metadata.value("review").toString(), 5, this);
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
