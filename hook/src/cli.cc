#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

#include <NickelHook.h>

#include "cli.h"
#include "files.h"
#include "settings.h"
#include "synccontroller.h"

CLI *CLI::listJournal(int limit, int offset) {
  QStringList arguments = {"list-journal", "--limit", QString::number(limit), "--offset", QString::number(offset)};
  arguments.append(getIdentifier());
  return new CLI(arguments);
}

CLI *CLI::insertJournal(QString text, int percentage) {
  QStringList arguments = {"insert-journal", "--text", text, "--percentage", QString::number(percentage)};
  arguments.append(getIdentifier());
  return new CLI(arguments);
}

CLI *CLI::getUser() { return new CLI({"get-user"}); }

CLI *CLI::getUserBook() {
  QStringList arguments = {"get-user-book"};
  arguments.append(getIdentifier());
  return new CLI(arguments);
}

CLI *CLI::setUserBook(int status) {
  QStringList arguments = {"set-user-book", "--status", QString::number(status)};
  arguments.append(getIdentifier());
  return new CLI(arguments);
}

CLI *CLI::setUserBook(float rating, QString text, bool spoilers, bool sponsored) {
  QStringList arguments = {"set-user-book"};

  arguments.append(getIdentifier());

  if (rating > 0.0) {
    arguments.append({"--rating", QString::number(rating)});
  }

  if (!text.trimmed().isEmpty()) {
    arguments.append({"--spoilers", spoilers ? "true" : "false"});
    arguments.append({"--sponsored", sponsored ? "true" : "false"});
    arguments.append({"--text", text});
  }

  return new CLI(arguments);
}

CLI *CLI::search(QString query, int limit, int page) {
  return new CLI({"search", "--limit", QString::number(limit), "--page", QString::number(page), "--query", query});
}

CLI *CLI::update(QString contentId, int percentage, bool silent) {
  QStringList arguments = {"update", "--content-id", contentId, "--value", QString::number(percentage)};

  QString linkedBook = Settings::getInstance()->getLinkedBook(contentId);
  if (!linkedBook.isEmpty()) {
    arguments.append({"--book-id", linkedBook});
  }

  QString lastSynced = Settings::getInstance()->getLastSynced(contentId);
  if (!lastSynced.isEmpty()) {
    arguments.append({"--after", lastSynced});
  }

  return new CLI(arguments, silent);
}

QStringList CLI::getIdentifier() {
  SyncController *ctl = SyncController::getInstance();
  QString linkedBook = Settings::getInstance()->getLinkedBook(ctl->contentId);
  if (linkedBook.isEmpty()) {
    return {"--content-id", ctl->contentId};
  } else {
    return {"--book-id", linkedBook};
  }
}

CLI::CLI(QStringList arguments, bool silent, QObject *parent) : QObject(parent), arguments(arguments), silent(silent) {
  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    networkConnected();
  } else {
    MainWindowController *mwc = MainWindowController__sharedInstance();
    QObject::connect(wfm, SIGNAL(connectingFailed()), this, SLOT(connectingFailed()));

    QWidget *window = MainWindowController__currentView(mwc)->window();
    wifiIcon = new QLabel(window);
    wifiIcon->setPixmap(QPixmap(Files::wifi));
    wifiIcon->resize(90, 90);
    wifiIcon->move(window->width() - 144, window->height() - 144);
    wifiIcon->show();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CLI::connectingFailed);
    timer->setSingleShot(true);
    timer->start(30000);

    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()));

    if (silent) {
      WirelessWorkflowManager__connectWirelessSilently(wfm);
    } else {
      WirelessWorkflowManager__connectWireless(wfm, false, false);
    }
  }
}

void CLI::connectingFailed() {
  nh_log("CLI::connectingFailed()");

  if (!silent) {
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Failed to connect to WIFI.");
  }

  if (timer != nullptr) {
    timer->stop();
    timer->deleteLater();
    timer = nullptr;
  }

  if (wifiIcon != nullptr) {
    wifiIcon->deleteLater();
    wifiIcon = nullptr;
  }
  deleteLater();
  failure();
}

void CLI::networkConnected() {
  nh_log("CLI::networkConnected()");

  if (timer != nullptr) {
    timer->stop();
    timer->deleteLater();
    timer = nullptr;
  }

  if (wifiIcon != nullptr) {
    wifiIcon->deleteLater();
    wifiIcon = nullptr;
  }

  QProcess *process = new QProcess(this);
  process->start(Files::cli, arguments);
  QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CLI::processFinished);
}

void CLI::processFinished(int exitCode) {
  QProcess *process = qobject_cast<QProcess *>(sender());
  deleteLater();

  QByteArray stdout = process->readAllStandardOutput();

  int index = stdout.indexOf("BEGIN_JSON");

  QByteArray bytes = stdout;
  if (index > -1) {
    bytes = stdout.left(index);
  }

  QList<QByteArray> lines = bytes.split('\n');
  for (QByteArray &line : lines) {
    if (line.length() == 0)
      continue;

    nh_log("%s", qPrintable(line));
  }

  if (exitCode > 0) {
    QByteArray stderr = process->readAllStandardError();
    nh_log("Error from command line \"%s\"", qPrintable(stderr));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", QString(stderr));
    failure();
    return;
  }

  if (index > -1) {
    QByteArray json = stdout.right(stdout.size() - index - 10);
    response(QJsonDocument::fromJson(json).object());
  }

  success();
}
