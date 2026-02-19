#include <QVBoxLayout>
#include <QWidget>

#include "../nickelhardcover.h"
#include "../widgets/dialog.h"

class SearchDialog : public Dialog {
  Q_OBJECT

public:
  static void show(QString query);

  void commit() override;

public Q_SLOTS:
  void search(int page);
  void response(QJsonObject doc);
  void tapped(QString id);

private:
  SearchDialog(QString query, QWidget *parent = nullptr);

  QVBoxLayout *results;
  PagingFooter *footer;

  TouchLineEdit *lineEdit;

  void clear();

protected:
  void build() override;
};
