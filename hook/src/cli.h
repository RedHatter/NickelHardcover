#include <QJsonObject>
#include <QObject>

class CLI : public QObject {
  Q_OBJECT

public:
  CLI(QObject *parent = nullptr);

  void listJournal(int limit, int offset);
  void getUserBook();
  void setUserBook(int status);
  void setUserBook(float rating, QString text, bool spoilers, bool sponsored);
  void search(QString query, int limit, int page);
  void update(int percentage);

public Q_SLOTS:
  void processFinished(int exitCode);

Q_SIGNALS:
  void response(QJsonObject doc);
  void success();
  void failure();

private:
  QStringList getIdentifier();
  void start(QStringList arguments);
};
