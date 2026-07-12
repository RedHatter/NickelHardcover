#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>

#include <NickelHook.h>

#include "../files.h"
#include "../nickelhardcover.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "editionrow.h"

EditionRow::EditionRow(QJsonObject json, QWidget *parent)
    : QFrame(parent), id(QString::number(json.value("id").toInt())) {
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

  cover = buildCover(json);
  hbox->addWidget(cover);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  hbox->addLayout(vbox, 1);
  vbox->addStretch(1);

  vbox->addWidget(new ElidedLabel(Label::Medium, json.value("title").toString()));
  vbox->addWidget(
      new ElidedLabel(Label::ExtraSmall, json.value("contributions").toVariant().toStringList().join(", ")));

  QString publisher = json.value("publisher").toString();
  vbox->addWidget(new Label(Label::ExtraSmall, "<b>Publisher:</b> " + (publisher.isEmpty() ? "No data" : publisher)));

  vbox->addStretch(1);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setText("Select edition");
  button->setProperty("borderedButton", true);
  hbox->addWidget(button);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(tapped()));

  QList<QPair<QString, QString>> list = {
      {"Type", json.value("reading_format").toString()},
      {"Format", json.value("edition_format").toString()},
      {"Information", json.value("edition_information").toString()},
      {"Pages", QString::number(json.value("pages").toInt())},
      {"Release Date", json.value("release_date").toString()},
      {"ISBN 10", json.value("isbn_10").toString()},
      {"ISBN 13", json.value("isbn_13").toString()},
      {"ASIN", json.value("asin").toString()},
      {"Data Score", QString::number(json.value("score").toInt())},
      {"Language", json.value("language").toString()},
      {"Country", json.value("country").toString()},
      {"Readers", QString::number(json.value("users_count").toInt())},
  };

  for (int i = 0; i < list.size(); i++) {
    QString text = QString("<b>%1:</b><br>%2")
                       .arg(list[i].first)
                       .arg(list[i].second.isEmpty() || list[i].second == "0" ? "No data" : list[i].second);
    layout->addWidget(new Label(Label::ExtraSmall, text), i / 4 + 1, i % 4);
  }
}

QGridLayout *EditionRow::layout() const { return qobject_cast<QGridLayout *>(QWidget::layout()); }

void EditionRow::setVerticalSpacing(int value) { layout()->setVerticalSpacing(value); }

int EditionRow::verticalSpacing() const { return layout()->verticalSpacing(); };

QLabel *EditionRow::buildCover(QJsonObject json) {
  QLabel *label = new QLabel();
  label->setObjectName("cover");
  label->setScaledContents(true);

  QString imageUrl = json.value("image").toString();
  if (imageUrl.isEmpty()) {
    label->setProperty("blank", true);
  } else {
    label->setPixmap(QPixmap(Files::loading_cover));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imageUrl)));
    QObject::connect(reply, &QNetworkReply::finished, this, &EditionRow::loadCover);
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

void EditionRow::tapped() { selected(id); }
