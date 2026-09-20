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

  static CLI *listBookmarks() { return new CLI({"list-bookmarks"}); }
  static CLI *listEditions(const QString &bookId, int readingFormat, const QString &language);
  static CLI *listJournal(int limit, int offset);
  static CLI *oauthRequest() { return new CLI({"oauth-request"}); }
  static CLI *oauthSet(const QString &deviceCode);
  static CLI *insertJournal(const QString &text, int percentage, const QString &privacy);
  static CLI *updateJournal(const QString &contentId);
  static CLI *getUser(bool silent = false) { return new CLI({"get-user"}, silent); }
  static CLI *getUserBook();
  static CLI *setUserBook(int status);
  static CLI *setUserBook(float rating, const QString &text, bool spoilers, bool sponsored);
  static CLI *search(const QString &query, int limit, int page);
  static CLI *update(const QString &contentId, int percentage, bool silent = false);

public Q_SLOTS:
  void networkConnected();
  void connectingFailed();

Q_SIGNALS:
  void response(Messages message);
  void success();
  void failure(FailureReason reason);

private:
  static QStringList getIdentifier(const QString &contentId = QString());

  CLI(QStringList arguments, bool silent = false, bool icon = false, const QString &contentId = QString(), QObject *parent = nullptr);

  ~CLI();

  void checkConnected();
  void processFinished(int errorCode);
  void linkBook();
  void deleteConfig();

  void showIcon(const char *path);

  QLabel *icon = nullptr;
  QTimer *timer = nullptr;
  int counter = 0;
  QStringList arguments;
  QString contentId;
  bool silent = false;
  bool iconEnabled = false;
};
