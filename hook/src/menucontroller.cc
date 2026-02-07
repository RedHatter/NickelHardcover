#include <QLabel>
#include <QPixmap>
#include <QWidgetAction>

#include <NickelHook.h>

#include "files.h"
#include "menucontroller.h"
#include "review/reviewdialog.h"
#include "search/searchdialog.h"
#include "synccontroller.h"

MenuController *MenuController::instance;

MenuController *MenuController::getInstance() {
  if (!instance) {
    instance = new MenuController();
  };

  return instance;
};

MenuController::MenuController(QObject *parent) : QObject(parent) {};

QWidget *MenuController::buildWidget(QWidget *parent) {
  TouchLabel *icon = reinterpret_cast<TouchLabel *>(calloc(1, 128));
  TouchLabel__constructor(icon, parent, 0);
  TouchLabel__setHitStateEnabled(icon, false);
  icon->setPixmap(QPixmap(Files::icon));

  QWidget::connect(icon, SIGNAL(tapped(bool)), this, SLOT(showMenu(bool)));

  return icon;
}

void MenuController::showMenu(bool checked) {
  nh_log("MenuController::showMenu(%s)", checked ? "true" : "false");

  TouchLabel *icon = qobject_cast<TouchLabel *>(sender());
  icon->setPixmap(QPixmap(Files::icon_selected));

  NickelTouchMenu *menu = reinterpret_cast<NickelTouchMenu *>(calloc(1, 512));
  NickelTouchMenu__constructor(menu, icon, 0);
  NickelTouchMenu__showDecoration(menu, true);
  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);
  QWidget::connect(menu, &QMenu::aboutToHide, icon, [icon] { icon->setPixmap(QPixmap(Files::icon)); });

  QWidgetAction *action = addMenuItem(menu, "Sync now");
  QObject::connect(action, &QAction::triggered, this, &MenuController::syncNow);

  menu->addSeparator();

  SyncController *ctl = SyncController::getInstance();

  action = addMenuItem(menu, ctl->isEnabled() ? "Disable auto-sync" : "Enable auto-sync");
  QObject::connect(action, &QAction::triggered, this, &MenuController::toggleEnabled);

  menu->addSeparator();

  action = addMenuItem(menu, ctl->getLinkedBook().isEmpty() ? "Manually link book" : "Unlink book");
  QObject::connect(action, &QAction::triggered, this, &MenuController::linkBook);

  menu->addSeparator();

  action = addMenuItem(menu, "Write a review");
  QObject::connect(action, &QAction::triggered, this, &MenuController::review);

  menu->ensurePolished();
  menu->popup(icon->mapToGlobal(icon->geometry().bottomRight()) + QPoint(0, 6));
}

QWidgetAction *MenuController::addMenuItem(NickelTouchMenu *menu, QString label) {
  MenuTextItem *item = reinterpret_cast<MenuTextItem *>(calloc(1, 256));
  MenuTextItem__constructor(item, menu, false, true);
  MenuTextItem__setText(item, label);
  MenuTextItem__registerForTapGestures(item);

  QWidgetAction *action = new QWidgetAction(menu);
  action->setDefaultWidget(item);
  action->setEnabled(true);
  menu->addAction(action);

  QWidget::connect(action, &QAction::triggered, menu, &QMenu::hide);
  QWidget::connect(item, SIGNAL(tapped(bool)), action, SIGNAL(triggered()));

  return action;
}

void MenuController::syncNow(bool checked) {
  nh_log("MenuController::syncNow(%s)", checked ? "true" : "false");

  SyncController::getInstance()->prepare(false);
}

void MenuController::toggleEnabled(bool checked) {
  nh_log("MenuController::toggleEnabled(%s)", checked ? "true" : "false");

  SyncController *ctl = SyncController::getInstance();
  ctl->setEnabled(!ctl->isEnabled());
}

void MenuController::linkBook(bool checked) {
  nh_log("MenuController::linkBook(%s)", checked ? "true" : "false");

  SyncController *ctl = SyncController::getInstance();

  if (ctl->getLinkedBook().isEmpty()) {
    SearchDialogContent::showSearchDialog(ctl->title + " " + ctl->author);
  } else {
    ctl->setLinkedBook("");
  }
}

void MenuController::review(bool checked) {
  nh_log("MenuController::review(%s)", checked ? "true" : "false");

  ReviewDialogContent::showReviewDialog();
}
