#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "nickelhardcover.h"
#include "syncqueue.h"

class SyncController : public QObject {
  Q_OBJECT

public:
  static SyncController *getInstance();

  QString title;
  QString author;
  QString contentId;
  QNetworkAccessManager *network = new QNetworkAccessManager();

  void manualSync();
  QDateTime getAlarm();

public Q_SLOTS:
  void currentViewIndexChanged(int index);
  void pageChanged();
  void alarm();

Q_SIGNALS:
  void currentViewChanged(QString name);

private:
  SyncController(QObject *parent = nullptr);

  static SyncController *instance;

  PowerTimer *timer = nullptr;
  SyncQueue *queue = new SyncQueue(this);

  QString lastViewName;
  int lastSyncDaily = 0;
};
