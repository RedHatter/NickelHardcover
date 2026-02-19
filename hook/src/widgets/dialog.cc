#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../synccontroller.h"
#include "dialog.h"

Dialog::Dialog(QString title, QWidget *parent) : QWidget(parent) { this->title = title; }

void Dialog::connectNetwork() {
  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    networkConnected();
  } else {
    WirelessWorkflowManager__connectWireless(wfm, false, false);
    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()));
  }
}

void Dialog::networkConnected() {
  dialog = N3DialogFactory__getDialog(this, true);
  N3Dialog__setTitle(dialog, title);

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dialog->setFixedSize(screenGeometry.width(), screenGeometry.height());

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, this,
                   &Dialog::currentViewChanged);
  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));

  build();
}

void Dialog::currentViewChanged(QString name) {
  if (name != "ReadingView") {
    dialog->deleteLater();
  }
}

void Dialog::showKeyboard() { N3Dialog__showKeyboard(dialog); }

void Dialog::hideKeyboard() { N3Dialog__hideKeyboard(dialog); }

KeyboardFrame *Dialog::buildKeyboardFrame(TouchLineEdit *lineEdit, QString goText) {
  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__constructor(receiver, lineEdit, false);

  KeyboardFrame *keyboard = buildKeyboardFrame(receiver, goText);
  QObject::connect(lineEdit, SIGNAL(tapped()), this, SLOT(showKeyboard()));

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrame(QTextEdit *textEdit, QString goText) {
  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__TextEdit_constructor(receiver, textEdit, false);

  KeyboardFrame *keyboard = buildKeyboardFrame(receiver, goText);

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrame(KeyboardReceiver *receiver, QString goText) {
  KeyboardFrame *keyboard = N3Dialog__keyboardFrame(dialog);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, QLocale::English);
  SearchKeyboardController__setReceiver(ctl, receiver, false);
  SearchKeyboardController__setGoText(ctl, goText);

  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(hideKeyboard()));
  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(commit()));

  return keyboard;
}
