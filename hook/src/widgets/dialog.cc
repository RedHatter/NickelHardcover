#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../synccontroller.h"
#include "dialog.h"

Dialog::Dialog(QString title) : QWidget() {
  dialog = N3DialogFactory__getDialog(this, true);
  N3Dialog__setTitle(dialog, title);
  dialog->setStyleSheet(dialog->styleSheet().append(R"(
    QLabel#empty {
      qproperty-alignment: AlignCenter;
      qproperty-text: "No results.";
    }

    QLabel#loading {
      qproperty-alignment: AlignCenter;
      qproperty-text: "Loading. Please wait...";
    }

    [qApp_deviceIsTrilogy=true] QLabel#empty,
    [qApp_deviceIsTrilogy=true] QLabel#loading,
    [qApp_deviceIsTrilogy=true] #regular {
      font-size: 17px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#empty,
    [qApp_deviceIsPhoenix=true] QLabel#loading,
    [qApp_deviceIsPhoenix=true] #regular {
      font-size: 22px;
    }
    [qApp_deviceIsDragon=true] QLabel#empty,
    [qApp_deviceIsDragon=true] QLabel#loading,
    [qApp_deviceIsDragon=true] #regular {
      font-size: 26px;
    }
    [qApp_deviceIsAlyssum=true] QLabel#empty,
    [qApp_deviceIsAlyssum=true] QLabel#loading,
    [qApp_deviceIsAlyssum=true] #regular,
    [qApp_deviceIsNova=true] QLabel#empty,
    [qApp_deviceIsNova=true] QLabel#loading,
    [qApp_deviceIsNova=true] #regular,
    [qApp_deviceIsStorm=true] QLabel#empty,
    [qApp_deviceIsStorm=true] QLabel#loading,
    [qApp_deviceIsStorm=true] #regular {
      font-size: 30px;
    }
    [qApp_deviceIsDaylight=true] QLabel#empty,
    [qApp_deviceIsDaylight=true] QLabel#loading,
    [qApp_deviceIsDaylight=true] #regular {
      font-size: 34px;
    }

    #metaData {
      font-family: Avenir, sans-serif;
      text-transform: uppercase;
    }

    [qApp_deviceIsTrilogy=true] #metaData {
      font-size: 14px;
    }
    [qApp_deviceIsPhoenix=true] #metaData {
      font-size: 17px;
    }
    [qApp_deviceIsDragon=true] #metaData {
      font-size: 25px;
    }
    [qApp_deviceIsStorm=true] #metaData {
      font-size: 29px;
    }
    [qApp_deviceIsDaylight=true] #metaData {
      font-size: 32px;
    }
  )"));

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dialog->setFixedSize(screenGeometry.width(), screenGeometry.height());

  MainWindowController *mwc = MainWindowController__sharedInstance();
  MainWindowController__pushView(mwc, dialog);

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, this,
                   &Dialog::currentViewChanged);
  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));

  dialog->show();
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
