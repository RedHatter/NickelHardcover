#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

#include <NickelHook.h>

#include "cli.h"
#include "files.h"
#include "messages.h"
#include "nickelhardcover.h"
#include "search/searchdialog.h"
#include "signin/signindialog.h"
#include "settings.h"
#include "synccontroller.h"

CLI *CLI::listEditions(const QString &bookId, int readingFormat, const QString &language) {
  QStringList arguments = {"list-editions", "--book-id", bookId};

  if (readingFormat != 0) {
    arguments.append({"--reading-format", QString::number(readingFormat)});
  }

  if (!language.isEmpty()) {
    arguments.append({"--language", language});
  }

  return new CLI(arguments);
}

CLI *CLI::listJournal(int limit, int offset) {
  QStringList arguments = {"list-journal", "--limit", QString::number(limit), "--offset", QString::number(offset)};
  arguments.append(getIdentifier());
  return new CLI(arguments);
}

CLI *CLI::oauthSet(const QString &deviceCode) {
  return new CLI({"oauth-set", "--device-code", deviceCode});
}

CLI *CLI::insertJournal(const QString &text, int percentage, const QString &privacy) {
  QStringList arguments = {"insert-journal", "--text", text, "--percentage", QString::number(percentage),
                           "--privacy",      privacy};
  arguments.append(getIdentifier());
  return new CLI(arguments);
}

CLI *CLI::updateJournal(const QString &contentId) {
  QStringList arguments = {"update-journal"};
  arguments.append(getIdentifier(contentId));
  return new CLI(arguments, false, true, contentId);
}

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

CLI *CLI::setUserBook(float rating, const QString &text, bool spoilers, bool sponsored) {
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

CLI *CLI::search(const QString &query, int limit, int page) {
  return new CLI({"search", "--limit", QString::number(limit), "--page", QString::number(page), "--query", query});
}

CLI *CLI::update(const QString &contentId, int percentage, bool silent) {
  QStringList arguments = {"update", "--value", QString::number(percentage)};
  arguments.append(getIdentifier(contentId));

  return new CLI(arguments, silent, true, contentId);
}

QStringList CLI::getIdentifier(const QString &contentId) {
  QString id = contentId.isEmpty() ? SyncController::getInstance()->contentId : contentId;
  QStringList identifiers = {"--content-id", id};

  QString linkedId = Settings::getInstance()->getLinkedId(id);
  if (!linkedId.isEmpty()) {
    identifiers.append({"--linked-id", linkedId});
  }

  return identifiers;
}

CLI::CLI(QStringList arguments, bool silent, bool icon, const QString &contentId, QObject *parent)
    : QObject(parent), arguments(arguments),
      contentId(contentId.isEmpty() ? SyncController::getInstance()->contentId : contentId), silent(silent),
      iconEnabled(icon) {
  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    networkConnected();
  } else {
    QObject::connect(wfm, SIGNAL(connectingFailed()), this, SLOT(connectingFailed()));

    showIcon(Files::wifi);

    // The `networkConnected` signal can be unreliable so we also poll the
    // connection status every second for 30 seconds
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CLI::checkConnected);
    timer->start(1000);

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
  if (icon != nullptr) {
    icon->deleteLater();
  }
}

void CLI::checkConnected() {
  WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

  if (WirelessWorkflowManager__isInternetAccessible(wfm)) {
    networkConnected();
  }

  counter++;

  if (counter > 30) {
    connectingFailed();
  }
}

void CLI::connectingFailed() {
  nh_log("CLI::connectingFailed()");

  if (!silent) {
    showBookErrorDialog(contentId, "Failed to connect to WIFI.");
  }

  if (timer != nullptr) {
    timer->stop();
    timer->deleteLater();
    timer = nullptr;
  }

  showIcon(Files::error);

  failure(FailureReason::Network);
  QTimer::singleShot(800, this, &CLI::deleteLater);
}

void CLI::showIcon(const char *path) {
  if (icon == nullptr) {
    MainWindowController *mwc = MainWindowController__sharedInstance();
    QWidget *window = MainWindowController__currentView(mwc)->window();
    icon = new QLabel(window);
    icon->resize(90, 90);
    icon->move(window->width() - 144, window->height() - 144);
  }

  icon->setPixmap(QPixmap(path));
  icon->show();
}

void CLI::networkConnected() {
  nh_log("CLI::networkConnected()");

  WirelessManager *wm = WirelessManager__sharedInstance();
  QObject::disconnect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()));

  if (iconEnabled) {
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

  QList<QByteArray> lines = process->readAllStandardOutput().split('\n');
  for (QByteArray &line : lines) {
    if (line.length() == 0)
      continue;

    Messages msg = Messages::fromJson(QJsonDocument::fromJson(line).object());

    switch (msg.kind) {
    case Messages::Kind::Unknown:
      nh_log("%s", qPrintable(line));
      break;

    case Messages::Kind::Log:
      nh_log("%s", qPrintable(msg.log->message));
      break;

    case Messages::Kind::Error:
      if (msg.error->error_code == "Unauthorized") {
        SignInDialog::show();
        failure(FailureReason::Unauthorized);
        return;
      } else if (msg.error->error_code == "BookNotFound") {
        QString message = msg.error->message;
        nh_log("%s", qPrintable(message));

        ConfirmationDialog *dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
        ConfirmationDialog__setAcceptButtonText(
            dialog,
            Settings::getInstance()->getLinkedId(contentId).isEmpty() ? "Link book" : "Unlink book");
        ConfirmationDialog__setRejectButtonText(dialog, "Cancel");
        ConfirmationDialog__setTitle(dialog, "Hardcover.app");
        ConfirmationDialog__setText(dialog, describeBookError(contentId, message));

        QObject::connect(dialog, &QDialog::accepted, this, &CLI::linkBook);
        QObject::connect(dialog, &QDialog::rejected, this, &CLI::deleteLater);
        dialog->open();

        failure(FailureReason::BookNotFound);
        return;
      } else {
        nh_log("Error from command line \"%s\"", qPrintable(msg.error->message));
        showBookErrorDialog(contentId, msg.error->message);
        failure(FailureReason::Error);
        deleteLater();
        return;
      }

      break;

    default:
      response(msg);

      break;
    }
  }

  if (exitCode == 0) {
    success();
  } else {
    showBookErrorDialog(contentId, "Encountered an unexpected error. Please report this.");
    failure(FailureReason::Error);
  }

  deleteLater();
}

void CLI::linkBook() {
  nh_log("CLI::linkBook()");

  if (Settings::getInstance()->getLinkedId(contentId).isEmpty()) {
    SearchDialog::show(contentId);
  } else {
    Settings::getInstance()->setLinkedId(contentId, QString());
  }

  deleteLater();
}
