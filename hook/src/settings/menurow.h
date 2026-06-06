#pragma once

#include <QLabel>
#include <QVariant>
#include <QWidget>

#include "../menucontroller.h"
#include "../nickelhardcover.h"

enum MenuRowType {
  Tap,
  Menu,
  Dialog,
};

class MenuRow : public QWidget {
  Q_OBJECT

public:
  MenuRow(QString heading, MenuRowType type, QList<Item> menuItems, QList<Item> dialogItems, QVariant defaultValue,
          QWidget *parent = nullptr);

  static QVariant OPEN_DIALOG;

public Q_SLOTS:
  void tapped();
  void up();
  void down();
  void accept();
  void menuTriggered(QAction *action);

Q_SIGNALS:
  void triggered(QVariant value);

private:
  MenuRowType type;
  QLabel *label = nullptr;
  QLabel *dialogLabel = nullptr;
  QList<Item> menuItems;
  QList<Item> dialogItems;
  int index = 0;
  Item item;

  void showMenu();
  void showDialog();
  void setItem(Item item);
};
