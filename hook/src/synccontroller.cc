#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include "settings.h"
#include "synccontroller.h"

SyncController *SyncController::instance = nullptr;

SyncController *SyncController::getInstance() {
  if (instance == nullptr) {
    instance = new SyncController();
  };

  return instance;
};

SyncController::SyncController(QObject *parent) : QObject(parent) {};

void SyncController::currentViewIndexChanged(int index) {
  if (index < 0)
    return;

  queue->failed = false;

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();
  nh_log("Current view changed to %s last view was %s", qPrintable(name), qPrintable(lastViewName));

  if (name != lastViewName) {
    currentViewChanged(name);
  }

  QDateTime alarm = getAlarm();
  if (name.endsWith("DragonPowerView") && alarm.isValid()) {
    nh_log("Alarm set for %s", qPrintable(alarm.toString()));
  }

  if (syncDisabled) {
    return;
  }

  Settings *settings = Settings::getInstance();
  if (lastViewName == "ReadingView" &&
      (name.endsWith("DragonPowerView") || MainWindowController__viewWithObjectName(mwc, "ReadingView") == nullptr) &&
      settings->isEnabled(contentId) && queue->checkThreshold(contentId, settings->getCloseThreshold())) {
    nh_log("Triggered on close auto-sync");
    queue->run(contentId);
  }

  if (name == "ReadingView") {
    if (settings->isEnabled(contentId)) {
      queue->updateReadProgress(contentId);
    }

    QObject::connect(cv, SIGNAL(pageChanged(int)), this, SLOT(pageChanged()), Qt::UniqueConnection);
  }

  lastViewName = name;
}

void SyncController::pageChanged() {
  nh_log("SyncController::pageChanged()");

  if (syncDisabled) {
    return;
  }

  Settings *settings = Settings::getInstance();

  if (!settings->isEnabled(contentId))
    return;

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();
  if (name == "ReadingView") {
    queue->updateReadProgress(contentId);
  }

  int syncDaily = settings->getSyncDaily();

  if (timer != nullptr && (PowerTimer__timeRemaining(timer) <= 0 || syncDaily != lastSyncDaily)) {
    timer->deleteLater();
    timer = nullptr;
  }

  lastSyncDaily = syncDaily;

  if (timer == nullptr && syncDaily >= 0 && queue->checkThreshold(contentId, 1)) {
    timer = construct_PowerTimer("NickelHardcover-alarm", this);

    QDateTime time = QDateTime::currentDateTime();
    if (time.time().hour() >= syncDaily) {
      time = time.addDays(1);
    }
    time.setTime(QTime(syncDaily, 0));
    nh_log("Setting alarm for %s", qPrintable(time.toString()));

    PowerTimer__fireAt(timer, time);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(alarm()));
  }

  if (!queue->failed && ((queue->getReadProgress(contentId) == 100 && settings->getLastProgress(contentId) != 100) ||
                         queue->checkThreshold(contentId, settings->getPageThreshold()))) {
    nh_log("Triggered threshold auto-sync");
    queue->run(contentId);
  }
}

void SyncController::alarm() {
  nh_log("SyncController::alarm()");

  timer->deleteLater();
  timer = nullptr;
  queue->runAll();
}

int SyncController::getReadProgress() { return queue->getReadProgress(contentId); }

void SyncController::clearReadProgress() { queue->clearReadProgress(contentId); }

void SyncController::manualSync() {
  nh_log("SyncController::manualSync()");
  queue->updateReadProgress(contentId);
  queue->run(contentId, true);
}

QDateTime SyncController::getAlarm() {
  if (timer != nullptr && PowerTimer__timeRemaining(timer) > 0) {
    return QDateTime::currentDateTime().addMSecs(PowerTimer__timeRemaining(timer));
  }

  return QDateTime();
}
