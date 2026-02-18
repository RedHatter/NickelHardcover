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
#include "synccontroller.h"

MenuController::MenuController(QWidget *parent) : QWidget(parent) {
  icon = reinterpret_cast<TouchLabel *>(calloc(1, 128));
  TouchLabel__constructor(icon, parent, 0);
  TouchLabel__setHitStateEnabled(icon, false);
  icon->setPixmap(QPixmap(Files::icon));

  QWidget::connect(icon, SIGNAL(tapped(bool)), this, SLOT(showMainMenu(bool)));
};

void MenuController::showMainMenu(bool checked) {
  nh_log("MenuController::showMainMenu(%s)", checked ? "true" : "false");

  icon->setPixmap(QPixmap(Files::icon_selected));

  NickelTouchMenu *menu = reinterpret_cast<NickelTouchMenu *>(calloc(1, 512));
  NickelTouchMenu__constructor(menu, icon, 0);
  NickelTouchMenu__showDecoration(menu, true);
  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);
  QWidget::connect(menu, &QMenu::aboutToHide, icon, [this] { icon->setPixmap(QPixmap(Files::icon)); });

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

  action = addMenuItem(menu, "Update book status");
  QObject::connect(action, &QAction::triggered, this, &MenuController::setBookStatus);

  QLabel *extraField = action->defaultWidget()->findChild<QLabel *>("extraField");
  if (extraField) {
    extraField->setPixmap(QPixmap(Files::right));
  }

  menu->addSeparator();

  action = addMenuItem(menu, "Open reading journal");
  QObject::connect(action, &QAction::triggered, this, &MenuController::openJournal);

  menu->addSeparator();

  action = addMenuItem(menu, "Write a review");
  QObject::connect(action, &QAction::triggered, this, &MenuController::review);

  menu->ensurePolished();
  menu->popup(icon->mapToGlobal(icon->geometry().bottomRight()) + QPoint(0, 6));
}

QWidgetAction *MenuController::addMenuItem(NickelTouchMenu *menu, QString label, bool checkable, bool checked) {
  MenuTextItem *item = reinterpret_cast<MenuTextItem *>(calloc(1, 256));
  MenuTextItem__constructor(item, menu, checkable, true);
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

  SyncController::getInstance()->prepare(true);
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

void MenuController::openJournal(bool checked) {
  nh_log("MenuController::openJournal(%s)", checked ? "true" : "false");

  JournalDialogContent::showJournalDialog();
}

void MenuController::setBookStatus(bool checked) {
  nh_log("MenuController::setBookStatus(%s)", checked ? "true" : "false");

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    networkConnected();
  } else {
    WirelessWorkflowManager__connectWireless(wfm, false, false);
    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()));
  }
}

void MenuController::networkConnected() {
  nh_log("MenuController::networkConnected()");

  CLI *cli = new CLI(this);
  cli->getUserBook();
  QObject::connect(cli, &CLI::response, this, &MenuController::showStatusMenu);
}

void MenuController::showStatusMenu(QJsonObject doc) {
  nh_log("MenuController::showStatusMenu()");

  icon->setPixmap(QPixmap(Files::icon_selected));

  NickelTouchMenu *menu = reinterpret_cast<NickelTouchMenu *>(calloc(1, 512));
  NickelTouchMenu__constructor(menu, icon, 0);
  NickelTouchMenu__showDecoration(menu, true);

  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);
  QWidget::connect(menu, &QMenu::aboutToHide, icon, [this] { icon->setPixmap(QPixmap(Files::icon)); });
  QWidget::connect(menu, &QMenu::triggered, this, &MenuController::statusSelected);

  QWidgetAction *action = addMenuItem(menu, "Back", true, true);
  QWidget::connect(action, SIGNAL(triggered(bool)), icon, SIGNAL(tapped(bool)));

  QLabel *check = action->defaultWidget()->findChild<QLabel *>("check");
  if (check) {
    check->setPixmap(QPixmap(Files::left));
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

  CLI *cli = new CLI(this);
  cli->setUserBook(status);
  QObject::connect(cli, &CLI::response, this, &MenuController::showStatusMenu);
}
