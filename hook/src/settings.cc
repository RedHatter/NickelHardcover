#include <NickelHook.h>
#include <QSettings>

#include "nickelhardcover.h"
#include "settings.h"

Settings *Settings::instance;

Settings *Settings::getInstance() {
  if (!instance) {
    instance = new Settings();
  };

  return instance;
};

Settings::Settings(QObject *parent) : QObject(parent) { internal = new QSettings(PATH + "settings.ini", QSettings::IniFormat); }

QString Settings::getContentId() { return contentId; }

void Settings::setContentId(QString value) {
  contentId = value;
  key = value.replace('/', '-').replace('\\', '-').append("/enabled");
  nh_log("'%s' '%s'", qPrintable(key), qPrintable(contentId));
}

bool Settings::isEnabled() { return internal->value(key, false).toBool(); }

void Settings::setEnabled(bool value) { internal->setValue(key, value); }
