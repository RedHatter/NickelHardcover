#pragma once

#include <QFrame>
#include <QLabel>
#include <QVariant>

class StaticRow : public QFrame {
  Q_OBJECT

public:
  StaticRow(QString heading, QString value, QWidget *parent = nullptr);

  void setValue(QString value);

private:
  QLabel *label = nullptr;
};
