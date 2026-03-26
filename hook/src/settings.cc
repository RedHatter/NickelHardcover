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
  internal->setValue(getPath(contentId, key), value);
}

QVariant Settings::getValue(QString contentId, QString key, QVariant defaultValue) {
  return internal->value(getPath(contentId, key), defaultValue);
}

void Settings::setEnabled(QString contentId, bool value) { setValue(contentId, "enabled", value); }

bool Settings::isEnabled(QString contentId) {
  bool defaultValue = config->value("auto_sync_default", false).toBool();
  return getValue(contentId, "enabled", defaultValue).toBool();
}

void Settings::setLinkedBook(QString contentId, QString value) {
  if (value.isEmpty()) {
    internal->remove(getPath(contentId, "linkedbook"));
  } else {
    setValue(contentId, "linkedbook", value);
  }
}

QString Settings::getLinkedBook(QString contentId) { return getValue(contentId, "linkedbook").toString(); }

void Settings::setLastProgress(QString contentId, int value) { setValue(contentId, "progress", value); }

int Settings::getLastProgress(QString contentId) { return getValue(contentId, "progress").toInt(); }

void Settings::setLastSynced(QString contentId, QString value) { setValue(contentId, "lastSynced", value); }

QString Settings::getLastSynced(QString contentId) { return getValue(contentId, "lastSynced").toString(); }

int Settings::getCloseThreshold() {
  QVariant syncOnClose = config->value("sync_on_close", "always");
  if (syncOnClose.toString() == "always") {
    return 1;
  }

  int threshold = syncOnClose.toInt();
  if (threshold > 0 && threshold < 100) {
    return threshold;
  }

  return -1;
}

int Settings::getPageThreshold() {
  int threshold = config->value("threshold", 20).toInt();
  if (threshold > 0 && threshold < 100) {
    return threshold;
  }

  return -1;
}

int Settings::getSyncDaily() {
  int hour = config->value("sync_daily").toInt();
  if (hour <= 24) {
    return hour;
  }

  return 24;
}
