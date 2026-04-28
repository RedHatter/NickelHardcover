#include <QHBoxLayout>
#include <QLabel>
#include <QWidgetAction>

#include <NickelHook.h>

#include "../files.h"
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
  QLabel *headingLabel = new QLabel(heading);
  headingLabel->setObjectName("regular");
  rowLayout->addWidget(headingLabel, 1);

  label = new QLabel("Unset");
  label->setObjectName("regular");
  label->setStyleSheet("font-style: italic;");
  rowLayout->addWidget(label);

  QObject::connect(row, SIGNAL(tapped()), this, SLOT(tapped()));

  if (type == MenuRowType::Menu) {
    QLabel *icon = new QLabel();
    icon->setPixmap(QPixmap(Files::arrow_menu));
    rowLayout->addWidget(icon);
  }

  for (int i = 0; i < menuItems.size(); ++i) {
    Item item = menuItems.at(i);
    if (item.value == defaultValue) {
      this->item = item;
      label->setText(item.text);
      return;
    }
  }

  for (int i = 0; i < dialogItems.size(); ++i) {
    Item item = dialogItems.at(i);
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
    openDialog();
    break;

  case MenuRowType::Menu:
    openMenu();
    break;
  }
}

void MenuRow::setItem(Item item) {
  this->item = item;
  label->setText(item.text);

  triggered(item.value);
}

void MenuRow::openDialog() {
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

  dialogLabel = new QLabel(dialogItems.at(0).text);
  dialogLabel->setObjectName("regular");
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

void MenuRow::openMenu() {
  NickelTouchMenu *menu = construct_NickelTouchMenu(label);
  NickelTouchMenu__showDecoration(menu, false);
  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);

  for (int i = 0; i < menuItems.size(); ++i) {
    if (i > 0) {
      menu->addSeparator();
    }

    Item item = menuItems.at(i);

    MenuTextItem *menuItem = construct_MenuTextItem(menu, false, true);
    MenuTextItem__setText(menuItem, item.text);
    MenuTextItem__registerForTapGestures(menuItem);

    QWidgetAction *action = new QWidgetAction(menu);
    action->setDefaultWidget(menuItem);
    action->setEnabled(true);
    menu->addAction(action);

    QObject::connect(action, &QAction::triggered, menu, &QMenu::hide);
    QObject::connect(menuItem, SIGNAL(tapped(bool)), action, SIGNAL(triggered()));
    QObject::connect(action, &QAction::triggered, [this, item]() {
      if (item.value == MenuRow::OPEN_DIALOG) {
        openDialog();
      } else {
        setItem(item);
      }
    });
  }

  menu->ensurePolished();
  menu->popup(label->mapToGlobal(label->geometry().topRight() - QPoint(0, 56)));
}
