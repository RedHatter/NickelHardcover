#include <QHBoxLayout>

#include "../files.h"
#include "../nickelhardcover.h"
#include "staticrow.h"

StaticRow::StaticRow(QString heading, QString value, bool showClear, QWidget *parent) : QFrame(parent) {
  QHBoxLayout *rowLayout = new QHBoxLayout(this);
  rowLayout->setContentsMargins(0, 0, 0, 0);

  label = new QLabel(heading);
  label->setObjectName("regular");
  rowLayout->addWidget(label, 1);

  label = new QLabel(value);
  label->setObjectName("regular");
  label->setStyleSheet("font-style: italic;");
  rowLayout->addWidget(label);

  if (!showClear)
    return;

  TouchLabel *icon = construct_TouchLabel(this);
  icon->setPixmap(QPixmap(Files::clear));
  rowLayout->addWidget(icon);
  QWidget::connect(icon, SIGNAL(tapped(bool)), this, SIGNAL(clear()));
}

void StaticRow::setValue(QString value) { label->setText(value); }
