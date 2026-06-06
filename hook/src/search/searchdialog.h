#include <QVBoxLayout>
#include <QWidget>

#include "../nickelhardcover.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class SearchDialog : public Dialog {
  Q_OBJECT

public:
  static void show(QString contentId, QString query);

  void commit() override;

public Q_SLOTS:
  void requestPage(int index);
  void response(QJsonObject doc);
  void tapped(QString id);

private:
  SearchDialog(QString contentId, QString query);

  PagedStack *pages = nullptr;
  TouchLineEdit *lineEdit = nullptr;
  QString contentId;

  void clear();
};
