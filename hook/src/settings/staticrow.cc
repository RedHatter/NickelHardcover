#include <QHBoxLayout>

#include "staticrow.h"

StaticRow::StaticRow(QString heading, QString value, QWidget *parent) : QFrame(parent) {
  QHBoxLayout *rowLayout = new QHBoxLayout(this);
  rowLayout->setContentsMargins(QMargins(28, 30, 28, 30));

  label = new QLabel(heading);
  rowLayout->addWidget(label, 1);

  label = new QLabel(value);
  label->setObjectName("value");
  rowLayout->addWidget(label);
}

void StaticRow::setValue(QString value) { label->setText(value); }
