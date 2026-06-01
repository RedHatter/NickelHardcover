#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

#include <NickelHook.h>

#include "search/searchdialog.h"
#include "cli.h"
#include "files.h"
#include "settings.h"
#include "synccontroller.h"

CLI *CLI::listJournal(int limit, int offset, bool silent, bool icon) {
  QStringList arguments = {"list-journal", "--limit", QString::number(limit), "--offset", QString::number(offset)};
  arguments.append(getIdentifier());
  return new CLI(arguments, silent, icon);
}

CLI *CLI::insertJournal(QString text, int percentage, QString privacy, bool silent, bool icon) {
  QStringList arguments = {"insert-journal", "--text", text, "--percentage", QString::number(percentage),
                           "--privacy",      privacy};
  arguments.append(getIdentifier());
  return new CLI(arguments, silent, icon);
}

CLI *CLI::getUser(bool silent, bool icon) { return new CLI({"get-user"}, silent, icon); }

CLI *CLI::getUserBook(bool silent, bool icon) {
  QStringList arguments = {"get-user-book"};
  arguments.append(getIdentifier());
  return new CLI(arguments, silent, icon);
}

CLI *CLI::setUserBook(int status, bool silent, bool icon) {
  QStringList arguments = {"set-user-book", "--status", QString::number(status)};
  arguments.append(getIdentifier());
  return new CLI(arguments, silent, icon);
}

CLI *CLI::setUserBook(float rating, QString text, bool spoilers, bool sponsored, bool silent, bool icon) {
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

  return new CLI(arguments, silent, icon);
}

CLI *CLI::search(QString query, int limit, int page, bool silent, bool icon) {
  return new CLI({"search", "--limit", QString::number(limit), "--page", QString::number(page), "--query", query},
                 silent, icon);
}

CLI *CLI::update(QString contentId, int percentage, bool silent, bool icon) {
  QStringList arguments = {"update", "--content-id", contentId, "--value", QString::number(percentage)};

  QString linkedBook = Settings::getInstance()->getLinkedBook(contentId);
  if (!linkedBook.isEmpty()) {
    arguments.append({"--book-id", linkedBook});
  }

  return new CLI(arguments, silent, icon);
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

CLI::CLI(QStringList arguments, bool silent, bool icon, QObject *parent)
    : QObject(parent), arguments(arguments), silent(silent), icon(icon) {

  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    networkConnected();
  } else {
    QObject::connect(wfm, SIGNAL(connectingFailed()), this, SLOT(connectingFailed()));

    showIcon(Files::wifi);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CLI::connectingFailed);
    timer->setSingleShot(true);
    timer->start(30000);

    WirelessManager *wm = WirelessManager__sharedInstance();
    QObject::connect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()));

    // Yield to caller so signals can be setup before a possible connectingFailed() is triggered
    QTimer::singleShot(0, this, [silent] {
      WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

      if (silent) {
        WirelessWorkflowManager__connectWirelessSilently(wfm);
      } else {
        WirelessWorkflowManager__connectWireless(wfm, false, false);
      }
    });
  }
}

CLI::~CLI() {
  if (iconLabel != nullptr) {
    iconLabel->deleteLater();
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

  deleteLater();
  failure();
}

void CLI::showIcon(const char *path) {
  if (iconLabel == nullptr) {
    MainWindowController *mwc = MainWindowController__sharedInstance();
    QWidget *window = MainWindowController__currentView(mwc)->window();
    iconLabel = new QLabel(window);
    iconLabel->resize(90, 90);
    iconLabel->move(window->width() - 144, window->height() - 144);
  }

  iconLabel->setPixmap(QPixmap(path));
  iconLabel->show();
}

void CLI::networkConnected() {
  nh_log("CLI::networkConnected()");

  if (icon) {
    showIcon(Files::icon);
  }

  if (timer != nullptr) {
    timer->stop();
    timer->deleteLater();
    timer = nullptr;
  }

  QProcess *process = new QProcess(this);
  process->start(Files::cli, arguments);
  QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CLI::processFinished);
}

void CLI::processFinished(int exitCode) {
  QProcess *process = qobject_cast<QProcess *>(sender());

  QByteArray stdout = process->readAllStandardOutput();

  int index = stdout.indexOf("BEGIN_JSON");

  QByteArray bytes = stdout;
  if (index >= 0) {
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
    deleteLater();
    return;
  }

  if (index >= 0) {
    QByteArray json = stdout.right(stdout.size() - index - 10);
    QJsonObject obj = QJsonDocument::fromJson(json).object();

    if (obj.value("error_code").toString() == "BOOK_NOT_FOUND") {
      ConfirmationDialog *dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
      ConfirmationDialog__setAcceptButtonText(
          dialog, Settings::getInstance()->getLinkedBook(SyncController::getInstance()->contentId).isEmpty()
                      ? "Link book"
                      : "Unlink book");
      ConfirmationDialog__setRejectButtonText(dialog, "Cancel");
      ConfirmationDialog__setTitle(dialog, "Hardcover.app");
      ConfirmationDialog__setText(dialog, obj.value("message").toString());

      QObject::connect(dialog, &QDialog::accepted, this, &CLI::linkBook);
      dialog->open();

      failure();
      return;
    }

    response(obj);
  }

  success();
  deleteLater();
}

void CLI::linkBook() {
  nh_log("CLI::linkBook()");

  SyncController *ctl = SyncController::getInstance();

  if (Settings::getInstance()->getLinkedBook(ctl->contentId).isEmpty()) {
    SearchDialog::show(ctl->title + " " + ctl->author);
  } else {
    Settings::getInstance()->setLinkedBook(ctl->contentId, QString());
  }

  deleteLater();
}
