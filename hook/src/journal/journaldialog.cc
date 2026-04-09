#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../insertjournal/insertjournaldialog.h"
#include "journaldialog.h"
#include "journalentry.h"

void JournalDialog::show() { new JournalDialog(); }

JournalDialog::JournalDialog() : Dialog("Reading Journal") {
  N3Dialog__enableFullViewMode(dialog);

  QVBoxLayout *layout = new QVBoxLayout(this);

  N3ButtonLabel *button = reinterpret_cast<N3ButtonLabel *>(calloc(1, 512));
  N3ButtonLabel__constructor(button, this);
  N3ButtonLabel__setPrimaryButton(button, true);
  button->setText("+ New entry");
  layout->addWidget(button, 0, Qt::AlignRight);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(newEntry()));

  pages = new PagedStack(this);
  layout->addWidget(pages, 1);
  QObject::connect(pages, &PagedStack::requestPage, this, &JournalDialog::requestPage);

  pages->next();
}

void JournalDialog::newEntry() {
  nh_log("JournalDialog::newEntry()");

  InsertJournalDialog::show();
  dialog->deleteLater();
}

void JournalDialog::requestPage(int index) {
  nh_log("JournalDialog::requestPage(%d)", index);

  CLI *cli = CLI::listJournal(15, offset);
  QObject::connect(cli, &CLI::response, this, &JournalDialog::response);
}

void JournalDialog::response(QJsonObject doc) {
  QWidget *box = new QWidget(this);
  QVBoxLayout *rows = new QVBoxLayout(box);
  rows->setSpacing(0);

  QJsonArray results = doc.value("reading_journals").toArray();
  int length = results.size();
  int availableHeight = pages->getAvailableHeight();

  int i = 0;
  for (; i < length; i++) {
    QJsonObject obj = results.at(i).toObject();
    JournalEntry *entry = new JournalEntry(obj, this);

    availableHeight -= entry->sizeHint().height();
    if (availableHeight < 0)
      break;

    rows->addWidget(entry);
  }

  offset += i;

  rows->addStretch(1);

  if (i == length) {
    pages->total = pages->countPages();
  }

  pages->addPage(box);
};
