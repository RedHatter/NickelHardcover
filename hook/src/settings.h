#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "files.h"

class Settings : public QObject {
  Q_OBJECT

public:
  static Settings *getInstance();

  void setEnabled(QString contentId, bool value);
  bool isEnabled(QString contentId);

  void setLinkedId(QString contentId, QString value);
  QString getLinkedId(QString contentId);

  void setLastProgress(QString contentId, int value);
  int getLastProgress(QString contentId);

  void setAutoSyncDefault(bool value);
  bool getAutoSyncDefault();

  void setSyncBookmarks(QString value);
  QString getSyncBookmarks();

  void setJournalPrivacy(QString value);
  QString getJournalPrivacy();

  void setRetryOnNetwork(bool value);
  bool isRetryOnNetwork();

  void setSyncDaily(int value);
  int getSyncDaily();

  void setCloseThreshold(int value);
  int getCloseThreshold();

  void setPageThreshold(int value);
  int getPageThreshold();

  void setDebug(bool value);
  bool getDebug();

  bool is24HourClock();

public Q_SLOTS:
  void currentViewChanged(QString name);

private:
  Settings(QObject *parent = nullptr);

  static Settings *instance;

  QSettings *internal = nullptr;
  QSettings *config = nullptr;
  QSettings *kobo = nullptr;

  QString getPath(QString contentId, QString key);
  void setValue(QString contentId, QString key, QVariant value);
  QVariant getValue(QString contentId, QString key, QVariant defaultValue = QVariant());
};
