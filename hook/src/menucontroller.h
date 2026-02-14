#include <QObject>
#include <QSettings>
#include <QWidgetAction>

#include "nickelhardcover.h"

class MenuController : public QWidget {
  Q_OBJECT

public:
  MenuController(QWidget *parent = nullptr);
  TouchLabel *icon;

public Q_SLOTS:
  void showMainMenu(bool checked);

  void syncNow(bool checked);
  void toggleEnabled(bool checked);
  void linkBook(bool checked);
  void review(bool checked);
  void setBookStatus(bool checked);

  void networkConnected();
  void showStatusMenu(QJsonObject doc);
  void statusSelected(QAction *action);

private:
  QWidgetAction *addMenuItem(NickelTouchMenu *menu, QString label, bool checkable = false, bool checked = false);
};
