#include <stdlib.h>
#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QProcess>
#include <QSettings>
#include <QTimer>

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

bool SyncController::isEnabled() { return settings->value(key + "/enabled", false).toBool(); }

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

  int threshold = 5;
  int lastProgress = getLastProgress();
  if (!manual && percentage != 100 && abs(lastProgress - percentage) < threshold) {
    nh_log("Reading progress is %d%% with a last synced progress of %d%% and a threshold of %d%%. Skipping update", percentage, lastProgress, threshold);
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
    ConfirmationDialog__setText(dialog, "Updating book progress on Hardcover.app...");
  }

  inProgress->show();

  setLastProgress(percentage);

  QStringList arguments = {"update", "--content-id", contentId, "--value", QString::number(percentage)};

  QString linkedBook = getLinkedBook();
  if (!linkedBook.isEmpty()) {
    arguments.append({"--book-id", linkedBook});
  }

  QString lastSynced = getLastSynced();
  if (!lastSynced.isEmpty()) {
    arguments.append({"--after", lastSynced});
  }

  QProcess *process = new QProcess();
  process->start(Files::cli, arguments);
  QObject::connect(process, &QProcess::readyReadStandardOutput, this, &SyncController::readyReadStandardOutput);
  QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SyncController::finished);
}

void SyncController::logLines(QByteArray msg) {
  QList<QByteArray> lines = msg.split('\n');
  for (QByteArray &line : lines) {
    if (line.length() == 0)
      continue;

    nh_log("%s", qPrintable(line));
  }
}

void SyncController::readyReadStandardOutput() {
  QProcess *process = qobject_cast<QProcess *>(sender());
  logLines(process->readAllStandardOutput());
}

void SyncController::finished(int exitCode) {
  sender()->deleteLater();
  inProgress->hide();

  nh_log("cli finished with exit code %d", exitCode);

  if (exitCode != 0) {
    closeDialog();

    QProcess *process = qobject_cast<QProcess *>(sender());
    QByteArray stderr = process->readAllStandardError();
    logLines(stderr);

    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", QString(stderr));
  } else if (dialog) {
    setLastSynced(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    ConfirmationDialog__setText(dialog, "Success!");
    QTimer::singleShot(800, this, &SyncController::closeDialog);
  }
}

void SyncController::closeDialog() {
  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
