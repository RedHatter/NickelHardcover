#include <QJsonArray>
#include <QVBoxLayout>
#include <QVector>

#include <NickelHook.h>

#include "../cli.h"
#include "../insertjournal/insertjournaldialog.h"
#include "journaldialog.h"
#include "journalentry.h"

JournalDialog::JournalDialog() : Dialog("Reading Journal") {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] QStackedWidget {
      margin: 0 12px;
    }
    [qApp_deviceIsPhoenix=true] QStackedWidget {
      margin: 0 16px;
    }
    [qApp_deviceIsDragon=true] QStackedWidget {
      margin: 0 22px;
    }
    [qApp_deviceIsStorm=true] QStackedWidget {
      margin: 0 25px;
    }
    [qApp_deviceIsDaylight=true] QStackedWidget {
      margin: 0 28px;
    }
  )");

  N3Dialog__enableFullViewMode(dialog);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setProperty("primaryButton", true);
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
  QObject::connect(cli, &CLI::failure, this, &JournalDialog::closeDialog);
}

void JournalDialog::response(const Messages &message) {
  if (!message.isJournalList()) {
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Unexpected CLI response for <i>listJournal</i>");
    return;
  }

  QVector<Journal> reading_journals = message.journal_list->reading_journals;

  QWidget *box = new QWidget(this);
  QVBoxLayout *rows = new QVBoxLayout(box);
  rows->setContentsMargins(0, 0, 0, 0);
  rows->setSpacing(0);

  int length = reading_journals.size();
  int availableHeight = pages->getAvailableHeight();

  int i = 0;
  for (; i < length; i++) {
    Journal obj = reading_journals.at(i);
    JournalEntry *entry = new JournalEntry(obj, pages);
    if (i == 0) {
      entry->setProperty("noBorder", true);
    }

    availableHeight -= entry->sizeHint().height();
    if (availableHeight < 0)
      break;

    rows->addWidget(entry);
  }

  offset += i;

  rows->addStretch(1);
  pages->addPage(box);

  if (i == length) {
    pages->setTotal(pages->countPages());
  }
}
