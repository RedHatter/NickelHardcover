#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPixmap>
#include <QWidgetAction>

#include <NickelHook.h>

#include "cli.h"
#include "files.h"
#include "journal/journaldialog.h"
#include "menucontroller.h"
#include "review/reviewdialog.h"
#include "search/searchdialog.h"
#include "settings.h"
#include "settings/settingsdialog.h"
#include "synccontroller.h"

MenuController::MenuController(QWidget *parent) : QWidget(parent) {
  icon = construct_TouchLabel(parent);
  TouchLabel__setHitStateEnabled(icon, false);
  icon->setPixmap(QPixmap(Files::icon));

  QWidget::connect(icon, SIGNAL(tapped(bool)), this, SLOT(showMainMenu(bool)));
};

void MenuController::showMainMenu(bool checked) {
  nh_log("MenuController::showMainMenu(%s)", checked ? "true" : "false");

  icon->setPixmap(QPixmap(Files::icon_hit));

  NickelTouchMenu *menu = construct_NickelTouchMenu(icon);
  NickelTouchMenu__showDecoration(menu, true);
  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);
  QWidget::connect(menu, &QMenu::aboutToHide, icon, [this] { icon->setPixmap(QPixmap(Files::icon)); });

  QWidgetAction *action = addMenuItem(menu, "Sync now");
  QObject::connect(action, &QAction::triggered, this, &MenuController::syncNow);

  menu->addSeparator();

  QString contentId = SyncController::getInstance()->contentId;

  action = addMenuItem(menu, Settings::getInstance()->isEnabled(contentId) ? "Disable auto-sync" : "Enable auto-sync");
  QObject::connect(action, &QAction::triggered, this, &MenuController::toggleEnabled);

  menu->addSeparator();

  action = addMenuItem(menu, Settings::getInstance()->getLinkedBook(contentId).isEmpty() ? "Manually link book"
                                                                                         : "Unlink book");
  QObject::connect(action, &QAction::triggered, this, &MenuController::linkBook);

  menu->addSeparator();

  action = addMenuItem(menu, "Update book status");
  QObject::connect(action, &QAction::triggered, this, &MenuController::setBookStatus);

  QLabel *extraField = action->defaultWidget()->findChild<QLabel *>("extraField");
  if (extraField) {
    extraField->setPixmap(QPixmap(Files::arrow_right));
  }

  menu->addSeparator();

  action = addMenuItem(menu, "Open reading journal");
  QObject::connect(action, &QAction::triggered, this, &MenuController::openJournal);

  menu->addSeparator();

  action = addMenuItem(menu, "Write a review");
  QObject::connect(action, &QAction::triggered, this, &MenuController::review);

  menu->addSeparator();

  action = addMenuItem(menu, "Settings");
  QObject::connect(action, &QAction::triggered, this, &MenuController::openSettings);

  menu->ensurePolished();
  menu->popup(icon->mapToGlobal(icon->geometry().bottomRight()) + QPoint(0, 6));
}

QWidgetAction *MenuController::addMenuItem(NickelTouchMenu *menu, QString label, bool checkable, bool checked) {
  MenuTextItem *item = construct_MenuTextItem(menu, checkable, true);
  MenuTextItem__setText(item, label);
  MenuTextItem__registerForTapGestures(item);

  if (checked) {
    MenuTextItem__setSelected(item, true);
  }

  QWidgetAction *action = new QWidgetAction(menu);
  action->setDefaultWidget(item);
  action->setEnabled(true);
  menu->addAction(action);

  QObject::connect(action, &QAction::triggered, menu, &QMenu::hide);
  QObject::connect(item, SIGNAL(tapped(bool)), action, SIGNAL(triggered()));

  return action;
}

void MenuController::syncNow(bool checked) {
  nh_log("MenuController::syncNow(%s)", checked ? "true" : "false");

  SyncController::getInstance()->manualSync();
}

void MenuController::toggleEnabled(bool checked) {
  nh_log("MenuController::toggleEnabled(%s)", checked ? "true" : "false");

  QString contentId = SyncController::getInstance()->contentId;
  Settings::getInstance()->setEnabled(contentId, !Settings::getInstance()->isEnabled(contentId));
}

void MenuController::linkBook(bool checked) {
  nh_log("MenuController::linkBook(%s)", checked ? "true" : "false");

  SyncController *ctl = SyncController::getInstance();

  if (Settings::getInstance()->getLinkedBook(ctl->contentId).isEmpty()) {
    SearchDialog::show(ctl->title + " " + ctl->author);
  } else {
    Settings::getInstance()->setLinkedBook(ctl->contentId, QString());
  }
}

void MenuController::review(bool checked) {
  nh_log("MenuController::review(%s)", checked ? "true" : "false");

  ReviewDialog::show();
}

void MenuController::openJournal(bool checked) {
  nh_log("MenuController::openJournal(%s)", checked ? "true" : "false");

  JournalDialog::show();
}

void MenuController::openSettings(bool checked) {
  nh_log("MenuController::openSettings(%s)", checked ? "true" : "false");

  SettingsDialog::show();
}

void MenuController::setBookStatus(bool checked) {
  nh_log("MenuController::setBookStatus(%s)", checked ? "true" : "false");

  CLI *cli = CLI::getUserBook();
  QObject::connect(cli, &CLI::response, this, &MenuController::showStatusMenu);
}

void MenuController::showStatusMenu(QJsonObject doc) {
  nh_log("MenuController::showStatusMenu()");

  icon->setPixmap(QPixmap(Files::icon_hit));

  NickelTouchMenu *menu = construct_NickelTouchMenu(icon);
  NickelTouchMenu__showDecoration(menu, true);

  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);
  QWidget::connect(menu, &QMenu::aboutToHide, icon, [this] { icon->setPixmap(QPixmap(Files::icon)); });
  QWidget::connect(menu, &QMenu::triggered, this, &MenuController::statusSelected);

  QWidgetAction *action = addMenuItem(menu, "Back", true, true);
  QWidget::connect(action, SIGNAL(triggered(bool)), icon, SIGNAL(tapped(bool)));

  QLabel *check = action->defaultWidget()->findChild<QLabel *>("check");
  if (check) {
    check->setPixmap(QPixmap(Files::arrow_left));
  }

  menu->addSeparator();

  int status = doc.value("status_id").toInt(0);

  action = addMenuItem(menu, "Want to Read", true, status == 1);
  action->setData(1);
  menu->addSeparator();

  action = addMenuItem(menu, "Currently Reading", true, status == 2);
  action->setData(2);
  menu->addSeparator();

  action = addMenuItem(menu, "Read", true, status == 3);
  action->setData(3);
  menu->addSeparator();

  action = addMenuItem(menu, "Did Not Finish", true, status == 5);
  action->setData(5);

  menu->ensurePolished();
  menu->popup(icon->mapToGlobal(icon->geometry().bottomRight()) + QPoint(0, 6));
}

void MenuController::statusSelected(QAction *action) {
  int status = action->data().toInt();
  nh_log("MenuController::statusSelected(%i)", status);

  if (status == 0)
    return;

  CLI *cli = CLI::setUserBook(status);
  QObject::connect(cli, &CLI::response, this, &MenuController::showStatusMenu);
}
