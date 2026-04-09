#pragma once

#include <QLabel>
#include <QStackedLayout>
#include <QWidget>

#include "../nickelhardcover.h"
#include "qobject.h"

class PagedStack : public QWidget {
  Q_OBJECT

public:
  PagedStack(QWidget *parent = nullptr);

  void addPage(QWidget *page);
  void clear();
  int getAvailableHeight();
  int countPages();

  int total = 0;

public Q_SLOTS:
  void next();
  void prev();

Q_SIGNALS:
  void requestPage(int index);

private:
  int current = 0;
  QLabel *label = nullptr;
  TouchLabel *nextButton = nullptr;
  TouchLabel *prevButton = nullptr;
  QStackedLayout *stack;

  void setCurrent(int value);
};

class PagedStackFilter : public QObject {
  Q_OBJECT

public:
  PagedStackFilter(PagedStack *pages);

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  PagedStack *pages = nullptr;
};
