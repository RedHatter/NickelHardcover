#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../widgets/dialog.h"

class JournalDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(QJsonObject doc);
  void newEntry();
  void goToPage(int page);

private:
  JournalDialog(QWidget *parent = nullptr);

  QVector<int> *pages = new QVector<int>(1, 0);
  int currentPage = 0;

  PagingFooter *footer;
  QVBoxLayout *rows;

protected:
  void build() override;
};
