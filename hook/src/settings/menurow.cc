#include <QHBoxLayout>
#include <QLabel>
#include <QWidgetAction>

#include <NickelHook.h>

#include "../files.h"
#include "../menucontroller.h"
#include "../widgets/label.h"
#include "menurow.h"

QVariant MenuRow::OPEN_DIALOG = QVariant("OPEN_DIALOG");

MenuRow::MenuRow(QString heading, MenuRowType type, QList<Item> menuItems, QList<Item> dialogItems,
                 QVariant defaultValue, QWidget *parent)
    : QWidget(parent), type(type), menuItems(menuItems), dialogItems(dialogItems) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  SettingContainer *row = construct_SettingContainer(this);
  layout->addWidget(row);

  QHBoxLayout *rowLayout = new QHBoxLayout(row);
  rowLayout->setContentsMargins(0, 0, 0, 0);
  rowLayout->addWidget(new Label(Label::Medium, heading), 1);

  label = new Label(Label::Medium, "Unset");
  label->setProperty("style", "italic");
  rowLayout->addWidget(label);

  QObject::connect(row, SIGNAL(tapped()), this, SLOT(tapped()));

  if (type == MenuRowType::Menu) {
    QLabel *icon = new QLabel();
    icon->setPixmap(QPixmap(Files::arrow_menu));
    rowLayout->addWidget(icon);
  }

  for (Item item : menuItems) {
    if (item.value == defaultValue) {
      this->item = item;
      label->setText(item.text);
      return;
    }
  }

  for (Item item : dialogItems) {
    if (item.value == defaultValue) {
      this->item = item;
      label->setText(item.text);
      return;
    }
  }
}

void MenuRow::tapped() {
  nh_log("MenuRow::tapped()");

  switch (type) {
  case MenuRowType::Tap:
    setItem(menuItems.at(0));
    break;

  case MenuRowType::Dialog:
    showDialog();
    break;

  case MenuRowType::Menu:
    showMenu();
    break;
  }
}

void MenuRow::setItem(Item item) {
  this->item = item;
  label->setText(item.text);

  triggered(item.value);
}

void MenuRow::showDialog() {
  ConfirmationDialog *dialog = ConfirmationDialogFactory__getConfirmationDialog(this);
  QLabel *title = dialog->findChild<QLabel *>("title");
  title->setText("Enter a value");
  title->show();

  QLabel *rejectButton = dialog->findChild<QLabel *>("rejectButton");
  rejectButton->setText("Cancel");
  rejectButton->show();
  QObject::connect(rejectButton, SIGNAL(tapped(bool)), dialog, SLOT(deleteLater()));

  QLabel *acceptButton = dialog->findChild<QLabel *>("acceptButton");
  acceptButton->setText("Set");
  acceptButton->show();
  QObject::connect(acceptButton, SIGNAL(tapped(bool)), this, SLOT(accept()));

  dialog->findChild<QFrame *>("bottomLine")->show();
  dialog->findChild<QFrame *>("topLine")->show();
  dialog->findChild<QLabel *>("text")->hide();

  QVBoxLayout *layout = dialog->findChild<QVBoxLayout *>("contentLayout");

  QHBoxLayout *row = new QHBoxLayout();
  row->setContentsMargins(0, 0, 0, 0);
  row->setSpacing(0);
  layout->addLayout(row);

  dialogLabel = new Label(Label::Medium, dialogItems.at(0).text);
  dialogLabel->setAlignment(Qt::AlignCenter);
  row->addWidget(dialogLabel, 1);

  for (int i = 0; i < dialogItems.size(); ++i) {
    Item item = dialogItems.at(i);
    if (item.value == this->item.value) {
      index = i;
      dialogLabel->setText(item.text);
    }
  }

  QVBoxLayout *buttons = new QVBoxLayout();
  buttons->setContentsMargins(0, 0, 0, 0);
  buttons->setSpacing(0);
  row->addLayout(buttons);

  TouchLabel *button = construct_TouchLabel(dialog);
  button->setPixmap(QPixmap(Files::arrow_up));
  buttons->addWidget(button);
  QWidget::connect(button, SIGNAL(tapped(bool)), this, SLOT(up()));

  button = construct_TouchLabel(dialog);
  button->setPixmap(QPixmap(Files::arrow_down));
  buttons->addWidget(button);
  QWidget::connect(button, SIGNAL(tapped(bool)), this, SLOT(down()));

  dialog->open();
}

void MenuRow::up() {
  nh_log("MenuRow::up()");

  if (index < dialogItems.size() - 1) {
    index++;
  } else {
    index = 0;
  }

  dialogLabel->setText(dialogItems.at(index).text);
}

void MenuRow::down() {
  nh_log("MenuRow::down()");

  if (index > 0) {
    index--;
  } else {
    index = dialogItems.size() - 1;
  }

  dialogLabel->setText(dialogItems.at(index).text);
}

void MenuRow::accept() { setItem(dialogItems.at(index)); }

void MenuRow::showMenu() {
  NickelTouchMenu *menu = MenuController::buildMenu(menuItems, label, -sizeHint().height(), false, false);
  QWidget::connect(menu, &QMenu::triggered, this, &MenuRow::menuTriggered);
}

void MenuRow::menuTriggered(QAction *action) {
  QVariant value = action->data();

  for (Item item : menuItems) {
    if (item.value != value) {
      continue;
    }

    if (item.value == MenuRow::OPEN_DIALOG) {
      showDialog();
    } else {
      setItem(item);
    }
  }
}
