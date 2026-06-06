#include <QJsonObject>
#include <QLabel>
#include <QObject>
#include <QStringList>

class CLI : public QObject {
  Q_OBJECT

public:
  struct Options {
    bool silent = false;
    bool icon = false;

    QString contentId = QString();
    QString query = QString();

    Options() {};
  };

  static CLI *listBookmarks(Options options = Options());
  static CLI *listEditions(QString bookId, int readingFormat, QString language, Options options = Options());
  static CLI *listJournal(int limit, int offset, Options options = Options());
  static CLI *insertJournal(QString text, int percentage, QString privacy, Options options = Options());
  static CLI *updateJournal(Options options = Options());
  static CLI *getUser(Options options = Options());
  static CLI *getUserBook(Options options = Options());
  static CLI *setUserBook(int status, Options options = Options());
  static CLI *setUserBook(float rating, QString text, bool spoilers, bool sponsored, Options options = Options());
  static CLI *search(QString query, int limit, int page, Options options = Options());
  static CLI *update(int percentage, Options options = Options());

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
  static QStringList getIdentifier(Options options);

  CLI(QStringList arguments, Options options = Options(), QObject *parent = nullptr);

  ~CLI();

  void showIcon(const char *path);

  QLabel *icon = nullptr;
  QTimer *timer = nullptr;
  QStringList arguments;
  Options options;
};
