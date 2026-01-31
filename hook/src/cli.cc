#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QProcess>
#include <QSettings>
#include <QTimer>

#include "cli.h"
#include "settings.h"

CLI *CLI::instance;

CLI *CLI::getInstance() {
  if (!instance) {
    instance = new CLI();
  };

  return instance;
};

CLI::CLI(QObject *parent) : QObject(parent) {
  QSettings config(PATH + "config.ini", QSettings::IniFormat);
  frequency = config.value("frequency", 15).toInt();
  if (frequency < 5) {
    frequency = 5;
  }

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QWidget *window = cv->window();
  inProgress = new QLabel(window);
  inProgress->setPixmap(QPixmap(PATH + "sync.png"));
  inProgress->resize(48, 48);
  inProgress->move(window->width() - 96, window->height() - 96);
};

void CLI::currentViewChanged(int index) {
  if (index < 0)
    return;

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();
  nh_log("Current view changed to %s", qPrintable(name));

  if (name == "ReadingView") {
    timestamp = QDateTime::currentMSecsSinceEpoch();
    QObject::connect(cv, SIGNAL(pageChanged(int)), this, SLOT(pageChanged()), Qt::UniqueConnection);
  }
}

void CLI::pageChanged() {
  if (!Settings::getInstance()->isEnabled())
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

void CLI::prepare(bool silent) {
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
  contentId = Settings::getInstance()->getContentId();
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

void CLI::networkConnected() {
  sender()->disconnect(this);
  run();
}

void CLI::run() {
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

  QProcess *process = new QProcess();
  process->start(PATH + "cli", {contentId, QString::number(percent)});
  QObject::connect(process, &QProcess::readyReadStandardOutput, this, &CLI::readyReadStandardOutput);
  QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CLI::finished);
}

void CLI::logLines(QByteArray msg) {
  QList<QByteArray> lines = msg.split('\n');
  for (QByteArray &line : lines) {
    if (line.length() == 0)
      continue;

    nh_log("%s", qPrintable(line));
  }
}

void CLI::readyReadStandardOutput() {
  QProcess *process = qobject_cast<QProcess *>(sender());
  logLines(process->readAllStandardOutput());
}

void CLI::finished(int exitCode) {
  sender()->deleteLater();
  inProgress->hide();

  nh_log("CLI finished with exit code %d", exitCode);

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
