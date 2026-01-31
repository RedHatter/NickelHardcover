#include <QLabel>
#include <QObject>
#include <QSettings>

#include "nickelhardcover.h"

class CLI : public QObject {
  Q_OBJECT

public:
  static CLI *getInstance();

  void prepare(bool silent);

public Q_SLOTS:
  void currentViewChanged(int index);
  void pageChanged();
  void networkConnected();
  void readyReadStandardOutput();
  void finished(int exitCode);

private:
  CLI(QObject *parent = nullptr);

  static CLI *instance;

  qint64 timestamp = INT64_MAX;
  int percent = 0;
  QString contentId = nullptr;
  int frequency;
  QLabel *inProgress;
  ConfirmationDialog *dialog = nullptr;

  void run();
  void logLines(QByteArray msg);
};
