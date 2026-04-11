#include <QHBoxLayout>

#include <NickelHook.h>

#include "../files.h"
#include "../nickelhardcover.h"
#include "rating.h"

Rating::Rating(float value, QWidget *parent) : QWidget(parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(0);

  for (int i = 0; i < 5; i++) {
    TouchLabel *icon = construct_TouchLabel(this);
    TouchLabel__setHitStateEnabled(icon, false);
    icon->setPixmap(QPixmap(i < value ? Files::left_star_hit : Files::left_star));
    icon->setContentsMargins(16, 0, 0, 0);
    layout->addWidget(icon);

    QObject::connect(icon, SIGNAL(mouseDown()), this, SLOT(mouseDown()));

    icon = construct_TouchLabel(this);
    TouchLabel__setHitStateEnabled(icon, false);
    icon->setPixmap(QPixmap(i + 0.5 < value ? Files::right_star_hit : Files::right_star));
    icon->setContentsMargins(0, 0, 16, 0);
    layout->addWidget(icon);

    QObject::connect(icon, SIGNAL(mouseDown()), this, SLOT(mouseDown()));
  }

  layout->addStretch(1);
}

void Rating::mouseDown() {
  QObject *icon = sender();

  int value = 10;

  for (int i = 0; i < 10; i++) {
    TouchLabel *item = qobject_cast<TouchLabel *>(layout()->itemAt(i)->widget());
    if (i % 2 == 0) {
      item->setPixmap(QPixmap(i < value ? Files::left_star_hit : Files::left_star));
    } else {
      item->setPixmap(QPixmap(i < value ? Files::right_star_hit : Files::right_star));
    }

    if (item == icon) {
      value = i;
    }
  }

  tapped((float)(value + 1) / 2);
}
