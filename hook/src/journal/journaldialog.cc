#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>
#include <QTimer>

#include <NickelHook.h>
#include <sys/types.h>

#include "../cli.h"
#include "../files.h"
#include "../insertjournal/insertjournaldialog.h"
#include "../synccontroller.h"
#include "../utils.h"
#include "journaldialog.h"
#include "journalentry.h"

void JournalDialogContent::showJournalDialog() {
  JournalDialogContent *content = new JournalDialogContent();

  N3Dialog *dialog = N3DialogFactory__getDialog(content, true);
  N3Dialog__setTitle(dialog, "Reading Journal");

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dialog->setFixedSize(screenGeometry.width(), screenGeometry.height());

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, dialog, [dialog](QString name) {
    if (name != "ReadingView") {
      dialog->deleteLater();
    }
  });

  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));
  QObject::connect(content, &JournalDialogContent::close, dialog, &QDialog::deleteLater);

  dialog->show();

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    content->networkConnected();
  } else {
    WirelessWorkflowManager__connectWireless(wfm, false, false);
    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), content, SLOT(networkConnected()));
  }
}

JournalDialogContent::JournalDialogContent(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  N3ButtonLabel *button = reinterpret_cast<N3ButtonLabel *>(calloc(1, 512));
  N3ButtonLabel__constructor(button, this);
  N3ButtonLabel__setPrimaryButton(button, true);
  button->setText("+ New entry");
  layout->addWidget(button, 0, Qt::AlignRight);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(newEntry()));

  rows = new QVBoxLayout();
  rows->setSpacing(0);
  rows->addStretch(1);
  layout->addLayout(rows, 1);

  footer = reinterpret_cast<PagingFooter *>(calloc(1, 128));
  PagingFooter__constructor(footer, this);
  layout->addWidget(footer);
  QObject::connect(footer, SIGNAL(goToPage(int)), this, SLOT(goToPage(int)));
}

void JournalDialogContent::networkConnected() { goToPage(1); }

void JournalDialogContent::newEntry() {
  nh_log("JournalDialogContent::newEntry()");

  InsertJournalContent::showInsertJournalDialog();
  close();
}

void JournalDialogContent::goToPage(int page) {
  nh_log("JournalDialogContent::goToPage(%d)", page);

  if (page > currentPage + 1) {
    currentPage++;
  } else {
    currentPage = page;
  }

  PagingFooter__setTotalPages(footer, currentPage + 1);
  PagingFooter__setCurrentPage(footer, currentPage);
  footer->findChild<QLabel *>("pages")->setText(QString("Page %1").arg(currentPage));

  int offset = 0;
  for (int i = 0; i < currentPage && i < pages->size(); i++) {
    offset += pages->at(i);
  }

  CLI *cli = new CLI(this);
  cli->listJournal(15, offset);
  QObject::connect(cli, &CLI::response, this, &JournalDialogContent::buildContent);
  QObject::connect(cli, &CLI::failure, this, &JournalDialogContent::close);
}

void JournalDialogContent::buildContent(QJsonObject doc) {
  int availableHeight = rows->geometry().height();

  while (QLayoutItem *item = rows->takeAt(0)) {
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }

    delete item;
  }

  QJsonArray results = doc.value("reading_journals").toArray();
  int length = results.size();

  int i = 0;
  for (; i < length; i++) {
    QJsonObject obj = results.at(i).toObject();
    JournalEntry *entry = new JournalEntry(obj, this);

    availableHeight -= entry->sizeHint().height();
    if (availableHeight < 0)
      break;

    rows->addWidget(entry);
  }

  rows->addStretch(1);

  if (pages->size() <= currentPage) {
    pages->append(i);
  }

  if (i == length) {
    PagingFooter__setTotalPages(footer, currentPage);
    return;
  }
};
