#include <QJsonArray>

#include "../nickelhardcover.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class EditionsDialog : public Dialog {
  Q_OBJECT

public:
  static EditionsDialog *show(QString bookId);

public Q_SLOTS:
  void readingFormatChanged(QVariant value);
  void showLangMenu();
  void langTriggered(QAction *action);
  void requestPage(int index);
  void response(QJsonObject doc);

Q_SIGNALS:
  void closed();
  void selected(QString id);

private:
  EditionsDialog(QString bookId);

  void request();

  N3ButtonLabel *langButton = nullptr;
  PagedStack *pages = nullptr;

  QString bookId = 0;
  QVariant readingFormat = 4;
  QString lang = "";

  int offset = 0;
  bool editionsInitialized = false;
  QJsonArray editions;
  QJsonArray languages;
};
