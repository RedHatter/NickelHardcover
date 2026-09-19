#pragma once

#include <QJsonObject>
#include <QLabel>
#include <QObject>
#include <QStringList>

#include "messages.h"

class CLI : public QObject {
  Q_OBJECT

public:
  enum FailureReason {
    Network,
    Error,
    BookNotFound,
    Unauthorized
  };

  struct Options {
    bool silent = false;
    bool icon = false;

    QString contentId = QString();
    QString query = QString();

    Options() {};

    QString getContentId() const;
    QString getQuery() const;
  };

  static CLI *listBookmarks(const Options &options = Options()) { return new CLI({"list-bookmarks"}, options); }
  static CLI *listEditions(const QString &bookId, int readingFormat, const QString &language, const Options &options = Options());
  static CLI *listJournal(int limit, int offset, const Options &options = Options());
  static CLI *oauthRequest(const Options &options = Options()) { return new CLI({"oauth-request"}, options); }
  static CLI *oauthSet(const QString &deviceCode, const Options &options = Options());
  static CLI *insertJournal(const QString &text, int percentage, const QString &privacy, const Options &options = Options());
  static CLI *updateJournal(const Options &options = Options());
  static CLI *getUser(const Options &options = Options()) { return new CLI({"get-user"}, options); }
  static CLI *getUserBook(const Options &options = Options());
  static CLI *setUserBook(int status, const Options &options = Options());
  static CLI *setUserBook(float rating, const QString &text, bool spoilers, bool sponsored, const Options &options = Options());
  static CLI *search(const QString &query, int limit, int page, const Options &options = Options());
  static CLI *update(int percentage, const Options &options = Options());

public Q_SLOTS:
  void networkConnected();
  void connectingFailed();

Q_SIGNALS:
  void response(Messages message);
  void success();
  void failure(FailureReason reason);

private:
  static QStringList getIdentifier(const Options &options);

  CLI(QStringList arguments, Options options = Options(), QObject *parent = nullptr);

  ~CLI();

  void checkConnected();
  void processFinished(int errorCode);
  void linkBook();

  void showIcon(const char *path);

  QLabel *icon = nullptr;
  QTimer *timer = nullptr;
  int counter = 0;
  QStringList arguments;
  Options options;
};
