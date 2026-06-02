#include <QGridLayout>
#include <QJsonArray>
#include <QVBoxLayout>
#include <QtMath>

#include <NickelHook.h>

#include "../cli.h"
#include "../nickelhardcover.h"
#include "../widgets/elidedlabel.h"
#include "annotationsdialog.h"
#include "annotationsrow.h"

void AnnotationsDialog::show() { new AnnotationsDialog(); }

AnnotationsDialog::AnnotationsDialog() : Dialog("Annotations") {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] QStackedWidget {
      margin: 0 20px;
    }
    [qApp_deviceIsPhoenix=true] QStackedWidget {
      margin: 0 24px;
    }
    [qApp_deviceIsDragon=true] QStackedWidget {
      margin: 0 37px;
    }
    [qApp_deviceIsStorm=true] QStackedWidget {
      margin: 0 42px;
    }
    [qApp_deviceIsDaylight=true] QStackedWidget {
      margin: 0 48px;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  pages = new PagedStack(this);
  layout->addWidget(pages, 1);
  QObject::connect(pages, &PagedStack::requestPage, this, &AnnotationsDialog::requestPage);

  CLI *cli = CLI::listBookmarks();
  QObject::connect(cli, &CLI::response, this, &AnnotationsDialog::response);
}

void AnnotationsDialog::response(QJsonObject doc) {
  bookmarks = doc.value("bookmarks").toArray();
  bookmarksInitialized = true;
  pages->next();
}

void AnnotationsDialog::requestPage(int index) {
  if (!bookmarksInitialized)
    return;

  int length = bookmarks.size();

  if (length < 1) {
    pages->setStatusText("No annotations found.");
    return;
  }

  QWidget *page = new QWidget(pages);
  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  int availableHeight = pages->getAvailableHeight();
  bool isFirst = true;
  for (; offset < length; offset++) {
    QJsonObject obj = bookmarks.at(offset).toObject();
    AnnotationsRow *row = new AnnotationsRow(obj, this);

    if (isFirst) {
      row->setObjectName("first");
      isFirst = false;
    }

    availableHeight -= row->sizeHint().height();
    if (availableHeight < 0) {
      row->deleteLater();
      break;
    }

    layout->addWidget(row);
  }

  layout->addStretch(1);
  pages->addPage(page);

  if (index == 1) {
    pages->setTotal(qCeil((float)length / offset));
  }
};
