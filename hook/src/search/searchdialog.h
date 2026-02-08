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

Q_SIGNALS:
  void tapped(QString id);

private:
  SearchDialogContent(QString query, QWidget *parent = nullptr);

  QVBoxLayout *results;
  PagingFooter *footer;

  TouchLineEdit *lineEdit;
  KeyboardFrame *keyboard;

  void clear();
  void finished(int exitCode);

  void search();
};
