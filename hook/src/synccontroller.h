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
  void setContentId(QString value);

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
  void networkConnected();
  void success();
  void closeDialog();

Q_SIGNALS:
  void currentViewChanged(QString name);

private:
  SyncController(QObject *parent = nullptr);

  static SyncController *instance;

  QSettings *settings = nullptr;
  QLabel *inProgress;
  ConfirmationDialog *dialog = nullptr;

  QString contentId = nullptr;
  QString key = nullptr;
  int percentage = 0;

  bool enabledDefault;
  int threshold;

  void run();
  void logLines(QByteArray msg);
};
