#pragma once

#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "files.h"

class Settings : public QObject {
  Q_OBJECT

public:
  static Settings *getInstance();

  void setEnabled(const QString &contentId, bool value) { setValue(contentId, "enabled", value); }
  bool isEnabled(const QString &contentId) const;

  void setLinkedId(const QString &contentId, const QString &value) { setValue(contentId, "linkedbook", value); }
  QString getLinkedId(const QString &contentId) const { return getValue(contentId, "linkedbook").toString(); }

  void setLastProgress(const QString &contentId, int value) { setValue(contentId, "progress", value); }
  int getLastProgress(const QString &contentId) const { return getValue(contentId, "progress").toInt(); }

  void clearAccessToken() { config->setValue("access_token", ""); }
  bool isSignedIn() const { return !config->value("access_token").toString().isEmpty(); }

  void setAutoSyncDefault(bool value) { config->setValue("auto_sync_default", value); }
  bool getAutoSyncDefault() const { return config->value("auto_sync_default", false).toBool(); }

  void setDebug(bool value);
  bool getDebug() const { return config->value("debug").toBool(); }

  void setJournalPrivacy(const QVariant &value) { config->setValue("journal_privacy", value); }
  QString getJournalPrivacy() const { return config->value("journal_privacy", "account").toString().toLower(); }

  void setRetryOnNetwork(bool value) { config->setValue("retry_on_network", value); }
  bool getRetryOnNetwork() const { return config->value("retry_on_network", false).toBool(); }

  void setSyncAnnotations(bool value) { config->setValue("sync_annotations", value); }
  bool getSyncAnnotations() const { return config->value("sync_annotations", false).toBool(); }

  void setSyncOnClose(const QVariant &value) { config->setValue("sync_on_close", value); }
  int getSyncOnClose() const;

  void setSyncOnRead(const QVariant &value) { config->setValue("sync_on_read", value); }
  int getSyncOnRead() const;

  void setSyncOnSchedule(const QVariant &value) { config->setValue("sync_on_schedule", value); }
  int getSyncOnSchedule() const;

  bool is24HourClock() const { return kobo->value("ApplicationPreferences/is24HourClock").toBool(); }

  void deleteConfig ();

private:
  Settings(QObject *parent = nullptr);

  void currentViewChanged(const QString &name);

  QSettings *library = nullptr;
  QSettings *config = nullptr;
  QSettings *kobo = nullptr;

  QString getPath(QString contentId, const QString &key) const;
  void setValue(const QString &contentId, const QString &key, const QVariant &value);
  QVariant getValue(const QString &contentId, const QString &key, const QVariant &defaultValue = QVariant()) const;
};
