#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>

#include <NickelHook.h>

#include "../files.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "bookrow.h"

BookRow::BookRow(QJsonObject json, QWidget *parent) : QWidget(parent), id(json.value("id").toString()) {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] SettingContainer {
      qproperty-spacing: 12;
      qproperty-topMargin: 12;
      qproperty-rightMargin: 12;
      qproperty-bottomMargin: 12;
      qproperty-leftMargin: 12;
    }
    [qApp_deviceIsPhoenix=true] SettingContainer {
      qproperty-spacing: 15;
      qproperty-topMargin: 15;
      qproperty-rightMargin: 15;
      qproperty-bottomMargin: 15;
      qproperty-leftMargin: 15;
    }
    [qApp_deviceIsDragon=true] SettingContainer {
      qproperty-spacing: 20;
      qproperty-topMargin: 20;
      qproperty-rightMargin: 20;
      qproperty-bottomMargin: 20;
      qproperty-leftMargin: 20;
    }
    [qApp_deviceIsStorm=true] SettingContainer {
      qproperty-spacing: 22;
      qproperty-topMargin: 22;
      qproperty-rightMargin: 22;
      qproperty-bottomMargin: 22;
      qproperty-leftMargin: 22;
    }
    [qApp_deviceIsDaylight=true] SettingContainer {
      qproperty-spacing: 26;
      qproperty-topMargin: 26;
      qproperty-rightMargin: 26;
      qproperty-bottomMargin: 26;
      qproperty-leftMargin: 26;
    }

    [qApp_deviceIsTrilogy=true] QLabel#blank-cover,
    [qApp_deviceIsTrilogy=true] QLabel#cover {
      max-width: 60px;
      min-width: 60px;
      max-height: 90px;
      min-height: 90px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#blank-cover,
    [qApp_deviceIsPhoenix=true] QLabel#cover {
      max-width: 70px;
      min-width: 70px;
      max-height: 110px;
      min-height: 110px;
    }
    [qApp_deviceIsDragon=true] QLabel#blank-cover,
    [qApp_deviceIsDragon=true] QLabel#cover {
      max-width: 108px;
      min-width: 108px;
      max-height: 168px;
      min-height: 168px;
    }
    [qApp_deviceIsStorm=true] QLabel#blank-cover,
    [qApp_deviceIsStorm=true] QLabel#cover {
      max-width: 126px;
      min-width: 126px;
      max-height: 196px;
      min-height: 196px;
    }
    [qApp_deviceIsDaylight=true] QLabel#blank-cover,
    [qApp_deviceIsDaylight=true] QLabel#cover {
      max-width: 140px;
      min-width: 140px;
      max-height: 218px;
      min-height: 218px;
    }

    [qApp_deviceIsTrilogy=true] #title {
      font-size: 23px;
    }
    [qApp_deviceIsPhoenix=true] #title {
      font-size: 28px;
    }
    [qApp_deviceIsDragon=true] #title {
      font-size: 36px;
    }
    [qApp_deviceIsAlyssum=true],
    [qApp_deviceIsNova=true] #title {
      font-size: 39px;
    }
    [qApp_deviceIsStorm=true] #title {
      font-size: 42px;
    }
    [qApp_deviceIsDaylight=true] #title {
      font-size: 47px;
    }

    #blank-cover {
      background-color: #d9d9d9;
    }

    SettingContainer {
      border-top: 1px solid #666666;
    }

    #first SettingContainer {
      border-top-width: 0;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  SettingContainer *row = reinterpret_cast<SettingContainer *>(calloc(1, 128));
  SettingContainer__constructor(row, nullptr);
  layout->addWidget(row);
  QObject::connect(row, SIGNAL(tapped()), this, SLOT(rowTapped()));

  QHBoxLayout *rowLayout = new QHBoxLayout(row);
  rowLayout->setContentsMargins(0, 0, 0, 0);

  cover = buildCover(json);
  rowLayout->addWidget(cover);

  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setContentsMargins(0, 0, 0, 0);
  textLayout->setSpacing(1);
  rowLayout->addLayout(textLayout, 1);
  textLayout->addStretch(1);

  if (QWidget *widget = buildTitle(json)) {
    textLayout->addWidget(widget);
  }

  if (QWidget *widget = buildSeries(json)) {
    textLayout->addWidget(widget);
  }

  if (QWidget *widget = buildAuthor(json)) {
    textLayout->addWidget(widget);
  }

  if (QWidget *widget = buildMeta(json)) {
    textLayout->addWidget(widget);
  }

  textLayout->addStretch(1);
}

QLabel *BookRow::buildCover(QJsonObject json) {
  QLabel *label = new QLabel();
  label->setObjectName("cover");
  label->setScaledContents(true);

  QJsonValue imageUrl = json.value("image");
  if (imageUrl.isString()) {
    label->setPixmap(QPixmap(Files::loading));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imageUrl.toString())));
    QObject::connect(reply, &QNetworkReply::finished, this, &BookRow::loadCover);
  } else {
    label->setObjectName("blank-cover");
  }

  return label;
}

QWidget *BookRow::buildTitle(QJsonObject json) {
  QJsonValue title = json.value("title");
  if (!title.isString())
    return nullptr;

  ElidedLabel *label = new ElidedLabel(title.toString());
  label->setObjectName("title");
  return label;
}

QWidget *BookRow::buildSeries(QJsonObject json) {
  QJsonObject series = json.value("series").toObject();
  QJsonValue seriesName = series.value("name");
  if (!seriesName.isString())
    return nullptr;

  QString seriesString = seriesName.toString();

  QJsonValue seriesPosition = series.value("position");
  if (seriesPosition.isDouble()) {
    seriesString.append(" - ").append(QString::number(seriesPosition.toDouble()));
  }

  ElidedLabel *label = new ElidedLabel(seriesString);
  label->setObjectName("metaData");
  return label;
}

QWidget *BookRow::buildAuthor(QJsonObject json) {
  QJsonValue author = json.value("authors");
  if (!author.isArray())
    return nullptr;

  QStringList authorList = author.toVariant().toStringList();
  if (authorList.isEmpty())
    return nullptr;

  ElidedLabel *label = new ElidedLabel(authorList.join(", "));
  label->setObjectName("regular");
  return label;
}

QWidget *BookRow::buildMeta(QJsonObject json) {
  QStringList meta;

  QJsonValue year = json.value("release_year");
  if (year.isDouble()) {
    meta.append(QString::number(year.toDouble()));
  }

  double usersCount = json.value("users_count").toDouble(0);
  if (usersCount > 0) {
    meta.append(QString::number(usersCount).append(" Readers"));
  }

  double rating = json.value("rating").toDouble(0);
  if (rating > 0) {
    meta.append(QString::number(rating, 'f', 1).append(" ★"));
  }

  ElidedLabel *label = new ElidedLabel(meta.join(" • "));
  label->setObjectName("regular");
  return label;
}

void BookRow::loadCover() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

  if (reply->error() == QNetworkReply::NoError) {
    QPixmap pixmap;
    pixmap.loadFromData(reply->readAll());
    cover->setPixmap(pixmap);
  } else {
    nh_log("Error loading image %s", qPrintable(reply->errorString()));
  }

  reply->deleteLater();
}

void BookRow::rowTapped() { tapped(id); }
