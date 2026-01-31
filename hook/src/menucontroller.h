#include <QObject>
#include <QSettings>

#include "nickelhardcover.h"

class MenuController : public QObject {
  Q_OBJECT

public:
  static MenuController *getInstance();

public Q_SLOTS:
  void itemSelected(int index);

public:
  void setupItems(ComboButton *button);

private:
  MenuController(QObject *parent = nullptr);

  static MenuController *instance;
};
