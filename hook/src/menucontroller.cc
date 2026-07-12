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

NickelTouchMenu *MenuController::showMenu(QList<Item> items, QWidget *anchor, int offset, bool checkable,
                                          bool decorated) {
  NickelTouchMenu *menu = construct_NickelTouchMenu(anchor);
  NickelTouchMenu__showDecoration(menu, decorated);

  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);

  for (Item item : items) {
    MenuTextItem *menuItem = construct_MenuTextItem(menu, checkable, true);
    MenuTextItem__setText(menuItem, item.text);

    if (item.checked) {
      MenuTextItem__setSelected(menuItem, true);
    }

    QWidgetAction *action = new QWidgetAction(menu);
    action->setDefaultWidget(menuItem);
    action->setData(item.value);
    menu->addAction(action);

    if (item.disabled) {
      nh_log("%s disabled", qPrintable(item.text));
      action->setEnabled(false);
      menuItem->setStyleSheet("color: #666666;");
    } else {
      action->setEnabled(true);
      MenuTextItem__registerForTapGestures(menuItem);
    }

    QObject::connect(action, &QAction::triggered, menu, &QMenu::hide);
    QObject::connect(menuItem, SIGNAL(tapped(bool)), action, SIGNAL(triggered()));

    menu->addSeparator();
  }

  menu->ensurePolished();
  menu->popup(anchor->parentWidget()->mapToGlobal(anchor->geometry().bottomLeft()) + QPoint(0, offset));

  return menu;
}

MenuController::MenuController(int iconHeight, QWidget *parent) : QWidget(parent), iconHeight(iconHeight) {
  icon = construct_TouchLabel(parent);
  TouchLabel__setHitStateEnabled(icon, false);
  setSelected(false);

  QWidget::connect(icon, SIGNAL(tapped(bool)), this, SLOT(showMainMenu()));
};

void MenuController::setSelected(bool selected) {
  icon->setPixmap(QPixmap(selected ? Files::icon_hit : Files::icon).scaledToHeight(iconHeight));
}

enum MenuOption {
  BACK,
  WANT_TO_READ = 1,
  CURRENTLY_READING = 2,
  READ = 3,
  DID_NOT_FINISH = 5,
  SYNC_NOW,
  TOGGLE_ENABLED,
  LINK,
  BOOK_STATUS,
  JOURNAL,
  REVIEW,
  SETTINGS,
};

void MenuController::showMainMenu() {
  nh_log("MenuController::showMainMenu()");

  setSelected(true);

  SyncController *syncController = SyncController::getInstance();
  QString contentId = syncController->contentId;
  Settings *settings = Settings::getInstance();

  NickelTouchMenu *menu = showMenu(
      {
          {"Sync now", MenuOption::SYNC_NOW, false, syncController->syncDisabled},
          {!settings->isEnabled(contentId) || syncController->syncDisabled ? "Enable auto-sync" : "Disable auto-sync",
           MenuOption::TOGGLE_ENABLED, false, syncController->syncDisabled},
          {settings->getLinkedId(contentId).isEmpty() ? "Manually link book" : "Unlink book", MenuOption::LINK},
          {"Update book status", MenuOption::BOOK_STATUS},
          {"Open reading journal", MenuOption::JOURNAL},
          {"Write a review", MenuOption::REVIEW},
          {"Settings", MenuOption::SETTINGS},
      },
      icon, 6);
  QWidget::connect(menu, &QMenu::aboutToHide, icon, [this] { setSelected(false); });
  QWidget::connect(menu, &QMenu::triggered, this, &MenuController::triggered);

  QList<QLabel *> extraField = menu->findChildren<QLabel *>("extraField");
  if (extraField[3]) {
    extraField[3]->setPixmap(QPixmap(Files::arrow_right));
  }
}

void MenuController::showStatusMenu(QJsonObject doc) {
  int status = doc.value("status_id").toInt(0);
  nh_log("MenuController::showStatusMenu(%d)", status);

  setSelected(true);

  NickelTouchMenu *menu = showMenu(
      {
          {"Back", MenuOption::BACK, true},
          {"Want to Read", MenuOption::WANT_TO_READ, status == MenuOption::WANT_TO_READ},
          {"Currently Reading", MenuOption::CURRENTLY_READING, status == MenuOption::CURRENTLY_READING},
          {"Read", MenuOption::READ, status == MenuOption::READ},
          {"Did Not Finish", MenuOption::DID_NOT_FINISH, status == MenuOption::DID_NOT_FINISH},
      },
      icon, 6, true);
  NickelTouchMenu__showDecoration(menu, true);

  QWidget::connect(menu, &QMenu::aboutToHide, icon, [this] { setSelected(false); });
  QWidget::connect(menu, &QMenu::triggered, this, &MenuController::triggered);

  QLabel *check = menu->findChild<QLabel *>("check");
  if (check) {
    check->setPixmap(QPixmap(Files::arrow_left));
  }
}

void MenuController::triggered(QAction *action) {
  int value = action->data().toInt();
  nh_log("MenuController::triggered(%d)", value);

  switch (value) {
  case MenuOption::BACK:
    showMainMenu();
    break;

  case MenuOption::WANT_TO_READ:
  case MenuOption::CURRENTLY_READING:
  case MenuOption::READ:
  case MenuOption::DID_NOT_FINISH: {
    CLI *cli = CLI::setUserBook(value);
    QObject::connect(cli, &CLI::response, this, &MenuController::showStatusMenu);
    break;
  }

  case MenuOption::SYNC_NOW:
    SyncController::getInstance()->manualSync();
    break;

  case MenuOption::TOGGLE_ENABLED: {
    QString contentId = SyncController::getInstance()->contentId;
    Settings::getInstance()->setEnabled(contentId, !Settings::getInstance()->isEnabled(contentId));
    break;
  }

  case MenuOption::LINK: {
    SyncController *ctl = SyncController::getInstance();

    if (Settings::getInstance()->getLinkedId(ctl->contentId).isEmpty()) {
      SearchDialog::show(ctl->contentId, ctl->title + " " + ctl->author);
    } else {
      Settings::getInstance()->setLinkedId(ctl->contentId, QString());
    }
    break;
  }

  case MenuOption::BOOK_STATUS: {
    CLI *cli = CLI::getUserBook();
    QObject::connect(cli, &CLI::response, this, &MenuController::showStatusMenu);
    break;
  }

  case MenuOption::JOURNAL:
    JournalDialog::show();
    break;

  case MenuOption::REVIEW:
    ReviewDialog::show();
    break;

  case MenuOption::SETTINGS:
    SettingsDialog::show();
    break;
  }
}
