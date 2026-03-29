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

public Q_SLOTS:
  void prepareNext();
  void success();
  void closeDialog();

Q_SIGNALS:
  void finished();

private:
  QLabel *progressIcon = nullptr;
  ConfirmationDialog *dialog = nullptr;

  QString contentId;
  QHash<QString, int> queue;
};
