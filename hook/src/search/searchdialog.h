#include <QVBoxLayout>
#include <QWidget>

#include "../nickelhardcover.h"

class SearchDialogContent : public QWidget {
  Q_OBJECT

public:
  static void showSearchDialog(QString query);

  void setKeyboardFrame(KeyboardFrame *frame);

public Q_SLOTS:
  void networkConnected();
  void commit();
  void search(int page);
  void response(QJsonObject doc);
  void tapped(QString id);

Q_SIGNALS:
  void close();

private:
  SearchDialogContent(QString query, QWidget *parent = nullptr);

  QVBoxLayout *results;
  PagingFooter *footer;

  TouchLineEdit *lineEdit;
  KeyboardFrame *keyboard;

  void clear();
};
