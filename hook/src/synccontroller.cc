#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QProcess>
#include <QSettings>
#include <QTimer>

#include "synccontroller.h"

const QString PATH = "/mnt/onboard/.adds/NickelHardcover/";

SyncController *SyncController::instance;

SyncController *SyncController::getInstance() {
  if (!instance) {
    instance = new SyncController();
  };

  return instance;
};

SyncController::SyncController(QObject *parent) : QObject(parent) {
  QSettings config(PATH + "config.ini", QSettings::IniFormat);
  frequency = config.value("frequency", 15).toInt();
  if (frequency < 5) {
    frequency = 5;
  }

  settings = new QSettings(PATH + "settings.ini", QSettings::IniFormat);
  network = new QNetworkAccessManager();

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QWidget *window = cv->window();
  inProgress = new QLabel(window);
  inProgress->setPixmap(QPixmap("/usr/share/NickelHardcover/icon.png"));
  inProgress->resize(48, 48);
  inProgress->move(window->width() - 96, window->height() - 96);
};

void SyncController::setContentId(QString value) {
  contentId = value;
  key = value.replace('/', '-').replace('\\', '-');
}

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

  qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

  if (timestamp == INT64_MAX) {
    timestamp = currentTimestamp;
    return;
  }

  int duration = (currentTimestamp - timestamp) / 1000 / 60;
  nh_log("Page changed. It has been %d minutes since the last sync attempt", duration);

  if (duration >= frequency) {
    prepare(true);
  }
}

void SyncController::prepare(bool silent) {
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();

  if (name != "ReadingView") {
    nh_log("Error: attempted to sync while current view is %s", qPrintable(name));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Can only update book progress when a book is open");
    return;
  }

  if (!silent) {
    dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__showCloseButton(dialog, false);
    dialog->open();
  }

  percent = ReadingView__getCalculatedReadProgressEv(cv);

  if (silent && getLastProgress() == percent) {
    nh_log("Reading progress hasn't changed skipping update");
    return;
  }

  timestamp = QDateTime::currentMSecsSinceEpoch();

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    run();
  } else {
    if (silent) {
      WirelessWorkflowManager__connectWirelessSilently(wfm);
    } else {
      if (dialog) {
        ConfirmationDialog__setText(dialog, "Connecting to wifi...");
      }

      WirelessWorkflowManager__connectWireless(wfm, false, false);
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
  if (percent == 0) {
    nh_log("Error: attempted to sync with 0 percent progress");
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Reading progress must be at least 1% in order to sync.");
    return;
  }

  if (contentId == nullptr) {
    nh_log("Error: attempted to sync with null contentId");
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "`contentId` is null. This should not be possible.");
    return;
  }

  if (dialog) {
    ConfirmationDialog__setText(dialog, "Updating book progress on Hardcover.app...");
  }

  inProgress->show();

  setLastProgress(percent);

  QProcess *process = new QProcess();
  QString linkedBook = getLinkedBook();
  if (linkedBook.isEmpty()) {
    process->start(PATH + "cli", {"update", contentId, QString::number(percent)});
  } else {
    process->start(PATH + "cli", {"update", contentId, QString::number(percent), linkedBook});
  }
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
    if (dialog) {
      dialog->close();
      dialog->deleteLater();
      dialog = nullptr;
    }

    QProcess *process = qobject_cast<QProcess *>(sender());
    QByteArray stderr = process->readAllStandardError();
    logLines(stderr);

    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", QString(stderr));
  } else if (dialog) {
    ConfirmationDialog__setText(dialog, "Success!");

    QTimer *timer = new QTimer(dialog);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, this, [this] {
      dialog->close();
      dialog->deleteLater();
      dialog = nullptr;
    });
    timer->start(800);
  }
}
