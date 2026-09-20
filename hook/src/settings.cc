#include <QSettings>
#include <QFile>

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

void Settings::currentViewChanged(const QString &name) {
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

bool Settings::isEnabled(const QString &contentId) const {
  bool defaultValue = config->value("auto_sync_default", false).toBool();
  return getValue(contentId, "enabled", defaultValue).toBool();
}

void Settings::setDebug(bool value) {
  if (value) {
    config->setValue("debug", value);
  } else {
    config->remove("debug");
  }
}

int Settings::getSyncOnClose() const {
  int threshold = config->value("sync_on_close", -1).toInt();
  return threshold > 0 && threshold < 100 ? threshold : -1;
}

int Settings::getSyncOnRead() const {
  int threshold = config->value("sync_on_read", -1).toInt();
  return threshold > 0 && threshold < 100 ? threshold : -1;
}

int Settings::getSyncOnSchedule() const {
  int hour = config->value("sync_on_schedule", -1).toInt();
  return hour >= 0 && hour <= 23 ? hour : -1;
}

void Settings::deleteConfig() {
  config->clear();
  config->sync();
  QFile::remove(config->fileName());
}
