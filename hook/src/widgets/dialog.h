#pragma once

#include <QWidget>

#include "../nickelhardcover.h"

class Dialog : public QWidget {
  Q_OBJECT

public Q_SLOTS:
  void networkConnected();
  void currentViewChanged(QString name);

  virtual void commit() {}

private:
  QString title;

  KeyboardFrame *buildKeyboardFrame(KeyboardReceiver *receiver, QString goText);

protected:
  Dialog(QString title, QWidget *parent = nullptr);

  N3Dialog *dialog;

  void connectNetwork();

  KeyboardFrame *buildKeyboardFrame(TouchLineEdit *lineEdit, QString goText);
  KeyboardFrame *buildKeyboardFrame(QTextEdit *textEdit, QString goText);

  virtual void build() = 0;
};
