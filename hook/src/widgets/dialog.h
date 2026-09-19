#pragma once

#include <QFrame>

#include "../nickelhardcover.h"

class Dialog : public QFrame {
  Q_OBJECT

public Q_SLOTS:
  void showKeyboard() { N3Dialog__showKeyboard(dialog); }
  void hideKeyboard() { N3Dialog__hideKeyboard(dialog); }

  virtual void commit() {}

private:
  void currentViewChanged(const QString &name);

  KeyboardFrame *buildKeyboardFrame(KeyboardReceiver *receiver, const QString &goText);

protected:
  Dialog(const QString &title);

  N3Dialog *dialog = nullptr;

  void closeDialog() {
    dialog->deleteLater();
  }

  KeyboardFrame *buildKeyboardFrame(TouchLineEdit *lineEdit, const QString &goText);
  KeyboardFrame *buildKeyboardFrame(QTextEdit *textEdit, const QString &goText);
};
