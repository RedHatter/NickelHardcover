#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../nickelhardcover.h"

class InsertJournalContent : public QWidget {
  Q_OBJECT

public:
  static void showInsertJournalDialog();

public Q_SLOTS:
  void networkConnected();
  void commit();

Q_SIGNALS:
  void close();

private:
  InsertJournalContent(QWidget *parent = nullptr);

  void buildDialog();
};
