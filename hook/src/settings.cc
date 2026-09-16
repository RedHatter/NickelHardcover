#include <QSettings>

#include <NickelHook.h>

#include "settings.h"
#include "synccontroller.h"

Settings *Settings::getInstance() {
  static Settings instance;
  return &instance;
}

Settings::Settings(QObject *parent)
    : QObject(parent), library(new QSettings(Files::library, QSettings::IniFormat, this)),
      config(new QSettings(Files::config, QSettings::IniFormat, this)),
      kobo(new QSettings(Files::koboSettings, QSettings::IniFormat, this)) {
  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, this,
                   &Settings::currentViewChanged);
}

void Settings::currentViewChanged(QString name) {
  if (name == "ReadingView") {
    library->sync();
    config->sync();
  }
}

QString Settings::getPath(QString contentId, const QString &key) const {
  return contentId.replace('/', '-').replace('\\', '-') + "/" + key;
}

void Settings::setValue(const QString &contentId, const QString &key, const QVariant &value) {
  if (value.isNull()) {
    library->remove(getPath(contentId, key));
  } else {
    library->setValue(getPath(contentId, key), value);
  }
}

QVariant Settings::getValue(const QString &contentId, const QString &key, const QVariant &defaultValue) const {
  return library->value(getPath(contentId, key), defaultValue);
}

void Settings::setEnabled(const QString &contentId, bool value) { setValue(contentId, "enabled", value); }

bool Settings::isEnabled(const QString &contentId) const {
  bool defaultValue = config->value("auto_sync_default", false).toBool();
  return getValue(contentId, "enabled", defaultValue).toBool();
}

void Settings::setLinkedId(const QString &contentId, const QString &value) { setValue(contentId, "linkedbook", value); }

QString Settings::getLinkedId(const QString &contentId) const { return getValue(contentId, "linkedbook").toString(); }

void Settings::setLastProgress(const QString &contentId, int value) { setValue(contentId, "progress", value); }

int Settings::getLastProgress(const QString &contentId) const { return getValue(contentId, "progress").toInt(); }

void Settings::clearAccessToken() { config->setValue("access_token", ""); }

bool Settings::isSignedIn() const { return !config->value("access_token").toString().isEmpty(); }

void Settings::setAutoSyncDefault(bool value) { config->setValue("auto_sync_default", value); }

bool Settings::getAutoSyncDefault() const { return config->value("auto_sync_default", false).toBool(); }

void Settings::setDebug(bool value) {
  if (value) {
    config->setValue("debug", value);
  } else {
    config->remove("debug");
  }
}

bool Settings::getDebug() const { return config->value("debug").toBool(); }

void Settings::setJournalPrivacy(const QVariant &value) { config->setValue("journal_privacy", value); }

QString Settings::getJournalPrivacy() const { return config->value("journal_privacy", "account").toString().toLower(); }

void Settings::setRetryOnNetwork(bool value) { config->setValue("retry_on_network", value); }

bool Settings::getRetryOnNetwork() const { return config->value("retry_on_network", false).toBool(); }

void Settings::setSyncAnnotations(bool value) { config->setValue("sync_annotations", value); }

bool Settings::getSyncAnnotations() const { return config->value("sync_annotations", false).toBool(); }

void Settings::setSyncOnClose(const QVariant &value) { config->setValue("sync_on_close", value); }

int Settings::getSyncOnClose() const {
  int threshold = config->value("sync_on_close", -1).toInt();
  return threshold > 0 && threshold < 100 ? threshold : -1;
}

void Settings::setSyncOnRead(const QVariant &value) { config->setValue("sync_on_read", value); }

int Settings::getSyncOnRead() const {
  int threshold = config->value("sync_on_read", -1).toInt();
  return threshold > 0 && threshold < 100 ? threshold : -1;
}

void Settings::setSyncOnSchedule(const QVariant &value) { config->setValue("sync_on_schedule", value); }

int Settings::getSyncOnSchedule() const {
  int hour = config->value("sync_on_schedule", -1).toInt();
  return hour >= 0 && hour <= 23 ? hour : -1;
}

bool Settings::is24HourClock() const { return kobo->value("ApplicationPreferences/is24HourClock").toBool(); }
