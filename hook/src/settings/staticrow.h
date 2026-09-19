#pragma once

#include <QFrame>
#include <QLabel>
#include <QVariant>

class StaticRow : public QFrame {
  Q_OBJECT

public:
  StaticRow(const QString &heading, const QString &value, bool showClear, QWidget *parent = nullptr);

  void setValue(const QString &value) { label->setText(value); }

Q_SIGNALS:
  void clear();

private:
  QLabel *label = nullptr;
};
