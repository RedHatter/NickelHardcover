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
  QSettings config(Files::config, QSettings::IniFormat);
  enabledDefault = config.value("auto_sync_default", false).toBool();
  threshold = config.value("threshold", 5).toInt();
  if (threshold < 1) {
    threshold = 1;
  } else if (threshold > 100) {
    threshold = 100;
  }

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
  contentId = value;
  key = value.replace('/', '-').replace('\\', '-');
}

QString SyncController::getContentId() { return contentId; }

bool SyncController::isEnabled() { return settings->value(key + "/enabled", enabledDefault).toBool(); }

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
  nh_log("Current view changed to %s", qPrintable(name));

  currentViewChanged(name);

  if (name == "ReadingView") {
    QObject::connect(cv, SIGNAL(pageChanged(int)), this, SLOT(pageChanged()), Qt::UniqueConnection);
  }
}

void SyncController::pageChanged() {
  if (!isEnabled())
    return;

  prepare(false);
}

void SyncController::prepare(bool manual) {
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();

  if (name != "ReadingView") {
    nh_log("Error: attempted to sync while current view is %s", qPrintable(name));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Can only update book progress while a book is open");
    return;
  }

  if (manual) {
    dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__showCloseButton(dialog, false);
    dialog->open();
  }

  percentage = ReadingView__getCalculatedReadProgress(cv);
  if (percentage == 0) {
    percentage = 1;
  }

  int lastProgress = getLastProgress();
  if (!manual && (percentage != 100 || lastProgress == 100) && abs(lastProgress - percentage) < threshold) {
    nh_log("Reading progress is %d%% with a last synced progress of %d%% "
           "and a threshold of %d%%. Skipping update",
           percentage, lastProgress, threshold);
    return;
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

  setLastProgress(percentage);

  if (percentage == 100) {
    setEnabled(false);
  }

  CLI *cli = new CLI(this);
  cli->update(percentage);
  QObject::connect(cli, &CLI::success, this, &SyncController::success);
  QObject::connect(cli, &CLI::failure, this, &SyncController::closeDialog);
}

void SyncController::success() {
  inProgress->hide();
  setLastSynced(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

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
