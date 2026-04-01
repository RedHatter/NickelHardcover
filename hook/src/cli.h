#include <QJsonObject>
#include <QLabel>
#include <QObject>
#include <QStringList>

class CLI : public QObject {
  Q_OBJECT

public:
  static CLI *listJournal(int limit, int offset);
  static CLI *insertJournal(QString text, int percentage);
  static CLI *getUser();
  static CLI *getUserBook();
  static CLI *setUserBook(int status);
  static CLI *setUserBook(float rating, QString text, bool spoilers, bool sponsored);
  static CLI *search(QString query, int limit, int page);
  static CLI *update(QString contentId, int percentage, bool silent);

public Q_SLOTS:
  void networkConnected();
  void connectingFailed();
  void processFinished(int exitCode);

Q_SIGNALS:
  void response(QJsonObject doc);
  void success();
  void failure();

private:
  static QStringList getIdentifier();

  CLI(QStringList arguments, bool silent = false, QObject *parent = nullptr);

  QLabel *wifiIcon = nullptr;
  QStringList arguments;
  bool silent = false;
};
