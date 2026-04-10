#pragma once

#include <QLabel>
#include <QStackedWidget>
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
  void afterLayout();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  int current = 0;
  QLabel *label = nullptr;
  TouchLabel *nextButton = nullptr;
  TouchLabel *prevButton = nullptr;
  QStackedWidget *stack;

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
