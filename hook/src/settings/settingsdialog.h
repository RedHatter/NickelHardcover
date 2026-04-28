#include <QJsonObject>

#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"
#include "staticrow.h"

class SettingsDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void buildPages();

  void setAutoSyncDefault(bool value);
  void setSyncBookmarks(QVariant value);
  void setSyncDaily(QVariant value);
  void setCloseThreshold(QVariant value);
  void setPageThreshold(QVariant value);

  void setDebug(bool value);
  void saveLogs();

  void clearLastSynced();

  void setUsername(QJsonObject doc);

private:
  SettingsDialog();

  PagedStack *pages = nullptr;
  StaticRow *username = nullptr;

  QFrame *buildGeneral();
  QFrame *buildAutoSync();
  QFrame *buildInformation();
  QFrame *buildAdvanced();
};
