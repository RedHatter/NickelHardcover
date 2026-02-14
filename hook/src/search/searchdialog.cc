#include <QApplication>
#include <QDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QScreen>
#include <QSignalMapper>
#include <QTimer>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../files.h"
#include "../synccontroller.h"
#include "bookrow.h"
#include "searchdialog.h"

void SearchDialogContent::showSearchDialog(QString query) {
  SearchDialogContent *content = new SearchDialogContent(query);

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    content->networkConnected();
  } else {
    WirelessWorkflowManager__connectWireless(wfm, false, false);
    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), content, SLOT(networkConnected()));
  }
}

void SearchDialogContent::networkConnected() {
  N3Dialog *dlg = N3DialogFactory__getDialog(this, true);
  N3Dialog__disableCloseButton(dlg);
  N3Dialog__enableBackButton(dlg, true);
  N3Dialog__setTitle(dlg, "Search");

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dlg->setFixedSize(screenGeometry.width(), screenGeometry.height());

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, dlg, [dlg](QString name) {
    if (name != "ReadingView") {
      dlg->deleteLater();
    }
  });
  QObject::connect(dlg, SIGNAL(backTapped()), dlg, SLOT(deleteLater()));
  QObject::connect(this, &SearchDialogContent::close, dlg, &QDialog::deleteLater);

  setKeyboardFrame(N3Dialog__keyboardFrame(dlg));

  dlg->show();
  commit();
}

SearchDialogContent::SearchDialogContent(QString query, QWidget *parent) : QWidget(parent) {
  QVBoxLayout *contentLayout = new QVBoxLayout(this);

  lineEdit = reinterpret_cast<TouchLineEdit *>(calloc(1, 128));
  TouchLineEdit__constructor(lineEdit, nullptr);
  lineEdit->setText(query);
  contentLayout->addWidget(lineEdit);

  results = new QVBoxLayout();
  results->setContentsMargins(0, 0, 0, 0);
  results->setSpacing(0);
  results->addStretch(1);

  contentLayout->addLayout(results, 1);

  footer = reinterpret_cast<PagingFooter *>(calloc(1, 128));
  PagingFooter__constructor(footer, this);
  QObject::connect(footer, SIGNAL(goToPage(int)), this, SLOT(search(int)));
  contentLayout->addWidget(footer);
  footer->hide();
}

void SearchDialogContent::setKeyboardFrame(KeyboardFrame *keyboardFrame) {
  keyboard = keyboardFrame;

  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__constructor(receiver, lineEdit, false);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, QLocale::English);
  SearchKeyboardController__setReceiver(ctl, receiver, false);

  QObject::connect(lineEdit, SIGNAL(tapped()), keyboard, SLOT(show()));
  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(commit()));
}

void SearchDialogContent::commit() {
  keyboard->hide();
  QTimer::singleShot(100, this, [this] { search(1); });
}

void SearchDialogContent::clear() {
  while (QLayoutItem *item = results->takeAt(0)) {
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }

    delete item;
  }
}

void SearchDialogContent::search(int page) {
  QString query = lineEdit->text();

  int limit = results->geometry().height() / 270;

  clear();
  footer->hide();

  QLabel *loadingLabel = new QLabel("Searching. Please wait...");
  loadingLabel->setAlignment(Qt::AlignCenter);
  loadingLabel->setStyleSheet("QLabel { font-size: 8pt; }");
  results->addWidget(loadingLabel, 1);

  CLI* cli = new CLI(this);
  cli->search(query, limit, page);
  QObject::connect(cli, &CLI::response, this, &SearchDialogContent::response);
  QObject::connect(cli, &CLI::failure, this, &SearchDialogContent::close);
}

void SearchDialogContent::response(QJsonObject doc) {
  clear();

  double total = doc.value("total").toDouble(1);

  if (total > 1) {
    footer->show();
    PagingFooter__setTotalPages(footer, total);
    PagingFooter__setCurrentPage(footer, doc.value("page").toDouble(1));
  }

  QJsonArray resultsArray = doc.value("results").toArray();

  QSignalMapper *signalMapper = new QSignalMapper(this);
  QObject::connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(tapped(QString)));

  int length = resultsArray.size();

  if (length < 1) {
    QLabel *label = new QLabel("No results.");
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { font-size: 8pt; }");
    results->addWidget(label, 1);
    return;
  }

  for (int i = 0; i < length; i++) {
    QJsonObject obj = resultsArray.at(i).toObject();
    SettingContainer *row = buildBookRow(obj, i < length - 1);
    results->addWidget(row);
    QObject::connect(row, SIGNAL(tapped()), signalMapper, SLOT(map()));
    signalMapper->setMapping(row, obj.value("id").toString());
  }

  results->addStretch(1);
}

void SearchDialogContent::tapped(QString id) {
  nh_log("SearchDialogContent::tapped(%s)", qPrintable(id));
  SyncController::getInstance()->setLinkedBook(id);
  close();
}
