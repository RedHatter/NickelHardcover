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

  void updateReadProgress(QString contentId);
  int getReadProgress(QString contentId);
  void clearReadProgress(QString contentId);
  bool checkThreshold(QString contentId, int threshold);

  void runAll();
  void run(QString contentId, bool manual = false);

  bool failed = false;

public Q_SLOTS:
  void networkConnected();
  void prepareNext();
  void success();
  void failure(CLI::FailureReason reason);
  void closeDialog();

Q_SIGNALS:
  void finished();

private:
  ConfirmationDialog *dialog = nullptr;

  QString currentContentId;
  int currentProgress = 0;

  QHash<QString, int> progress;
  QSet<QString> queue;
  QSet<QString> retryQueue;
};
