#include <NickelHook.h>

#include "../cli.h"
#include "../synccontroller.h"
#include "../widgets/label.h"
#include "../widgets/rating.h"
#include "reviewdialog.h"

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

void ReviewDialog::response(const Messages &message) {
  if (!message.isUserBook()) {
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Unexpected CLI response for <i>getUserBook</i>");
    return;
  }

  QLayout *column = layout();

  QLayoutItem *item = column->takeAt(0);
  item->widget()->deleteLater();
  delete item;

  rating = message.user_book->rating.value_or_default();
  spoilers = message.user_book->review_has_spoilers;
  sponsored = message.user_book->sponsored_review;

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
  textEdit->setText(message.user_book->review_raw.value_or_default());
  column->addWidget(touchText);

  buildKeyboardFrame(textEdit, "Submit");
  showKeyboard();
}

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
}
