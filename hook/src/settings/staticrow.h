#pragma once

#include <QFrame>
#include <QLabel>
#include <QVariant>

class StaticRow : public QFrame {
  Q_OBJECT

public:
  StaticRow(QString heading, QString value, bool showClear, QWidget *parent = nullptr);

  void setValue(QString value);

Q_SIGNALS:
  void clear();

private:
  QLabel *label = nullptr;
};
