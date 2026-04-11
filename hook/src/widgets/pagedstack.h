#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QWidget>

#include "../nickelhardcover.h"
#include "qobject.h"

class PagedStack : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int footerHeight READ footerHeight WRITE setFooterHeight)
  Q_PROPERTY(int footerButtonWidth READ footerButtonWidth WRITE setFooterButtonWidth)

public:
  PagedStack(QWidget *parent = nullptr);

  void addPage(QWidget *page);
  void clear();
  int getAvailableHeight();
  int countPages();
  void setTotal(int value);

  void setFooterHeight(int value);
  int footerHeight() const;

  void setFooterButtonWidth(int value);
  int footerButtonWidth() const;

  QGridLayout *layout() const;

public Q_SLOTS:
  void next();
  void prev();

Q_SIGNALS:
  void requestPage(int index);
  void afterLayout();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  int total = 0;
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
