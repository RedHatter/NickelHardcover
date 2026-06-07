#include <QFrame>

#include "../settings/menurow.h"

class ButtonGroup : public QFrame {
  Q_OBJECT
  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

public:
  ButtonGroup(QList<Item> items, QVariant defaultValue, QString label = nullptr, QWidget *parent = nullptr);

  void setValue(QVariant value);
  QVariant value() const;

public Q_SLOTS:
  void tapped();

Q_SIGNALS:
  void valueChanged(QVariant value);

private:
  QVariant m_value;
};
