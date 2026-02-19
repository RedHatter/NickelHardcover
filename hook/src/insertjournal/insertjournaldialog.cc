#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../cli.h"
#include "../synccontroller.h"
#include "qglobal.h"
#include "insertjournaldialog.h"

void InsertJournalContent::showInsertJournalDialog() {
  InsertJournalContent *content = new InsertJournalContent();

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    content->networkConnected();
  } else {
    WirelessWorkflowManager__connectWireless(wfm, false, false);
    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), content, SLOT(buildContent()));
  }
}

InsertJournalContent::InsertJournalContent(QWidget *parent) : QWidget(parent) {}

void InsertJournalContent::networkConnected() {
  QVBoxLayout *layout = new QVBoxLayout(this);

  TouchTextEdit *touchText = reinterpret_cast<TouchTextEdit *>(calloc(1, 128));
  TouchTextEdit__constructor(touchText, this);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Write a new note...");
  layout->addWidget(touchText);

  N3Dialog *dialog = N3DialogFactory__getDialog(this, true);
  N3Dialog__setTitle(dialog, "Add New Journal Entry");

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dialog->setFixedSize(screenGeometry.width(), screenGeometry.height());

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, dialog, [dialog](QString name) {
    if (name != "ReadingView") {
      dialog->deleteLater();
    }
  });
  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));
  QObject::connect(this, &InsertJournalContent::close, dialog, &QDialog::deleteLater);

  KeyboardFrame *keyboard = N3Dialog__keyboardFrame(dialog);
  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();

  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__TextEdit_constructor(receiver, textEdit, false);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, QLocale::English);
  SearchKeyboardController__setReceiver(ctl, receiver, false);
  SearchKeyboardController__setGoText(ctl, "Add note");

  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(commit()));

  keyboard->show();
  dialog->show();
}

void InsertJournalContent::commit() {
  nh_log("InsertJournalContent::commit()");

  QTextEdit *textEdit = findChild<QTextEdit *>();

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();

  if (name != "ReadingView") {
    nh_log("Error: attempted to call CLI while current view is %s", qPrintable(name));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Can only insert new journal entry while a book is open");
    return;
  }

  int percentage = ReadingView__getCalculatedReadProgress(cv);
  if (percentage == 0) {
    percentage = 1;
  }

  CLI *cli = new CLI(this);
  cli->insertJournal(textEdit->toPlainText(), percentage);
  QObject::connect(cli, &CLI::success, this, &InsertJournalContent::close);
  QObject::connect(cli, &CLI::failure, this, &InsertJournalContent::close);
}
