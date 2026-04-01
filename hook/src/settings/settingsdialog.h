#include <QJsonObject>

#include "../widgets/dialog.h"
#include "staticrow.h"

class SettingsDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void setAutoSyncDefault(QVariant value);
  void setSyncBookmarks(QVariant value);
  void setSyncDaily(QVariant value);
  void setCloseThreshold(QVariant value);
  void setPageThreshold(QVariant value);

  void setUsername(QJsonObject doc);

private:
  SettingsDialog();

  StaticRow *username = nullptr;
};
