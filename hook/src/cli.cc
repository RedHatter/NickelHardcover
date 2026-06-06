#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

#include <NickelHook.h>

#include "cli.h"
#include "files.h"
#include "search/searchdialog.h"
#include "settings.h"
#include "synccontroller.h"

CLI *CLI::listBookmarks(Options options) { return new CLI({"list-bookmarks"}, options); }

CLI *CLI::listEditions(QString bookId, int readingFormat, QString language, Options options) {
  QStringList arguments = {"list-editions", "--book-id", bookId};

  if (readingFormat != 0) {
    arguments.append({"--reading-format", QString::number(readingFormat)});
  }

  if (!language.isEmpty()) {
    arguments.append({"--language", language});
  }

  return new CLI(arguments, options);
}

CLI *CLI::listJournal(int limit, int offset, Options options) {
  QStringList arguments = {"list-journal", "--limit", QString::number(limit), "--offset", QString::number(offset)};
  arguments.append(getIdentifier(options));
  return new CLI(arguments, options);
}

CLI *CLI::insertJournal(QString text, int percentage, QString privacy, Options options) {
  QStringList arguments = {"insert-journal", "--text", text, "--percentage", QString::number(percentage),
                           "--privacy",      privacy};
  arguments.append(getIdentifier(options));
  return new CLI(arguments, options);
}

CLI *CLI::updateJournal(Options options) {
  QStringList arguments = {"update-journal"};
  arguments.append(getIdentifier(options));
  return new CLI(arguments, options);
}

CLI *CLI::getUser(Options options) { return new CLI({"get-user"}, options); }

CLI *CLI::getUserBook(Options options) {
  QStringList arguments = {"get-user-book"};
  arguments.append(getIdentifier(options));
  return new CLI(arguments, options);
}

CLI *CLI::setUserBook(int status, Options options) {
  QStringList arguments = {"set-user-book", "--status", QString::number(status)};
  arguments.append(getIdentifier(options));
  return new CLI(arguments, options);
}

CLI *CLI::setUserBook(float rating, QString text, bool spoilers, bool sponsored, Options options) {
  QStringList arguments = {"set-user-book"};

  arguments.append(getIdentifier(options));

  if (rating > 0.0) {
    arguments.append({"--rating", QString::number(rating)});
  }

  if (!text.trimmed().isEmpty()) {
    arguments.append({"--spoilers", spoilers ? "true" : "false"});
    arguments.append({"--sponsored", sponsored ? "true" : "false"});
    arguments.append({"--text", text});
  }

  return new CLI(arguments, options);
}

CLI *CLI::search(QString query, int limit, int page, Options options) {
  return new CLI({"search", "--limit", QString::number(limit), "--page", QString::number(page), "--query", query},
                 options);
}

CLI *CLI::update(int percentage, Options options) {
  QStringList arguments = {"update", "--value", QString::number(percentage)};
  arguments.append(getIdentifier(options));

  return new CLI(arguments, options);
}

QStringList CLI::getIdentifier(Options options) {
  QString contentId = options.contentId.isEmpty() ? SyncController::getInstance()->contentId : options.contentId;
  QStringList identifiers = {"--content-id", contentId};

  QString linkedId = Settings::getInstance()->getLinkedId(contentId);
  if (!linkedId.isEmpty()) {
    identifiers.append({"--linked-id", linkedId});
  }

  return identifiers;
}

CLI::CLI(QStringList arguments, Options options, QObject *parent)
    : QObject(parent), arguments(arguments), options(options) {

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
    QTimer::singleShot(0, this, [options] {
      WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();

      if (options.silent) {
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

void CLI::connectingFailed() {
  nh_log("CLI::connectingFailed()");

  if (!options.silent) {
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

  if (options.icon) {
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
      QString contentId = options.contentId.isEmpty() ? SyncController::getInstance()->contentId : options.contentId;
      ConfirmationDialog__setAcceptButtonText(
          dialog, Settings::getInstance()->getLinkedId(contentId).isEmpty() ? "Link book" : "Unlink book");
      ConfirmationDialog__setRejectButtonText(dialog, "Cancel");
      ConfirmationDialog__setTitle(dialog, "Hardcover.app");
      ConfirmationDialog__setText(dialog, obj.value("message").toString());

      QObject::connect(dialog, &QDialog::accepted, this, &CLI::linkBook);
      QObject::connect(dialog, &QDialog::rejected, this, &CLI::deleteLater);
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
  QString contentId = options.contentId.isEmpty() ? ctl->contentId : options.contentId;

  if (Settings::getInstance()->getLinkedId(contentId).isEmpty()) {
    SearchDialog::show(contentId, options.query.isEmpty() ? ctl->title + " " + ctl->author : options.query);
  } else {
    Settings::getInstance()->setLinkedId(contentId, QString());
  }

  deleteLater();
}
