#include <QProcess>
#include <QJsonDocument>

#include <NickelHook.h>

#include "cli.h"
#include "files.h"
#include "synccontroller.h"

CLI::CLI(QObject *parent) : QObject(parent) {}

void CLI::getUserBook() {
  QStringList arguments = {"get-user-book"};
  arguments.append(getIdentifier());
  start(arguments);
}

void CLI::setUserBook(int status) {
  QStringList arguments = {"set-user-book", "--status", QString::number(status)};
  arguments.append(getIdentifier());
  start(arguments);
}

void CLI::setUserBook(int rating, QString text, bool spoilers, bool sponsored) {
  QStringList arguments = {"set-user-book", "--rating", QString::number(rating), "--text", text};

  arguments.append(getIdentifier());

  if (spoilers) {
    arguments.append("--spoilers");
  }

  if (sponsored) {
    arguments.append("--sponsored");
  }

  start(arguments);
}

void CLI::search(QString query, int limit, int page) {
  start({"search", "--limit", QString::number(limit), "--page", QString::number(page), "--query", query});
}

void CLI::update(int percentage) {
  SyncController *ctl = SyncController::getInstance();

  QStringList arguments = {"update", "--content-id", ctl->getContentId(), "--value", QString::number(percentage)};

  QString linkedBook = ctl->getLinkedBook();
  if (!linkedBook.isEmpty()) {
    arguments.append({"--book-id", linkedBook});
  }

  QString lastSynced = ctl->getLastSynced();
  if (!lastSynced.isEmpty()) {
    arguments.append({"--after", lastSynced});
  }

  start(arguments);
}

QStringList CLI::getIdentifier() {
  SyncController *ctl = SyncController::getInstance();
  QString linkedBook = ctl->getLinkedBook();
  if (linkedBook.isEmpty()) {
    return {"--content-id", ctl->getContentId()};
  } else {
    return {"--book-id", linkedBook};
  }
}

void CLI::start(QStringList arguments) {
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
