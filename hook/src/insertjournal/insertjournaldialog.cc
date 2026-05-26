#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../cli.h"
#include "../settings.h"
#include "../synccontroller.h"
#include "insertjournaldialog.h"

void InsertJournalDialog::show() { new InsertJournalDialog(); }

InsertJournalDialog::InsertJournalDialog() : Dialog("Add New Journal Entry") {
  QVBoxLayout *layout = new QVBoxLayout(this);

  privacy = new ButtonGroup({{"Public", "public"}, {"Follows", "follows"}, {"Private", "private"}},
                            Settings::getInstance()->getJournalPrivacy(), "Privacy");
  layout->addWidget(privacy, 0, Qt::AlignLeft);

  TouchTextEdit *touchText = construct_TouchTextEdit(this);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Write a new note...");
  layout->addWidget(touchText);

  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();
  buildKeyboardFrame(textEdit, "Submit");
  showKeyboard();
}

void InsertJournalDialog::commit() {
  nh_log("InsertJournalDialog::commit()");

  QTextEdit *textEdit = findChild<QTextEdit *>();

  CLI *cli = CLI::insertJournal(textEdit->toPlainText(), SyncController::getInstance()->getReadProgress(),
                                privacy->value().toString());
  QObject::connect(cli, &CLI::success, dialog, &QDialog::deleteLater);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}
