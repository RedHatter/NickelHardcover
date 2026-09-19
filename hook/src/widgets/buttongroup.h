#pragma once

#include <QFrame>

#include "../settings/menurow.h"

class ButtonGroup : public QFrame {
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
  ButtonGroup(const QList<Item> &items, const QVariant &defaultValue, const QString &label = QString(), QWidget *parent = nullptr);

  void setValue(const QVariant &value);
  QVariant value() const { return m_value; }

public Q_SLOTS:
  void tapped() { setValue(sender()->property("value")); }

Q_SIGNALS:
  void valueChanged(QVariant value);

private:
  QVariant m_value;
};
