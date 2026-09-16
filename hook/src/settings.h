#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "files.h"

class Settings : public QObject {
  Q_OBJECT

public:
  static Settings *getInstance();

  void setEnabled(const QString &contentId, bool value);
  bool isEnabled(const QString &contentId) const;

  void setLinkedId(const QString &contentId, const QString &value);
  QString getLinkedId(const QString &contentId) const;

  void setLastProgress(const QString &contentId, int value);
  int getLastProgress(const QString &contentId) const;

  void clearAccessToken();
  bool isSignedIn() const;

  void setAutoSyncDefault(bool value);
  bool getAutoSyncDefault() const;

  void setDebug(bool value);
  bool getDebug() const;

  void setJournalPrivacy(const QVariant &value);
  QString getJournalPrivacy() const;

  void setRetryOnNetwork(bool value);
  bool getRetryOnNetwork() const;

  void setSyncAnnotations(bool value);
  bool getSyncAnnotations() const;

  void setSyncOnClose(const QVariant &value);
  int getSyncOnClose() const;

  void setSyncOnRead(const QVariant &value);
  int getSyncOnRead() const;

  void setSyncOnSchedule(const QVariant &value);
  int getSyncOnSchedule() const;

  bool is24HourClock() const;

public Q_SLOTS:
  void currentViewChanged(QString name);

private:
  Settings(QObject *parent = nullptr);

  QSettings *library = nullptr;
  QSettings *config = nullptr;
  QSettings *kobo = nullptr;

  QString getPath(QString contentId, const QString &key) const;
  void setValue(const QString &contentId, const QString &key, const QVariant &value);
  QVariant getValue(const QString &contentId, const QString &key, const QVariant &defaultValue = QVariant()) const;
};
