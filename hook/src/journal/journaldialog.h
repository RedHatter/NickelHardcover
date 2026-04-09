#include <QJsonObject>
#include <QStackedLayout>
#include <QWidget>

#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class JournalDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(QJsonObject doc);
  void newEntry();
  void requestPage(int index);

private:
  JournalDialog();

  int offset = 0;
  PagedStack *pages;
};
