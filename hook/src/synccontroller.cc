#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include "cli.h"
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

  if (lastViewName == "ReadingView" && Settings::getInstance()->isEnabled(contentId)) {
    if (queue->checkThreshold(contentId, Settings::getInstance()->getCloseThreshold())) {
      nh_log("Triggered on close auto-sync");
      queue->run(contentId);
    }
  }

  if (name == "ReadingView") {
    queue->updateReadProgress(contentId);

    QObject::connect(cv, SIGNAL(pageChanged(int)), this, SLOT(pageChanged()), Qt::UniqueConnection);
  }

  lastViewName = name;
}

void SyncController::pageChanged() {
  nh_log("SyncController::pageChanged()");

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();
  if (name == "ReadingView") {
    queue->updateReadProgress(contentId);
  }

  if (!Settings::getInstance()->isEnabled(contentId))
    return;

  int syncDaily = Settings::getInstance()->getSyncDaily();

  if (timer != nullptr && (PowerTimer__timeRemaining(timer) <= 0 || syncDaily != lastSyncDaily)) {
    timer->deleteLater();
    timer = nullptr;
  }

  lastSyncDaily = syncDaily;

  if (timer == nullptr && syncDaily > 0 && queue->checkThreshold(contentId, 1)) {
    timer = reinterpret_cast<PowerTimer *>(calloc(1, 128));
    PowerTimer__constructor(timer, "NickelHardcover-alarm", this);

    QDateTime time = QDateTime::currentDateTime();
    if (time.time().hour() >= syncDaily) {
      time = time.addDays(1);
    }
    time.setTime(QTime(syncDaily, 0));
    nh_log("Setting alarm for %s", qPrintable(time.toString()));

    PowerTimer__fireAt(timer, time);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(alarm()));
  }

  if ((queue->getReadProgress(contentId) == 100 && Settings::getInstance()->getLastProgress(contentId) != 100) ||
      queue->checkThreshold(contentId, Settings::getInstance()->getPageThreshold())) {
    nh_log("Triggered threshold auto-sync");
    queue->run(contentId);
  }
}

void SyncController::alarm() {
  nh_log("SyncController::alarm()");

  timer->deleteLater();
  timer = nullptr;
  queue->prepareNext();
}

int SyncController::getReadProgress() { return queue->getReadProgress(contentId); }

void SyncController::manualSync() {
  nh_log("SyncController::manualSync()");
  queue->run(contentId, true);
}

QDateTime SyncController::getAlarm() {
  if (timer != nullptr && PowerTimer__timeRemaining(timer) > 0) {
    return QDateTime::currentDateTime().addMSecs(PowerTimer__timeRemaining(timer));
  }

  return QDateTime();
}
