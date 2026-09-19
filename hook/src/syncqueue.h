#pragma once

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "cli.h"
#include "nickelhardcover.h"

class SyncQueue : public QObject {
  Q_OBJECT

public:
  SyncQueue(QObject *parent = nullptr);

  const QHash<QString, int> &getProgress() const { return progress; }

  void clearReadProgress(const QString &contentId) { progress.remove(contentId); }

  void updateReadProgress(const QString &contentId);
  bool checkThreshold(const QString &contentId, int threshold) const;

  void runAll();
  void run(const QString &contentId, bool manual = false);

  bool failed = false;
  bool running = false;

public Q_SLOTS:
  void networkConnected();

Q_SIGNALS:
  void finished();

private:
  void prepareNext();
  void success();
  void failure(CLI::FailureReason reason);
  void closeDialog();

  ConfirmationDialog *dialog = nullptr;

  QString currentContentId;
  int currentProgress = 0;

  QHash<QString, int> progress;
  QSet<QString> queue;
  QSet<QString> retryQueue;
};
