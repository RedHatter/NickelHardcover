#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QScreen>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../settings.h"
#include "../synccontroller.h"
#include "bookrow.h"
#include "searchdialog.h"

void SearchDialog::show(QString query) { new SearchDialog(query); }

SearchDialog::SearchDialog(QString query) : Dialog("Manually link book") {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] TouchLineEdit {
      margin: 0 20px;
    }
    [qApp_deviceIsPhoenix=true] TouchLineEdit {
      margin: 0 25px;
    }
    [qApp_deviceIsDragon=true] TouchLineEdit {
      margin: 0 35px;
    }
    [qApp_deviceIsStorm=true] TouchLineEdit {
      margin: 0 40px;
    }
    [qApp_deviceIsDaylight=true] TouchLineEdit {
      margin: 0 45px;
    }

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
  layout->setSpacing(0);

  lineEdit = reinterpret_cast<TouchLineEdit *>(calloc(1, 128));
  TouchLineEdit__constructor(lineEdit, nullptr);
  lineEdit->setText(query);
  layout->addWidget(lineEdit);

  pages = new PagedStack(this);
  layout->addWidget(pages, 1);
  QObject::connect(pages, &PagedStack::requestPage, this, &SearchDialog::requestPage);
  QObject::connect(pages, &PagedStack::afterLayout, this, &SearchDialog::commit);

  buildKeyboardFrame(lineEdit, "Search");
}

void SearchDialog::commit() {
  QObject::disconnect(pages, &PagedStack::afterLayout, this, &SearchDialog::commit);

  pages->clear();
  pages->next();
}

void SearchDialog::requestPage(int index) {
  QString query = lineEdit->text();

  BookRow *dummy = new BookRow(QJsonObject());
  int limit = pages->getAvailableHeight() / dummy->sizeHint().height();

  CLI *cli = CLI::search(query, limit, index);
  QObject::connect(cli, &CLI::response, this, &SearchDialog::response);
}

void SearchDialog::response(QJsonObject doc) {
  pages->setTotal(doc.value("total").toInt(1));

  QJsonArray resultsArray = doc.value("results").toArray();
  int length = resultsArray.size();

  if (length < 1) {
    QLabel *loading = new QLabel();
    loading->setObjectName("empty");
    pages->addPage(loading);
    return;
  }

  QWidget *box = new QWidget(pages);
  QVBoxLayout *results = new QVBoxLayout(box);
  results->setContentsMargins(0, 0, 0, 0);
  results->setSpacing(0);

  for (int i = 0; i < length; i++) {
    BookRow *row = new BookRow(resultsArray.at(i).toObject());
    if (i == 0)
      row->setObjectName("first");

    results->addWidget(row);
    QObject::connect(row, &BookRow::tapped, this, &SearchDialog::tapped);
  }

  results->addStretch(1);
  pages->addPage(box);
}

void SearchDialog::tapped(QString id) {
  nh_log("SearchDialog::tapped(%s)", qPrintable(id));
  Settings::getInstance()->setLinkedBook(SyncController::getInstance()->contentId, id);
  dialog->deleteLater();
}
