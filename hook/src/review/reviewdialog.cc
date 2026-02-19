#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../cli.h"
#include "../files.h"
#include "../synccontroller.h"
#include "../widgets/rating.h"
#include "reviewdialog.h"

void ReviewDialog::show() {
  ReviewDialog *dialog = new ReviewDialog();
  dialog->connectNetwork();
}

ReviewDialog::ReviewDialog(QWidget *parent) : Dialog("Write your review", parent) {}

void ReviewDialog::build() {
  CLI *cli = new CLI(this);
  cli->getUserBook();
  QObject::connect(cli, &CLI::response, this, &ReviewDialog::response);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}

void ReviewDialog::response(QJsonObject doc) {
  rating = doc.value("rating").toDouble(0);
  spoilers = doc.value("review_has_spoilers").toBool(false);
  sponsored = doc.value("sponsored_review").toBool(false);

  QVBoxLayout *layout = new QVBoxLayout(this);

  SyncController *ctl = SyncController::getInstance();

  // Title and author
  if (ctl->title != nullptr) {
    QLabel *label = new QLabel(ctl->title);
    label->setStyleSheet("QLabel { font-size: 12pt; }");
    layout->addWidget(label);
  }

  if (ctl->author != nullptr) {
    QLabel *label = new QLabel("by " + ctl->author);
    label->setStyleSheet("QLabel { font-size: 8pt; }");
    layout->addWidget(label);
  }

  // Rating
  Rating *ratingWidget = new Rating(rating, this);
  layout->addWidget(ratingWidget);
  QObject::connect(ratingWidget, &Rating::tapped, this, &ReviewDialog::setRating);

  // Has spoilers
  TouchCheckBox *checkbox = reinterpret_cast<TouchCheckBox *>(calloc(1, 128));
  TouchCheckBox__constructor(checkbox, this);
  checkbox->setCheckState(spoilers ? Qt::Checked : Qt::Unchecked);
  checkbox->setText("This review contains spoilers");
  layout->addWidget(checkbox);
  QObject::connect(checkbox, &QCheckBox::stateChanged, this, &ReviewDialog::setSpoilers);

  // Is sponsored
  checkbox = reinterpret_cast<TouchCheckBox *>(calloc(1, 128));
  TouchCheckBox__constructor(checkbox, this);
  checkbox->setCheckState(sponsored ? Qt::Checked : Qt::Unchecked);
  checkbox->setText("Sponsored or ARC Review");
  layout->addWidget(checkbox);
  QObject::connect(checkbox, &QCheckBox::stateChanged, this, &ReviewDialog::setSponsored);

  layout->addSpacing(16);

  // Textbox
  TouchTextEdit *touchText = reinterpret_cast<TouchTextEdit *>(calloc(1, 128));
  TouchTextEdit__constructor(touchText, this);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Share you thoughts about this book with the world. Make "
                                                     "sure to Mark any spoilers!");
  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();
  textEdit->setText(doc.value("review_text").toString(""));
  layout->addWidget(touchText);

  buildKeyboardFrame(textEdit, "Submit");
  showKeyboard();
  dialog->show();
};

void ReviewDialog::setRating(float value) {
  nh_log("ReviewDialog::setRating(%f)", value);

  rating = value;
}

void ReviewDialog::setSpoilers(int state) {
  nh_log("ReviewDialog::setSpoilers(%i)", state);

  spoilers = state == Qt::Checked;
}

void ReviewDialog::setSponsored(int state) {
  nh_log("ReviewDialog::setSponsored(%i)", state);

  sponsored = state == Qt::Checked;
}

void ReviewDialog::commit() {
  nh_log("ReviewDialog::commit()");

  QTextEdit *textEdit = findChild<QTextEdit *>();

  CLI *cli = new CLI(this);
  cli->setUserBook(rating, textEdit->toPlainText(), spoilers, sponsored);
  QObject::connect(cli, &CLI::success, dialog, &QDialog::deleteLater);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}
