#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QScreen>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../settings.h"
#include "../synccontroller.h"
#include "../widgets/loadinglabel.h"
#include "bookrow.h"
#include "searchdialog.h"

void SearchDialog::show(QString query) { new SearchDialog(query); }

SearchDialog::SearchDialog(QString query) : Dialog("Manually link book") {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  lineEdit = reinterpret_cast<TouchLineEdit *>(calloc(1, 128));
  TouchLineEdit__constructor(lineEdit, nullptr);
  lineEdit->setText(query);
  lineEdit->setStyleSheet("margin: 0 48px;");
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

  int limit = pages->getAvailableHeight() / 266;

  CLI *cli = CLI::search(query, limit, index);
  QObject::connect(cli, &CLI::response, this, &SearchDialog::response);
}

void SearchDialog::response(QJsonObject doc) {
  pages->total = doc.value("total").toInt(1);

  QJsonArray resultsArray = doc.value("results").toArray();
  int length = resultsArray.size();

  if (length < 1) {
    pages->addPage(new LoadingLabel("No results."));
    return;
  }

  QWidget *box = new QWidget(pages);
  QVBoxLayout *results = new QVBoxLayout(box);
  results->setContentsMargins(48, 0, 48, 0);
  results->setSpacing(0);

  for (int i = 0; i < length; i++) {
    BookRow *row = new BookRow(resultsArray.at(i).toObject(), i == length - 1, box);
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
