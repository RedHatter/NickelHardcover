#include <NickelHook.h>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include "menucontroller.h"
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

void MenuController::itemSelected(int index) {
  nh_log("MenuController::itemSelected(%d)", index);

  ComboButton *button = qobject_cast<ComboButton *>(sender());
  SyncController *ctl = SyncController::getInstance();

  switch (index) {
  case 3:
    nh_log("Manually triggering sync");
    ctl->prepare(false);
    break;
  case 5:
    if (ctl->getLinkedBook().isEmpty()) {
      SearchDialogContent::showSearchDialog(ctl->query);
      ComboButton__renameItem(button, 5, "Unlink book");
    } else {
      nh_log("Unlinking book");
      ctl->setLinkedBook("");
      ComboButton__renameItem(button, 5, "Manually link book");
    }
    break;
  case 4:
    bool enabled = !ctl->isEnabled();

    nh_log("Setting auto-sync to %s", enabled ? "enabled" : "disabled");

    ctl->setEnabled(enabled);
    ComboButton__renameItem(button, 4, enabled ? "Disable auto-sync" : "Enable auto-sync");
    break;
  }
}

void MenuController::setupItems(ComboButton *button) {
  QTimer *timer = new QTimer(this);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, this, [this, button] {
    ComboButton__addItem(button, "Sync now", "sync-now", false);

    QString autoSynclabel = SyncController::getInstance()->isEnabled() ? "Disable auto-sync" : "Enable auto-sync";
    ComboButton__addItem(button, autoSynclabel, "toggle-auto-sync", false);

    QString linkLabel = SyncController::getInstance()->getLinkedBook().isEmpty() ? "Manually link book" : "Unlink book";
    ComboButton__addItem(button, linkLabel, "link-book", false);

    QObject::connect(button, SIGNAL(currentIndexChanged(int)), instance, SLOT(itemSelected(int)), Qt::UniqueConnection);
  });
  timer->start(10);
}
