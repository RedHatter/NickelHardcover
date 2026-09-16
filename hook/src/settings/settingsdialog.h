#include <QJsonObject>

#include "../messages.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"
#include "menurow.h"

class SettingsDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void buildPages();

  void saveLogs();
  void signOut();

  void clearReadProgress();
  void clearLastSynced();

  void setUsername(Messages user);

private:
  SettingsDialog();

  PagedStack *pages = nullptr;
  MenuRow *username = nullptr;

  QFrame *buildGeneral();
  QFrame *buildAutoSync();
  QFrame *buildInformation();
  QFrame *buildAdvanced();
};
