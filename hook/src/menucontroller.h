#pragma once

#include <QObject>
#include <QSettings>
#include <QWidgetAction>

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

public Q_SLOTS:
  static NickelTouchMenu *showMenu(QList<Item> items, QWidget *anchor, int offset, bool checkable = false,
                                   bool decorated = true);

  void showMainMenu();
  void showStatusMenu(QJsonObject doc);
  void triggered(QAction *action);

private:
  void setSelected(bool selected);

  int iconHeight;
};
