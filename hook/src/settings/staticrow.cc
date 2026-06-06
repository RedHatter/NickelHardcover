#include <QHBoxLayout>

#include "../files.h"
#include "../nickelhardcover.h"
#include "../widgets/label.h"
#include "staticrow.h"

StaticRow::StaticRow(QString heading, QString value, bool showClear, QWidget *parent) : QFrame(parent) {
  QHBoxLayout *rowLayout = new QHBoxLayout(this);
  rowLayout->setContentsMargins(0, 0, 0, 0);

  rowLayout->addWidget(new Label(Label::Medium, heading), 1);

  label = new Label(Label::Medium, value);
  label->setProperty("style", "italic");
  rowLayout->addWidget(label);

  if (!showClear)
    return;

  TouchLabel *icon = construct_TouchLabel(this);
  icon->setPixmap(QPixmap(Files::clear));
  rowLayout->addWidget(icon);
  QWidget::connect(icon, SIGNAL(tapped(bool)), this, SIGNAL(clear()));
}

void StaticRow::setValue(QString value) { label->setText(value); }
