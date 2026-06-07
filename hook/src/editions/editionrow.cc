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
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] EditionRow {
      padding: 12px;
    }
    [qApp_deviceIsPhoenix=true] EditionRow {
      padding: 15px;
    }
    [qApp_deviceIsDragon=true] EditionRow {
      padding: 20px;
    }
    [qApp_deviceIsStorm=true] EditionRow {
      padding: 22px;
    }
    [qApp_deviceIsDaylight=true] EditionRow {
      padding: 26px;
    }

    EditionRow {
      border-top: 1px solid #666666;
    }

    EditionRow[noBorder=true] {
      border-top-width: 0;
    }

    [qApp_deviceIsTrilogy=true] QLabel#cover {
      max-width: 36px;
      min-width: 36px;
      max-height: 55px;
      min-height: 55px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#cover {
      max-width: 42px;
      min-width: 42px;
      max-height: 64px;
      min-height: 64px;
    }
    [qApp_deviceIsDragon=true] QLabel#cover {
      max-width: 65px;
      min-width: 65px;
      max-height: 100px;
      min-height: 100px;
    }
    [qApp_deviceIsStorm=true] QLabel#cover {
      max-width: 75px;
      min-width: 75px;
      max-height: 116px;
      min-height: 116px;
    }
    [qApp_deviceIsDaylight=true] QLabel#cover {
      max-width: 84px;
      min-width: 84px;
      max-height: 130px;
      min-height: 130px;
    }

    QLabel#cover[blank=true] {
      background-color: #d9d9d9;
    }
  )");

  QGridLayout *layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  QHBoxLayout *hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(hbox, 0, 0, 1, -1);

  cover = buildCover(json);
  hbox->addWidget(cover);

  QVBoxLayout *vbox = new QVBoxLayout();
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  hbox->addLayout(vbox, 1);
  vbox->addStretch(1);

  if (QWidget *widget = buildTitle(json)) {
    vbox->addWidget(widget);
  }

  if (QWidget *widget = buildAuthor(json)) {
    vbox->addWidget(widget);
  }

  if (QWidget *widget = buildPublisher(json)) {
    vbox->addWidget(widget);
  }

  vbox->addStretch(1);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setProperty("borderedButton", true);
  button->setText("Select edition");
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
    QString text =
        QString("<b>%1:</b><br>%2").arg(list[i].first).arg(list[i].second.isEmpty() ? "No data" : list[i].second);
    layout->addWidget(new Label(Label::ExtraSmall, text), i / 4 + 1, i % 4);
  }
}

QLabel *EditionRow::buildCover(QJsonObject json) {
  QLabel *label = new QLabel();
  label->setObjectName("cover");
  label->setScaledContents(true);

  QString imageUrl = json.value("image").toString();
  if (imageUrl.isEmpty()) {
    label->setProperty("blank", true);
  } else {
    label->setPixmap(QPixmap(Files::loading));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imageUrl)));
    QObject::connect(reply, &QNetworkReply::finished, this, &EditionRow::loadCover);
  }

  return label;
}

QWidget *EditionRow::buildTitle(QJsonObject json) {
  QString title = json.value("title").toString();
  if (title.isEmpty()) {
    return nullptr;
  }

  return new ElidedLabel(Label::Medium, title);
}

QWidget *EditionRow::buildAuthor(QJsonObject json) {
  QStringList author = json.value("contributions").toVariant().toStringList();
  if (author.isEmpty()) {
    return nullptr;
  }

  return new ElidedLabel(Label::ExtraSmall, author.join(", "));
}

QWidget *EditionRow::buildPublisher(QJsonObject json) {
  QString publisher = json.value("publisher").toString();
  if (publisher.isEmpty()) {
    return nullptr;
  }

  return new Label(Label::ExtraSmall, "<b>Publisher:</b> " + publisher);
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
