#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "nickelhardcover.h"

class SyncController : public QObject {
  Q_OBJECT

public:
  static SyncController *getInstance();

  void prepare(bool silent);

  QString getContentId();
  void setContentId(QString value, bool temp = false);

  bool isEnabled();
  void setEnabled(bool value);

  void setLinkedBook(QString value);
  QString getLinkedBook();

  void setLastProgress(int value);
  int getLastProgress();

  void setLastSynced(QString value);
  QString getLastSynced();

  QString title;
  QString author;
  QNetworkAccessManager *network;

public Q_SLOTS:
  void currentViewIndexChanged(int index);
  void pageChanged();
  void alarm();
  void queuedFinished();
  void networkConnected();
  void success();
  void closeDialog();

Q_SIGNALS:
  void currentViewChanged(QString name);

  void finished();

private:
  SyncController(QObject *parent = nullptr);

  static SyncController *instance;

  QSettings *config = nullptr;
  QSettings *settings = nullptr;
  QLabel *inProgress;
  ConfirmationDialog *dialog = nullptr;
  PowerTimer *timer = nullptr;

  QString lastViewName = QString("");

  QString bkContentId = nullptr;
  QString contentId = nullptr;
  QString key = nullptr;
  QHash<QString, int> queue;

  int lastSyncDaily = 0;

  void nextQueued();
  void run();
  void logLines(QByteArray msg);
};
