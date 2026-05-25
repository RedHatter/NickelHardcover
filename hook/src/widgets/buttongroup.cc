#include <NickelHook.h>
#include <QHBoxLayout>
#include <QStyle>

#include "../nickelhardcover.h"
#include "buttongroup.h"

ButtonGroup::ButtonGroup(QList<Item> items, QVariant defaultValue, QString label, QWidget *parent)
    : QWidget(parent), m_value(defaultValue) {
  setStyleSheet(R"(
    N3ButtonLabel {
      font-family: Avenir;
      font-style: normal;
      background-color: #d9d9d9;
    }

    N3ButtonLabel[selected=true] {
      background-color: #262626;
      color: white;
    }
  )");

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(2);

  if (label != nullptr) {
    QLabel *widget = new QLabel(label);
    widget->setObjectName("regular");
    layout->addWidget(widget);
    layout->addSpacing(18);
  }

  for (Item item : items) {
    N3ButtonLabel *button = construct_N3ButtonLabel(this);
    button->setText(item.text);
    button->setProperty("value", item.value);
    button->setProperty("selected", item.value == defaultValue);
    layout->addWidget(button);
    QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(tapped()));
  }
}

void ButtonGroup::setValue(QVariant value) {
  if (value == m_value)
    return;

  m_value = value;

  QList<QLabel *> children = findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
  for (QLabel *child : children) {
    child->setProperty("selected", child->property("value") == value);
    QStyle *style = child->style();
    style->unpolish(child);
    style->polish(child);
    child->update();
  }

  valueChanged(value);
}

QVariant ButtonGroup::value() const { return m_value; }

void ButtonGroup::tapped() { setValue(sender()->property("value")); }
