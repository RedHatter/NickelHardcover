#include <QJsonObject>
#include <QLabel>
#include <QObject>
#include <QStringList>

class CLI : public QObject {
  Q_OBJECT

public:
  static CLI *listJournal(int limit, int offset, bool silent = false, bool icon = false);
  static CLI *insertJournal(QString text, int percentage, QString privacy, bool silent = false, bool icon = false);
  static CLI *getUser(bool silent = false, bool icon = false);
  static CLI *getUserBook(bool silent = false, bool icon = false);
  static CLI *setUserBook(int status, bool silent = false, bool icon = false);
  static CLI *setUserBook(float rating, QString text, bool spoilers, bool sponsored, bool silent = false,
                          bool icon = false);
  static CLI *search(QString query, int limit, int page, bool silent = false, bool icon = false);
  static CLI *update(QString contentId, int percentage, bool silent = false, bool icon = false);

public Q_SLOTS:
  void networkConnected();
  void connectingFailed();
  void processFinished(int exitCode);
  void linkBook();

Q_SIGNALS:
  void response(QJsonObject doc);
  void success();
  void failure();

private:
  static QStringList getIdentifier();

  CLI(QStringList arguments, bool silent = false, bool icon = false, QObject *parent = nullptr);

  ~CLI();

  void showIcon(const char *path);

  QLabel *iconLabel = nullptr;
  QTimer *timer = nullptr;
  QStringList arguments;
  bool silent;
  bool icon;
};
