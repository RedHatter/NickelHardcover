#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "nickelhardcover.h"

class SyncQueue : public QObject {
  Q_OBJECT

public:
  SyncQueue(QObject *parent = nullptr);

  void updateReadProgress(QString contentId);
  int getReadProgress(QString contentId);
  bool checkThreshold(QString contentId, int threshold);

  void run(QString contentId, bool manual = false);

  bool failed;

public Q_SLOTS:
  void prepareNext();
  void success();
  void failure();
  void closeDialog();

Q_SIGNALS:
  void finished();

private:
  ConfirmationDialog *dialog = nullptr;

  QString contentId;
  int progress;
  QHash<QString, int> queue;
};
