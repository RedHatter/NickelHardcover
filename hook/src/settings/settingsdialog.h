#pragma once

#include <QJsonObject>

#include <NickelHook.h>

#include "../messages.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"
#include "menurow.h"

class SettingsDialog : public Dialog {
  Q_OBJECT

public:
  static void show() { new SettingsDialog(); }

private:
  SettingsDialog();

  void buildPages();

  void saveLogs() { nh_dump_log(); }
  void signOut();

  void clearReadProgress();
  void clearLastSynced();

  void setUsername(const Messages &user);

  PagedStack *pages = nullptr;
  MenuRow *username = nullptr;

  QFrame *buildGeneral();
  QFrame *buildAutoSync();
  QFrame *buildInformation();
  QFrame *buildAdvanced();
};
