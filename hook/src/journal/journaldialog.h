#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../nickelhardcover.h"

class JournalDialogContent : public QWidget {
  Q_OBJECT

public:
  static void showJournalDialog();

public Q_SLOTS:
  void networkConnected();
  void newEntry();
  void buildContent(QJsonObject doc);
  void goToPage(int page);

Q_SIGNALS:
  void close();

private:
  JournalDialogContent(QWidget *parent = nullptr);

  QVector<int> *pages = new QVector<int>(1, 0);
  int currentPage = 0;

  PagingFooter *footer;
  QVBoxLayout *rows;
};
