#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include <stdlib.h>

#include "cli.h"
#include "settings.h"
#include "synccontroller.h"

Settings *Settings::instance = nullptr;

Settings *Settings::getInstance() {
  if (instance == nullptr) {
    instance = new Settings();
  };

  return instance;
};

Settings::Settings(QObject *parent)
    : QObject(parent), internal(new QSettings(Files::settings, QSettings::IniFormat)),
      config(new QSettings(Files::config, QSettings::IniFormat)) {
  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, this,
                   &Settings::currentViewChanged);
};

void Settings::currentViewChanged(QString name) {
  if (name == "ReadingView") {
    internal->sync();
    config->sync();
  }
}

QString Settings::getPath(QString contentId, QString key) {
  return contentId.replace('/', '-').replace('\\', '-') + "/" + key;
}

void Settings::setValue(QString contentId, QString key, QVariant value) {
  if (value.isNull()) {
    internal->remove(getPath(contentId, key));
  } else {
    internal->setValue(getPath(contentId, key), value);
  }
}

QVariant Settings::getValue(QString contentId, QString key, QVariant defaultValue) {
  return internal->value(getPath(contentId, key), defaultValue);
}

void Settings::setEnabled(QString contentId, bool value) { setValue(contentId, "enabled", value); }

bool Settings::isEnabled(QString contentId) {
  bool defaultValue = config->value("auto_sync_default", false).toBool();
  return getValue(contentId, "enabled", defaultValue).toBool();
}

void Settings::setLinkedBook(QString contentId, QString value) { setValue(contentId, "linkedbook", value); }

QString Settings::getLinkedBook(QString contentId) { return getValue(contentId, "linkedbook").toString(); }

void Settings::setLastProgress(QString contentId, int value) { setValue(contentId, "progress", value); }

int Settings::getLastProgress(QString contentId) { return getValue(contentId, "progress").toInt(); }

void Settings::setLastSynced(QString contentId, QString value) { setValue(contentId, "lastSynced", value); }

QString Settings::getLastSynced(QString contentId) { return getValue(contentId, "lastSynced").toString(); }

void Settings::setSyncDaily(int value) { config->setValue("sync_daily", value); }

int Settings::getSyncDaily() {
  int hour = config->value("sync_daily").toInt();
  if (hour <= 0) {
    return 0;
  }

  if (hour <= 24) {
    return hour;
  }

  return 0;
}

void Settings::setAutoSyncDefault(bool value) { config->setValue("auto_sync_default", value); }

bool Settings::getAutoSyncDefault() { return config->value("auto_sync_default", false).toBool(); }

void Settings::setSyncBookmarks(QString value) { config->setValue("sync_bookmarks", value); }

QString Settings::getSyncBookmarks() { return config->value("sync_bookmarks", "never").toString(); }

void Settings::setCloseThreshold(int value) {
  QVariant realValue = "never";

  if (value == 1) {
    realValue = "always";
  } else if (value > 0 && value < 100) {
    realValue = value;
  }

  config->setValue("sync_on_close", realValue);
}

int Settings::getCloseThreshold() {
  QVariant syncOnClose = config->value("sync_on_close", "always");
  if (syncOnClose.toString() == "always") {
    return 1;
  }

  int threshold = syncOnClose.toInt();
  if (threshold > 0 && threshold < 100) {
    return threshold;
  }

  return 0;
}

int Settings::getPageThreshold() {
  int threshold = config->value("threshold").toInt();
  if (threshold > 0 && threshold < 100) {
    return threshold;
  }

  return 0;
}

void Settings::setPageThreshold(int value) { config->setValue("threshold", value > 0 && value < 100 ? value : 0); }
