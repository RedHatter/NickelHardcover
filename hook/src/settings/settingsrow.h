#pragma once

#include <QLabel>
#include <QVariant>
#include <QWidget>

enum SettingsRowType {
  Toggle,
  Menu,
  Dialog,
};

struct Item {
  QString text;
  QVariant value;
};

class SettingsRow : public QWidget {
  Q_OBJECT

public:
  SettingsRow(QString heading, SettingsRowType type, QList<Item> menuItems, QList<Item> dialogItems,
              QVariant defaultValue, bool showBorder = true, QWidget *parent = nullptr);

  static QVariant OPEN_DIALOG;

public Q_SLOTS:
  void tapped();
  void up();
  void down();
  void accept();

Q_SIGNALS:
  void triggered(QVariant value);

private:
  SettingsRowType type;
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
