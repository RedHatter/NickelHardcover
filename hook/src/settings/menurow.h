#pragma once

#include <QLabel>
#include <QVariant>
#include <QWidget>

#include "../widgets/label.h"
#include "../menucontroller.h"
#include "../nickelhardcover.h"

enum class MenuRowType {
  Tap,
  Menu,
  Dialog,
};

class MenuRow : public QWidget {
  Q_OBJECT

public:
  MenuRow(const QString &heading, MenuRowType type, const QList<Item> &menuItems, const QList<Item> &dialogItems,
          const QVariant &defaultValue, QWidget *parent = nullptr);

  static QVariant OPEN_DIALOG;

  void setHeading(const QString &heading) { headingLabel->setText(heading); }

public Q_SLOTS:
  void tapped();
  void up();
  void down();
  void accept() { setItem(dialogItems.at(index)); }

Q_SIGNALS:
  void triggered(QVariant value);

private:
  MenuRowType type;
  Label *headingLabel = nullptr;
  QLabel *label = nullptr;
  QLabel *dialogLabel = nullptr;
  QList<Item> menuItems;
  QList<Item> dialogItems;
  int index = 0;
  Item item;

  void showMenu();
  void showDialog();
  void setItem(const Item &item);
  void menuTriggered(const QAction *action);
};
