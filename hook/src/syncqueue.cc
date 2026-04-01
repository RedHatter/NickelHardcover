#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include <stdlib.h>

#include "cli.h"
#include "files.h"
#include "qglobal.h"
#include "settings.h"
#include "syncqueue.h"

SyncQueue::SyncQueue(QObject *parent) : QObject(parent) {
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *window = MainWindowController__currentView(mwc)->window();
  progressIcon = new QLabel(window);
  progressIcon->setPixmap(QPixmap(Files::icon));
  progressIcon->resize(90, 90);
  progressIcon->move(window->width() - 144, window->height() - 144);
};

void SyncQueue::updateReadProgress(QString contentId) {
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  QString name = cv->objectName();

  int progress = ReadingView__getCalculatedReadProgress(cv);
  if (progress == queue[contentId]) {
    return;
  }

  queue[contentId] = ReadingView__getCalculatedReadProgress(cv);
  if (queue[contentId] == 0) {
    queue[contentId] = 1;
  }

  nh_log("Update %s queued progress to %d%%", qPrintable(contentId), queue[contentId]);
}

int SyncQueue::getReadProgress(QString contentId) { return queue[contentId]; }

bool SyncQueue::checkThreshold(QString contentId, int threshold) {
  return threshold > 0 && queue[contentId] > 0 &&
         abs(Settings::getInstance()->getLastProgress(contentId) - queue[contentId]) >= threshold;
}

void SyncQueue::prepareNext() {
  QObject::connect(this, &SyncQueue::finished, this, &SyncQueue::prepareNext, Qt::UniqueConnection);

  QHashIterator<QString, int> i(queue);
  if (i.hasNext()) {
    i.next();

    QString contentId = i.key();
    if (Settings::getInstance()->isEnabled(contentId)) {
      run(contentId);
    } else {
      nh_log("Removing %s from queue and skipping", qPrintable(contentId));
      queue.remove(contentId);
      prepareNext();
    }
  } else {
    nh_log("No more items in queue");
    QObject::disconnect(this, &SyncQueue::finished, this, &SyncQueue::prepareNext);
  }
}

void SyncQueue::run(QString contentId, bool manual) {
  if (queue[contentId] == 0) {
    nh_log("Attempted to sync %s with no saved reading progress", qPrintable(contentId));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Attempted to sync with no saved reading progress");
    return;
  }

  this->contentId = contentId;

  if (manual) {
    dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__showCloseButton(dialog, false);
    ConfirmationDialog__setText(dialog, "Syncing with Hardcover.app...");
    dialog->open();
  }

  progressIcon->show();

  if (queue[contentId] == 100) {
    Settings::getInstance()->setEnabled(contentId, false);
  }

  Settings::getInstance()->setLastProgress(contentId, queue[contentId]);

  CLI *cli = CLI::update(contentId, queue[contentId], !manual);
  queue.remove(contentId);
  QObject::connect(cli, &CLI::success, this, &SyncQueue::success);
  QObject::connect(cli, &CLI::failure, this, &SyncQueue::closeDialog);
  QObject::connect(cli, &CLI::failure, this, &SyncQueue::finished);
  queue.remove(contentId);
}

void SyncQueue::success() {
  progressIcon->hide();
  Settings::getInstance()->setLastSynced(contentId, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
  finished();

  if (dialog == nullptr)
    return;

  ConfirmationDialog__setText(dialog, "Success!");
  QTimer::singleShot(800, this, &SyncQueue::closeDialog);
}

void SyncQueue::closeDialog() {
  progressIcon->hide();

  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
