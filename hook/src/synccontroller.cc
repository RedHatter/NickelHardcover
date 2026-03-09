#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include <stdlib.h>

#include "cli.h"
#include "files.h"
#include "synccontroller.h"

SyncController *SyncController::instance;

SyncController *SyncController::getInstance() {
  if (!instance) {
    instance = new SyncController();
  };

  return instance;
};

SyncController::SyncController(QObject *parent) : QObject(parent) {
  config = new QSettings(Files::config, QSettings::IniFormat);
  settings = new QSettings(Files::settings, QSettings::IniFormat);
  network = new QNetworkAccessManager();

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QWidget *window = cv->window();
  inProgress = new QLabel(window);
  inProgress->setPixmap(QPixmap(Files::icon));
  inProgress->resize(90, 90);
  inProgress->move(window->width() - 144, window->height() - 144);
};

void SyncController::setContentId(QString value) {
  nh_log("SyncController::setContentId(%s)", qPrintable(value));

  contentId = value;
  key = value.replace('/', '-').replace('\\', '-');
}

QString SyncController::getContentId() { return contentId; }

bool SyncController::isEnabled() {
  return settings->value(key + "/enabled", config->value("auto_sync_default", false).toBool()).toBool();
}

void SyncController::setEnabled(bool value) { settings->setValue(key + "/enabled", value); }

void SyncController::setLinkedBook(QString value) {
  if (value.isEmpty()) {
    settings->remove(key + "/linkedbook");
  } else {
    settings->setValue(key + "/linkedbook", value);
  }
}

QString SyncController::getLinkedBook() { return settings->value(key + "/linkedbook").toString(); }

void SyncController::setLastProgress(int value) { settings->setValue(key + "/progress", value); }

int SyncController::getLastProgress() { return settings->value(key + "/progress", 0).toInt(); }

void SyncController::setLastSynced(QString value) { settings->setValue(key + "/lastSynced", value); }

QString SyncController::getLastSynced() { return settings->value(key + "/lastSynced").toString(); }

void SyncController::currentViewIndexChanged(int index) {
  if (index < 0)
    return;

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();
  nh_log("Current view changed to %s last view was %s", qPrintable(name), qPrintable(lastViewName));

  currentViewChanged(name);

  if (name == "ReadingView" && lastViewName != "ReadingView") {
    config->sync();
    settings->sync();
  }

  if (lastViewName == "ReadingView" && name != "N3Dialog" && isEnabled()) {
    QVariant syncOnClose = config->value("sync_on_close", "always");
    int onCloseThreshold = syncOnClose.toInt();
    bool enableOnClose = false;

    if (onCloseThreshold > 0 && onCloseThreshold < 100) {
      enableOnClose = true;
    } else if (syncOnClose.toString() == "always") {
      enableOnClose = true;
      onCloseThreshold = 1;
    }

    int lastProgress = getLastProgress();
    if (enableOnClose && abs(lastProgress - queue[contentId]) >= onCloseThreshold) {
      prepare(false);
    } else {
      nh_log("Reading progress is %d%% with a last synced progress of %d%% and a on close threshold of %d%%. Skipping "
             "update",
             queue[contentId], lastProgress, onCloseThreshold);
    }
  }

  if (name == "ReadingView") {
    queue[contentId] = ReadingView__getCalculatedReadProgress(cv);
    if (queue[contentId] == 0) {
      queue[contentId] = 1;
    }

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
    queue[contentId] = ReadingView__getCalculatedReadProgress(cv);
    if (queue[contentId] == 0) {
      queue[contentId] = 1;
    }
  }

  if (!isEnabled())
    return;

  int lastProgress = getLastProgress();

  int syncDaily = config->value("sync_daily").toInt();
  if (syncDaily > 24) {
    syncDaily = 24;
  }

  if (timer != nullptr && (!PowerTimer__isActive(timer) || syncDaily != lastSyncDaily)) {
    timer->deleteLater();
    timer = nullptr;
  }

  lastSyncDaily = syncDaily;

  if (timer == nullptr && syncDaily > 0 && queue[contentId] != lastProgress) {
    timer = reinterpret_cast<PowerTimer *>(calloc(1, 128));
    PowerTimer__constructor(timer, "NickelHardcover-alarm", this);

    QDateTime time = QDateTime::currentDateTime();
    if (time.time().hour() >= syncDaily) {
      time = time.addDays(1);
    }
    time.setTime(QTime(syncDaily, 0));
    nh_log("Setting alarm for %s %d %d", qPrintable(time.toString()), syncDaily, time.time().hour());

    PowerTimer__fireAt(timer, time);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(alarm()));
  }

  int threshold = config->value("threshold", 20).toInt();
  if (threshold < 1) {
    threshold = 1;
  } else if (threshold > 100) {
    threshold = 100;
  }

  if (!((queue[contentId] == 100 && lastProgress != 100) || abs(lastProgress - queue[contentId]) >= threshold)) {
    nh_log("Reading progress is %d%% with a last synced progress of %d%% and a threshold of %d%%. Skipping update",
           queue[contentId], lastProgress, threshold);
    return;
  }

  prepare(false);
}

void SyncController::alarm() {
  nh_log("SyncController::alarm()");

  timer->deleteLater();
  QObject::connect(this, &SyncController::finished, this, &SyncController::next, Qt::UniqueConnection);

  QHashIterator<QString, int> i(queue);
  if (i.hasNext()) {
    i.next();
    setContentId(i.key());

    if (isEnabled()) {
      prepare(false);
    } else {
      next();
    }
  }
}

void SyncController::next() {
  queue.remove(contentId);

  QHashIterator<QString, int> i(queue);
  if (i.hasNext()) {
    i.next();
    setContentId(i.key());

    if (isEnabled()) {
      prepare(false);
    } else {
      next();
    }
  } else {
    QObject::disconnect(this, &SyncController::finished, this, &SyncController::next);
  }
}

void SyncController::prepare(bool manual) {
  if (manual) {
    dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__showCloseButton(dialog, false);
    dialog->open();
  }

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    run();
  } else {
    if (manual) {
      if (dialog) {
        ConfirmationDialog__setText(dialog, "Connecting to wifi...");
      }

      WirelessWorkflowManager__connectWireless(wfm, false, false);
    } else {
      WirelessWorkflowManager__connectWirelessSilently(wfm);
    }

    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()), Qt::UniqueConnection);
  }
}

void SyncController::networkConnected() {
  sender()->disconnect(this);
  run();
}

void SyncController::run() {
  if (contentId == nullptr) {
    closeDialog();
    nh_log("Error: attempted to sync with null contentId");
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "`contentId` is null. This should not be possible.");
    return;
  }

  if (dialog) {
    ConfirmationDialog__setText(dialog, "Syncing with Hardcover.app...");
  }

  inProgress->show();

  setLastProgress(queue[contentId]);

  if (queue[contentId] == 100) {
    setEnabled(false);
  }

  CLI *cli = new CLI(this);
  cli->update(queue[contentId]);
  QObject::connect(cli, &CLI::success, this, &SyncController::success);
  QObject::connect(cli, &CLI::failure, this, &SyncController::closeDialog);
  QObject::connect(cli, &CLI::failure, this, &SyncController::finished);
}

void SyncController::success() {
  inProgress->hide();
  setLastSynced(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  finished();

  if (dialog == nullptr)
    return;

  ConfirmationDialog__setText(dialog, "Success!");
  QTimer::singleShot(800, this, &SyncController::closeDialog);
}

void SyncController::closeDialog() {
  inProgress->hide();

  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
