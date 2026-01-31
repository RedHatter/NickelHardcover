#include <NickelHook.h>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include "cli.h"
#include "menucontroller.h"
#include "settings.h"

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

  switch (index) {
  case 3:
    nh_log("Manually triggering sync");
    CLI::getInstance()->prepare(false);
    break;
  case 4:
    bool enabled = !Settings::getInstance()->isEnabled();

    nh_log("Setting auto-sync to %s", enabled ? "enabled" : "disabled");

    Settings::getInstance()->setEnabled(enabled);

    ComboButton *button = qobject_cast<ComboButton *>(sender());
    ComboButton__renameItem(button, 4, enabled ? "Disable auto-sync" : "Enable auto-sync");
    break;
  }
}

void MenuController::setupItems(ComboButton *button) {
  QTimer *timer = new QTimer(this);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, this, [this, button] {
    ComboButton__addItem(button, "Sync now", "sync-now", false);

    QString label = Settings::getInstance()->isEnabled() ? "Disable auto-sync" : "Enable auto-sync";
    ComboButton__addItem(button, label, "toggle-auto-sync", false);

    QObject::connect(button, SIGNAL(currentIndexChanged(int)), instance, SLOT(itemSelected(int)), Qt::UniqueConnection);
  });
  timer->start(10);
}
