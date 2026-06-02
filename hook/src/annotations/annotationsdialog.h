#include <QJsonArray>
#include <QJsonObject>
#include <QStackedLayout>

#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class AnnotationsDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(QJsonObject doc);
  void requestPage(int index);

private:
  AnnotationsDialog();

  int offset = 0;
  bool bookmarksInitialized = false;
  QJsonArray bookmarks;
  PagedStack *pages;
};
