#include <QGridLayout>
#include <QHBoxLayout>
#include <QNetworkReply>

#include <NickelHook.h>

#include "../files.h"
#include "../nickelhardcover.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "editionrow.h"

EditionRow::EditionRow(const Edition &edition, QWidget *parent) : QFrame(parent), id(QString::number(edition.id)) {
  QGridLayout *layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] EditionRow {
      padding:  11px;
      qproperty-verticalSpacing: 11;
    }
    [qApp_deviceIsPhoenix=true] EditionRow {
      padding: 13px;
      qproperty-verticalSpacing: 13;
    }
    [qApp_deviceIsDragon=true] EditionRow {
      padding: 18px;
      qproperty-verticalSpacing: 18;
    }
    [qApp_deviceIsStorm=true] EditionRow {
      padding: 21px;
      qproperty-verticalSpacing: 21;
    }
    [qApp_deviceIsDaylight=true] EditionRow {
      padding: 23px;
      qproperty-verticalSpacing: 23;
    }

    EditionRow {
      border-top: 1px solid #666666;
    }

    EditionRow[noBorder=true] {
      border-top-width: 0;
    }

    [qApp_deviceIsTrilogy=true] QLabel#cover {
      margin-right: 12px;
      max-width: 36px;
      min-width: 36px;
      max-height: 55px;
      min-height: 55px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#cover {
      margin-right: 15px;
      max-width: 42px;
      min-width: 42px;
      max-height: 64px;
      min-height: 64px;
    }
    [qApp_deviceIsDragon=true] QLabel#cover {
      margin-right: 20px;
      max-width: 65px;
      min-width: 65px;
      max-height: 100px;
      min-height: 100px;
    }
    [qApp_deviceIsStorm=true] QLabel#cover {
      margin-right: 22px;
      max-width: 75px;
      min-width: 75px;
      max-height: 116px;
      min-height: 116px;
    }
    [qApp_deviceIsDaylight=true] QLabel#cover {
      margin-right: 26px;
      max-width: 84px;
      min-width: 84px;
      max-height: 130px;
      min-height: 130px;
    }

    QLabel#cover[blank=true] {
      background-color: #d9d9d9;
    }
  )");

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(0);
  layout->addLayout(hbox, 0, 0, 1, -1);

  cover = buildCover(edition);
  hbox->addWidget(cover);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  hbox->addLayout(vbox, 1);
  vbox->addStretch(1);

  if (edition.title) {
    vbox->addWidget(new ElidedLabel(Label::Medium, *edition.title));
  }

  vbox->addWidget(new ElidedLabel(Label::ExtraSmall, join(edition.contributions, ", ")));

  vbox->addWidget(new Label(Label::ExtraSmall, "<b>Publisher:</b> " + edition.publisher.value_or("No data")));

  vbox->addStretch(1);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setText("Select edition");
  button->setProperty("borderedButton", true);
  hbox->addWidget(button);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(tapped()));

  QList<QPair<QString, QString>> list = {
      {"Type", edition.reading_format},
      {"Format", edition.edition_format.value_or_default()},
      {"Information", edition.edition_information.value_or_default()},
      {"Pages", QString::number(edition.pages.value_or_default())},
      {"Release Date", edition.release_date.value_or_default()},
      {"ISBN 10", edition.isbn_10.value_or_default()},
      {"ISBN 13", edition.isbn_13.value_or_default()},
      {"ASIN", edition.asin.value_or_default()},
      {"Data Score", QString::number(edition.score)},
      {"Language", edition.language.value_or_default()},
      {"Country", edition.country.value_or_default()},
      {"Readers", QString::number(edition.users_count)},
  };

  for (int i = 0; i < list.size(); i++) {
    QString text = QString("<b>%1:</b><br>%2")
                       .arg(list[i].first)
                       .arg(list[i].second.isEmpty() || list[i].second == "0" ? "No data" : list[i].second);
    layout->addWidget(new Label(Label::ExtraSmall, text), i / 4 + 1, i % 4);
  }
}

QLabel *EditionRow::buildCover(const Edition &edition) {
  QLabel *label = new QLabel();
  label->setObjectName("cover");
  label->setScaledContents(true);

  if (edition.image && !edition.image->isEmpty()) {
    label->setPixmap(QPixmap(Files::loading_cover));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(*edition.image)));
    QObject::connect(reply, &QNetworkReply::finished, this, &EditionRow::loadCover);
  } else {
    label->setProperty("blank", true);
  }

  return label;
}

void EditionRow::loadCover() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

  if (reply->error() == QNetworkReply::NoError) {
    QPixmap pixmap;
    pixmap.loadFromData(reply->readAll());
    cover->setPixmap(pixmap);
  } else {
    nh_log("Error loading image %s", qPrintable(reply->errorString()));
    cover->clear();
    cover->setProperty("blank", true);
  }

  reply->deleteLater();
}
