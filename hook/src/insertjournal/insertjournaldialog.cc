#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../cli.h"
#include "../synccontroller.h"
#include "insertjournaldialog.h"

void InsertJournalDialog::show() {
  InsertJournalDialog *dialog = new InsertJournalDialog();
  dialog->connectNetwork();
}

InsertJournalDialog::InsertJournalDialog(QWidget *parent) : Dialog("Add New Journal Entry", parent) {}

void InsertJournalDialog::build() {
  nh_log("InsertJournalDialog::build()");

  QVBoxLayout *layout = new QVBoxLayout(this);

  TouchTextEdit *touchText = reinterpret_cast<TouchTextEdit *>(calloc(1, 128));
  TouchTextEdit__constructor(touchText, this);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Write a new note...");
  layout->addWidget(touchText);

  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();
  buildKeyboardFrame(textEdit, "Submit");
  showKeyboard();

  dialog->show();
}

void InsertJournalDialog::commit() {
  nh_log("InsertJournalDialog::commit()");

  QTextEdit *textEdit = findChild<QTextEdit *>();

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();

  if (name != "ReadingView") {
    nh_log("Error: attempted to call CLI while current view is %s", qPrintable(name));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app",
                                               "Can only insert new journal entry while a book is open");
    return;
  }

  int percentage = ReadingView__getCalculatedReadProgress(cv);
  if (percentage == 0) {
    percentage = 1;
  }

  CLI *cli = new CLI(this);
  cli->insertJournal(textEdit->toPlainText(), percentage);
  QObject::connect(cli, &CLI::success, dialog, &QDialog::deleteLater);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}
