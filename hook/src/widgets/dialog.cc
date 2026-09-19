#include <QTextEdit>

#include <NickelHook.h>

#include "../synccontroller.h"
#include "dialog.h"

Dialog::Dialog(const QString &title) : QFrame() {
  dialog = N3DialogFactory__getDialog(this, true);
  N3Dialog__setTitle(dialog, title);

  MainWindowController *mwc = MainWindowController__sharedInstance();
  MainWindowController__pushView(mwc, dialog);

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, this,
                   &Dialog::currentViewChanged);
  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));

  dialog->show();
}

void Dialog::currentViewChanged(const QString &name) {
  if (name != "ReadingView") {
    dialog->deleteLater();
  }
}

KeyboardFrame *Dialog::buildKeyboardFrame(TouchLineEdit *lineEdit, const QString &goText) {
  KeyboardReceiver *receiver = construct_KeyboardReceiver(lineEdit);
  KeyboardFrame *keyboard = buildKeyboardFrame(receiver, goText);
  QObject::connect(lineEdit, SIGNAL(tapped()), this, SLOT(showKeyboard()));

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrame(QTextEdit *textEdit, const QString &goText) {
  KeyboardReceiver *receiver = construct_KeyboardReceiver(textEdit);
  KeyboardFrame *keyboard = buildKeyboardFrame(receiver, goText);

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrame(KeyboardReceiver *receiver, const QString &goText) {
  KeyboardFrame *keyboard = N3Dialog__keyboardFrame(dialog);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, locale());
  SearchKeyboardController__setReceiver(ctl, receiver, false);
  SearchKeyboardController__setGoText(ctl, goText);

  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(hideKeyboard()));
  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(commit()));

  return keyboard;
}
