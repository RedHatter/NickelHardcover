#pragma once

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "nickelhardcover.h"
#include "syncqueue.h"

struct BookInfo {
  QString title;
  QString author;
};

class SyncController : public QObject {
  Q_OBJECT

public:
  static SyncController *getInstance();

  QString contentId;
  bool notRealBook;
  QNetworkAccessManager *network = new QNetworkAccessManager(this);

  void registerBook(const QString &contentId, const QString &title, const QString &author) {
    books[contentId] = {title, author};
  }

  BookInfo getBookInfo(const QString &contentId) const { return books.value(contentId); }

  int getCurrentProgress() const { return queue->getProgress().value(contentId); }

  const QHash<QString, int> &getProgress() const { return queue->getProgress(); }

  void clearReadProgress() { queue->clearReadProgress(contentId); }

  void manualSync();
  QDateTime getAlarm() const;

  bool isEnabled() const { return !notRealBook && !queue->running; }

  void currentViewIndexChanged(int index);

public Q_SLOTS:
  void pageChanged();
  void alarm();

Q_SIGNALS:
  void currentViewChanged(const QString &name);

private:
  SyncController(QObject *parent = nullptr);

  PowerTimer *timer = nullptr;
  SyncQueue *queue = new SyncQueue(this);

  QHash<QString, BookInfo> books;

  QString lastViewName;
  int lastSyncDaily = 0;
};
