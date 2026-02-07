#include <QObject>
#include <QSettings>
#include <QWidgetAction>

#include "nickelhardcover.h"

class MenuController : public QObject {
  Q_OBJECT

public:
  static MenuController *getInstance();

public Q_SLOTS:
  void showMenu(bool checked);

  void syncNow(bool checked);
  void toggleEnabled(bool checked);
  void linkBook(bool checked);
  void review(bool checked);

public:
  QWidget *buildWidget(QWidget *parent = nullptr);

private:
  MenuController(QObject *parent = nullptr);

  QWidgetAction *addMenuItem(NickelTouchMenu *menu, QString label);

  static MenuController *instance;
};
