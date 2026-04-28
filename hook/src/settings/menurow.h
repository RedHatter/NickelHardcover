#pragma once

#include <QLabel>
#include <QVariant>
#include <QWidget>

#include "../nickelhardcover.h"

enum MenuRowType {
  Tap,
  Menu,
  Dialog,
};

struct Item {
  QString text;
  QVariant value;
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

  void openMenu();
  void openDialog();
  void setItem(Item item);
};
