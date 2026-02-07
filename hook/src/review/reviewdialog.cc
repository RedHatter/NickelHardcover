#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../files.h"
#include "../synccontroller.h"
#include "rating.h"
#include "reviewdialog.h"

void ReviewDialogContent::showReviewDialog() {
  ReviewDialogContent *content = new ReviewDialogContent();

  SyncController *ctl = SyncController::getInstance();
  QProcess *process = new QProcess();
  QString linkedBook = ctl->getLinkedBook();
  if (linkedBook.isEmpty()) {
    process->start(Files::cli, {"get-review", "--content-id", ctl->getContentId()});
  } else {
    process->start(Files::cli, {"get-review", "--book-id", linkedBook});
  }

  QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), content, &ReviewDialogContent::buildContent);
}

void ReviewDialogContent::buildDialog() {
  N3Dialog *dialog = N3DialogFactory__getDialog(this, true);
  N3Dialog__setTitle(dialog, "Write your review");

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dialog->setFixedSize(screenGeometry.width(), screenGeometry.height());

  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, dialog, [dialog](QString name) {
    if (name != "ReadingView") {
      dialog->deleteLater();
    }
  });
  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));
  QObject::connect(this, &ReviewDialogContent::close, dialog, &QDialog::deleteLater);

  KeyboardFrame *keyboard = N3Dialog__keyboardFrame(dialog);
  QTextEdit *textEdit = findChild<QTextEdit *>();

  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__TextEdit_constructor(receiver, textEdit, false);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, QLocale::English);
  SearchKeyboardController__setReceiver(ctl, receiver, false);
  SearchKeyboardController__setGoText(ctl, "Submit");

  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(commit()));

  keyboard->show();
  dialog->show();
}

void ReviewDialogContent::buildContent(int exitCode) {
  QProcess *cli = qobject_cast<QProcess *>(sender());

  if (exitCode > 0) {
    QByteArray stderr = cli->readAllStandardError();
    nh_log("Error from command line \"%s\"", qPrintable(stderr));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", QString(stderr));

    return;
  }

  QByteArray json = cli->readAllStandardOutput();
  QJsonObject doc = QJsonDocument::fromJson(json).object();

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
  QObject::connect(ratingWidget, &Rating::tapped, this, &ReviewDialogContent::setRating);

  // Has spoilers
  TouchCheckBox *checkbox = reinterpret_cast<TouchCheckBox *>(calloc(1, 128));
  TouchCheckBox__constructor(checkbox, this);
  checkbox->setCheckState(spoilers ? Qt::Checked : Qt::Unchecked);
  checkbox->setText("This review contains spoilers");
  layout->addWidget(checkbox);
  QObject::connect(checkbox, &QCheckBox::stateChanged, this, &ReviewDialogContent::setSpoilers);

  // Is sponsored
  checkbox = reinterpret_cast<TouchCheckBox *>(calloc(1, 128));
  TouchCheckBox__constructor(checkbox, this);
  checkbox->setCheckState(sponsored ? Qt::Checked : Qt::Unchecked);
  checkbox->setText("Sponsored or ARC Review");
  layout->addWidget(checkbox);
  QObject::connect(checkbox, &QCheckBox::stateChanged, this, &ReviewDialogContent::setSponsored);

  layout->addSpacing(16);

  // Textbox
  TouchTextEdit *touchText = reinterpret_cast<TouchTextEdit *>(calloc(1, 128));
  TouchTextEdit__constructor(touchText, this);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Share you thoughts about this book with the world. Make sure to Mark any spoilers!");
  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();
  textEdit->setText(doc.value("review_text").toString(""));
  layout->addWidget(touchText);

  buildDialog();
};

ReviewDialogContent::ReviewDialogContent(QWidget *parent) : QWidget(parent) {}

void ReviewDialogContent::setRating(float value) {
  nh_log("ReviewDialogContent::setRating(%f)", value);

  rating = value;
}

void ReviewDialogContent::setSpoilers(int state) {
  nh_log("ReviewDialogContent::setSpoilers(%i)", state);

  spoilers = state == Qt::Checked;
}

void ReviewDialogContent::setSponsored(int state) {
  nh_log("ReviewDialogContent::setSponsored(%i)", state);

  sponsored = state == Qt::Checked;
}

void ReviewDialogContent::commit() {
  nh_log("ReviewDialogContent::commit()");

  QTextEdit *textEdit = findChild<QTextEdit *>();
  QStringList args = {"review", "--rating", QString::number(rating), "--text", textEdit->toPlainText()};

  SyncController *ctl = SyncController::getInstance();
  QString linkedBook = ctl->getLinkedBook();
  if (linkedBook.isEmpty()) {
    args.append({"--content-id", ctl->getContentId()});
  } else {
    args.append({"--book-id", linkedBook});
  }

  if (spoilers) {
    args.append("--spoilers");
  }

  if (sponsored) {
    args.append("--sponsored");
  }

  QProcess *cli = new QProcess();
  cli->start(Files::cli, args);
  QObject::connect(cli, &QProcess::readyReadStandardOutput, this, &ReviewDialogContent::readyReadStandardOutput);
  QObject::connect(cli, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ReviewDialogContent::finished);
}

void ReviewDialogContent::readyReadStandardOutput() {
  QProcess *process = qobject_cast<QProcess *>(sender());

  QByteArray msg = process->readAllStandardOutput();
  QList<QByteArray> lines = msg.split('\n');

  for (QByteArray &line : lines) {
    if (line.length() == 0)
      continue;

    nh_log("%s", qPrintable(line));
  }
}

void ReviewDialogContent::finished(int exitCode) {
  QProcess *cli = qobject_cast<QProcess *>(sender());

  close();

  if (exitCode > 0) {
    QByteArray stderr = cli->readAllStandardError();
    nh_log("Error from command line \"%s\"", qPrintable(stderr));
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", QString(stderr));
  }
}
