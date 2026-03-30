#pragma once

#include <QWidget>

#include "../nickelhardcover.h"

class Dialog : public QWidget {
  Q_OBJECT

public Q_SLOTS:
  void currentViewChanged(QString name);
  void showKeyboard();
  void hideKeyboard();

  virtual void commit() {}

private:
  KeyboardFrame *buildKeyboardFrame(KeyboardReceiver *receiver, QString goText);

protected:
  Dialog(QString title);

  N3Dialog *dialog = nullptr;

  KeyboardFrame *buildKeyboardFrame(TouchLineEdit *lineEdit, QString goText);
  KeyboardFrame *buildKeyboardFrame(QTextEdit *textEdit, QString goText);
};
