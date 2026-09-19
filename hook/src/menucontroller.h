#pragma once

#include <QObject>
#include <QSettings>
#include <QWidgetAction>

#include "messages.h"
#include "nickelhardcover.h"

struct Item {
  QString text;
  QVariant value;
  bool checked;
  bool disabled;
};

class MenuController : public QWidget {
  Q_OBJECT

public:
  MenuController(int iconHeight, QWidget *parent = nullptr);
  TouchLabel *icon = nullptr;

  static NickelTouchMenu *showMenu(const QList<Item> &items, QWidget *anchor, int offset, bool checkable = false,
                                   bool decorated = true);

public Q_SLOTS:
  void showMainMenu();

private:
  void showStatusMenu(const Messages &message);
  void triggered(const QAction *action);

  void setSelected(bool selected);

  int iconHeight;
};
