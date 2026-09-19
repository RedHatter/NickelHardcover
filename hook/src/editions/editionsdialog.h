#pragma once

#include <QJsonArray>

#include "../messages.h"
#include "../nickelhardcover.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class EditionsDialog : public Dialog {
  Q_OBJECT

public:
  static EditionsDialog *show(const QString &bookId) { return new EditionsDialog(bookId); }

public Q_SLOTS:
  void showLangMenu();

Q_SIGNALS:
  void closed();
  void selected(QString id);

private:
  EditionsDialog(const QString &bookId);

  void request();

  void readingFormatChanged(const QVariant &value);
  void langTriggered(const QAction *action);
  void requestPage(int index);
  void response(const Messages &messages);

  N3ButtonLabel *langButton = nullptr;
  PagedStack *pages = nullptr;

  QString bookId = "";
  QVariant readingFormat = 4;
  QString lang = "";

  int offset = 0;
  Optional<EditionList> editionList;
};
