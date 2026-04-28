#include <QKeyEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../files.h"
#include "../nickelhardcover.h"
#include "pagedstack.h"
#include "qapplication.h"
#include "qcoreevent.h"
#include "qevent.h"
#include "qlayoutitem.h"
#include "qnamespace.h"

PagedStack::PagedStack(QWidget *parent) : QWidget(parent) {
  QApplication::instance()->installEventFilter(new PagedStackFilter(this));

  QGridLayout *layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->setRowStretch(0, 1);
  layout->setColumnStretch(1, 1);

  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] PagedStack {
      qproperty-footerHeight: 66;
      qproperty-footerButtonWidth: 88;
    }
    [qApp_deviceIsPhoenix=true] PagedStack {
      qproperty-footerHeight: 75;
      qproperty-footerButtonWidth: 110;
    }
    [qApp_deviceIsDragon=true] PagedStack {
      qproperty-footerHeight: 120;
      qproperty-footerButtonWidth: 147;
    }
    [qApp_deviceIsStorm=true] PagedStack {
      qproperty-footerHeight: 138;
      qproperty-footerButtonWidth: 169;
    }
    [qApp_deviceIsDaylight=true] PagedStack {
      qproperty-footerHeight: 156;
      qproperty-footerButtonWidth: 191;
    }
  )");

  stack = new QStackedWidget();
  stack->setContentsMargins(0, 0, 0, 0);
  stack->layout()->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(stack, 0, 0, 1, -1);

  prevButton = construct_TouchLabel(this);
  prevButton->setPixmap(QPixmap(Files::arrow_backward));
  prevButton->setAlignment(Qt::AlignCenter);
  layout->addWidget(prevButton, 1, 0);
  prevButton->hide();
  QWidget::connect(prevButton, SIGNAL(tapped(bool)), this, SLOT(prev()));

  label = new QLabel();
  label->setObjectName("small");
  label->setStyleSheet("font-style: italic;");
  layout->addWidget(label, 1, 1, Qt::AlignCenter);
  label->hide();

  nextButton = construct_TouchLabel(this);
  nextButton->setPixmap(QPixmap(Files::arrow_forward));
  nextButton->setAlignment(Qt::AlignCenter);
  layout->addWidget(nextButton, 1, 2);
  nextButton->hide();
  QWidget::connect(nextButton, SIGNAL(tapped(bool)), this, SLOT(next()));

  QLabel *loading = new QLabel();
  loading->setObjectName("loading");
  stack->addWidget(loading);
}

void PagedStack::setCurrent(int value) {
  current = value;
  stack->setCurrentIndex(current);

  if (current <= 1 || total == 1) {
    prevButton->hide();
  } else {
    prevButton->show();
  }

  if (total <= -1 || total == 1 || (total > 0 && total <= current)) {
    nextButton->hide();
  } else {
    nextButton->show();
  }

  if (total == 1) {
    label->hide();
  } else if (total > 1) {
    label->setText(QString("Page %1 of %2").arg(current).arg(total));
    label->show();
  } else if (current > 0) {
    label->setText(QString("Page %1").arg(current));
    label->show();
  }
}

void PagedStack::setTotal(int value) {
  total = value;
  setCurrent(current);
}

void PagedStack::next() {
  nh_log("PagedStack::next()");

  int next = current + 1;
  if (next < stack->count()) {
    setCurrent(next);
  } else if (total <= 0 || next <= total) {
    stack->setCurrentIndex(0);
    nextButton->hide();
    prevButton->hide();
    requestPage(next);
  } else {
    setCurrent(1);
  }
}

void PagedStack::prev() {
  nh_log("PagedStack::prev()");

  if (current > 1) {
    setCurrent(current - 1);
  }
}

void PagedStack::addPage(QWidget *page) { setCurrent(stack->addWidget(page)); }

void PagedStack::clear() {
  while (QLayoutItem *item = stack->layout()->takeAt(1)) {
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }

    delete item;
  }

  setCurrent(0);
}

int PagedStack::getAvailableHeight() { return stack->contentsRect().height(); }

int PagedStack::countPages() { return stack->count() - 1; }

void PagedStack::resizeEvent(QResizeEvent *event) {
  afterLayout();
  QWidget::resizeEvent(event);
}

QGridLayout *PagedStack::layout() const { return qobject_cast<QGridLayout *>(QWidget::layout()); }

void PagedStack::setFooterHeight(int value) { layout()->setRowMinimumHeight(1, value); }

int PagedStack::footerHeight() const { return layout()->rowMinimumHeight(1); }

void PagedStack::setFooterButtonWidth(int value) {
  QGridLayout *grid = layout();
  grid->setColumnMinimumWidth(0, value);
  grid->setColumnMinimumWidth(2, value);
}

int PagedStack::footerButtonWidth() const { return layout()->columnMinimumWidth(0); }

PagedStackFilter::PagedStackFilter(PagedStack *pages) : QObject(pages), pages(pages) {}

bool PagedStackFilter::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    switch (static_cast<QKeyEvent *>(event)->key()) {
    case Qt::Key_Down:
      pages->next();
      return true;

    case Qt::Key_Up:
      pages->prev();
      return true;
    }
  }

  return QObject::eventFilter(obj, event);
}
