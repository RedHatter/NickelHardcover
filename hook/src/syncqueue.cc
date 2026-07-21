#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include "settings.h"
#include "syncqueue.h"

SyncQueue::SyncQueue(QObject *parent) : QObject(parent) {
  WirelessManager *wm = WirelessManager__sharedInstance();
  QObject::connect(wm, SIGNAL(networkConnected()), this, SLOT(networkConnected()));
};

void SyncQueue::updateReadProgress(QString contentId) {
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);

  int newProgress = ReadingView__getCalculatedReadProgress(cv);
  if (newProgress == 99) {
    newProgress = 100;
  }

  if (newProgress < 2 || newProgress == progress[contentId]) {
    return;
  }

  progress[contentId] = newProgress;

  nh_log("Update %s queued progress to %d%%", qPrintable(contentId), progress[contentId]);
}

int SyncQueue::getReadProgress(QString contentId) { return progress[contentId]; }

void SyncQueue::clearReadProgress(QString contentId) { progress.remove(contentId); }

bool SyncQueue::checkThreshold(QString contentId, int threshold) {
  return threshold > 0 && progress[contentId] > 0 &&
         abs(Settings::getInstance()->getLastProgress(contentId) - progress[contentId]) >= threshold;
}

void SyncQueue::networkConnected() {
  if (retryQueue.isEmpty() || !Settings::getInstance()->isRetryOnNetwork())
    return;

  nh_log("Retrying %d items", retryQueue.size());

  foreach (const QString &value, retryQueue) {
    queue.insert(value);
  }

  retryQueue.clear();
  prepareNext();
}

void SyncQueue::runAll() {
  QHash<QString, int>::const_iterator i = progress.constBegin();
  while (i != progress.constEnd()) {
    queue.insert(i.key());
    ++i;
  }

  prepareNext();
}

void SyncQueue::prepareNext() {
  QObject::connect(this, &SyncQueue::finished, this, &SyncQueue::prepareNext, Qt::UniqueConnection);

  if (queue.isEmpty()) {
    nh_log("No more items in queue");
    QObject::disconnect(this, &SyncQueue::finished, this, &SyncQueue::prepareNext);
  } else {
    QString contentId = *queue.begin();
    queue.remove(contentId);

    nh_log("Syncing %s", qPrintable(contentId));
    run(contentId);
  }
}

void SyncQueue::run(QString contentId, bool manual) {
  retryQueue.remove(contentId);
  currentProgress = progress[contentId];

  if (currentProgress == 0) {
    nh_log("Attempted to sync %s with no saved reading progress", qPrintable(contentId));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app",
                                               "Reading progress must be at least 2% to sync with Hardcover.app");
    finished();
    return;
  }

  failed = false;
  this->currentContentId = contentId;

  if (manual) {
    dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__showCloseButton(dialog, false);
    ConfirmationDialog__setText(dialog, "Syncing with Hardcover.app...");
    dialog->open();
  }

  CLI::Options options;
  options.silent = !manual;
  options.icon = true;
  options.contentId = contentId;

  CLI *cli = CLI::update(currentProgress, options);
  QObject::connect(cli, &CLI::success, this, &SyncQueue::success);
  QObject::connect(cli, &CLI::failure, this, &SyncQueue::failure);
}

void SyncQueue::success() {
  if (currentProgress == 100) {
    Settings::getInstance()->setEnabled(currentContentId, false);
  }

  Settings::getInstance()->setLastProgress(currentContentId, currentProgress);
  progress.remove(currentContentId);

  finished();

  if (dialog == nullptr)
    return;

  ConfirmationDialog__setText(dialog, "Success!");
  QTimer::singleShot(800, this, &SyncQueue::closeDialog);
}

void SyncQueue::failure(CLI::FailureReason reason) {
  failed = true;

  if (reason == CLI::FailureReason::Network && Settings::getInstance()->isRetryOnNetwork()) {
    retryQueue.insert(currentContentId);
  } else {
    progress.remove(currentContentId);
  }

  closeDialog();
  finished();
}

void SyncQueue::closeDialog() {
  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
