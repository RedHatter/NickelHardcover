#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

#include "../cli.h"
#include "../nickelhardcover.h"
#include "../search/searchdialog.h"
#include "../settings.h"
#include "../widgets/elidedlabel.h"
#include "annotationsrow.h"
#include "qnamespace.h"

AnnotationsRow::AnnotationsRow(QJsonObject doc, QWidget *parent) : QFrame(parent), doc(doc) {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] AnnotationsRow {
      padding: 12px 0;
    }
    [qApp_deviceIsPhoenix=true] AnnotationsRow {
      padding: 16px 0;
    }
    [qApp_deviceIsDragon=true] AnnotationsRow {
      padding: 22px 0;
    }
    [qApp_deviceIsStorm=true] AnnotationsRow {
      padding: 25px 0;
    }
    [qApp_deviceIsDaylight=true] AnnotationsRow {
      padding: 28px 0;
    }

    AnnotationsRow {
      border-top: 1px solid #666666;
    }

    AnnotationsRow#first {
      border-top-width: 0;
    }
  )");

  QHBoxLayout *hbox = new QHBoxLayout(this);
  hbox->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);
  hbox->addLayout(vbox, 1);

  ElidedLabel *label = new ElidedLabel(doc.value("title").toString());
  label->setObjectName("large");
  vbox->addWidget(label);

  label = new ElidedLabel(doc.value("attribution").toString());
  label->setObjectName("small");
  vbox->addWidget(label);

  vbox->addSpacing(10);

  int n = doc.value("count").toInt();
  QLabel *count = new QLabel(n == 1 ? "1 annotation" : QString::number(n) + " annotations");
  count->setObjectName("small");
  vbox->addWidget(count);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setText("Sync Now");
  hbox->addWidget(button, 0, Qt::AlignTop);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(tapped()));
}

void AnnotationsRow::tapped() {
  dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
  ConfirmationDialog__showCloseButton(dialog, false);
  ConfirmationDialog__setText(dialog, "Syncing annotations with Hardcover.app...");
  dialog->open();

  CLI::Options options;
  options.icon = true;
  options.contentId = doc.value("volume_id").toString();
  options.query = doc.value("title").toString() + " " + doc.value("author").toString();

  CLI *cli = CLI::updateJournal(options.contentId, options);
  QObject::connect(cli, &CLI::success, this, &AnnotationsRow::success);
  QObject::connect(cli, &CLI::failure, this, &AnnotationsRow::closeDialog);
}

void AnnotationsRow::success() {
  if (dialog == nullptr)
    return;

  ConfirmationDialog__setText(dialog, "Success!");
  QTimer::singleShot(800, this, &AnnotationsRow::closeDialog);
}

void AnnotationsRow::closeDialog() {
  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
