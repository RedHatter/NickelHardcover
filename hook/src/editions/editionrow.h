#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QJsonObject>
#include <QLabel>

#include "../messages.h"

class EditionRow : public QFrame {
  Q_OBJECT
  Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)

public:
  EditionRow(const Edition &edition, QWidget *parent = nullptr);

  QGridLayout *layout() const { return qobject_cast<QGridLayout *>(QWidget::layout()); }

  void setVerticalSpacing(int value) { layout()->setVerticalSpacing(value); }
  int verticalSpacing() const { return layout()->verticalSpacing(); }

public Q_SLOTS:
  void tapped() { selected(id); }

Q_SIGNALS:
  void selected(QString id);

private:
  QString id;
  QLabel *cover = nullptr;

  void loadCover();

  QLabel *buildCover(const Edition &edition);
};
