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

AnnotationsRow::AnnotationsRow(Annotation annotation, QWidget *parent) : QFrame(parent), annotation(annotation) {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] AnnotationsRow {
      padding: 12px;
    }
    [qApp_deviceIsPhoenix=true] AnnotationsRow {
      padding: 16px;
    }
    [qApp_deviceIsDragon=true] AnnotationsRow {
      padding: 22px;
    }
    [qApp_deviceIsStorm=true] AnnotationsRow {
      padding: 25px;
    }
    [qApp_deviceIsDaylight=true] AnnotationsRow {
      padding: 28px;
    }

    AnnotationsRow {
      border-top: 1px solid #666666;
    }

    AnnotationsRow[noBorder=true] {
      border-top-width: 0;
    }
  )");

  QHBoxLayout *hbox = new QHBoxLayout(this);
  hbox->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);
  hbox->addLayout(vbox, 1);

  vbox->addWidget(new ElidedLabel(Label::Large, annotation.title));

  vbox->addWidget(new ElidedLabel(Label::Small, annotation.attribution));

  vbox->addSpacing(10);

  vbox->addWidget(new Label(Label::Small, annotation.count == 1 ? "1 annotation"
                                                                : QString::number(annotation.count) + " annotations"));

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
  options.contentId = annotation.volume_id;
  options.query = annotation.title + " " + annotation.attribution;

  CLI *cli = CLI::updateJournal(options);
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
