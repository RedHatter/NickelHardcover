#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../cli.h"
#include "../synccontroller.h"
#include "../widgets/label.h"
#include "../widgets/rating.h"
#include "reviewdialog.h"

void ReviewDialog::show() { new ReviewDialog(); }

ReviewDialog::ReviewDialog() : Dialog("Write your review") {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] ReviewDialog {
      margin: 0 12px 12px;
    }
    [qApp_deviceIsPhoenix=true] ReviewDialog {
      margin: 0 16px 16px;
    }
    [qApp_deviceIsDragon=true] ReviewDialog {
      margin: 0 22px 22px;
    }
    [qApp_deviceIsStorm=true] ReviewDialog {
      margin: 0 25px 25px;
    }
    [qApp_deviceIsDaylight=true] ReviewDialog {
      margin: 0 28px 28px;
    }

    [qApp_deviceIsTrilogy=true] TouchTextEdit {
      margin-top: 12px;
    }
    [qApp_deviceIsPhoenix=true] TouchTextEdit {
      margin-top: 16px;
    }
    [qApp_deviceIsDragon=true] TouchTextEdit {
      margin-top: 22px;
    }
    [qApp_deviceIsStorm=true] TouchTextEdit {
      margin-top: 25px;
    }
    [qApp_deviceIsDaylight=true] TouchTextEdit {
      margin-top: 28px;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  Label *loading = new Label(Label::Small, "Loading. Please wait...");
  loading->setAlignment(Qt::AlignCenter);
  layout->addWidget(loading, 1);

  CLI *cli = CLI::getUserBook();
  QObject::connect(cli, &CLI::response, this, &ReviewDialog::response);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}

void ReviewDialog::response(QJsonObject doc) {
  QLayout *column = layout();
  column->takeAt(0)->widget()->deleteLater();

  rating = doc.value("rating").toDouble(0);
  spoilers = doc.value("review_has_spoilers").toBool(false);
  sponsored = doc.value("sponsored_review").toBool(false);

  SyncController *ctl = SyncController::getInstance();

  // Title and author
  if (!ctl->title.isEmpty()) {
    column->addWidget(new Label(Label::Large, ctl->title));
  }

  if (!ctl->author.isEmpty()) {
    column->addWidget(new Label(Label::Small, "by " + ctl->author));
  }

  // Rating
  Rating *ratingWidget = new Rating(rating, this);
  column->addWidget(ratingWidget);
  QObject::connect(ratingWidget, &Rating::tapped, this, &ReviewDialog::setRating);

  // Has spoilers
  TouchCheckBox *checkbox = construct_TouchCheckBox(this);
  checkbox->setChecked(spoilers);
  checkbox->setText("This review contains spoilers");
  column->addWidget(checkbox);
  QObject::connect(checkbox, &QCheckBox::stateChanged, this, &ReviewDialog::setSpoilers);

  // Is sponsored
  checkbox = construct_TouchCheckBox(this);
  checkbox->setChecked(sponsored);
  checkbox->setText("Sponsored or ARC Review");
  column->addWidget(checkbox);
  QObject::connect(checkbox, &QCheckBox::stateChanged, this, &ReviewDialog::setSponsored);

  // Textbox
  TouchTextEdit *touchText = construct_TouchTextEdit(this);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Share you thoughts about this book with the world. Make "
                                                     "sure to Mark any spoilers!");
  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();
  textEdit->setText(doc.value("review_raw").toString(""));
  column->addWidget(touchText);

  buildKeyboardFrame(textEdit, "Submit");
  showKeyboard();
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

  CLI *cli = CLI::setUserBook(rating, textEdit->toPlainText(), spoilers, sponsored);
  QObject::connect(cli, &CLI::success, dialog, &QDialog::deleteLater);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}
