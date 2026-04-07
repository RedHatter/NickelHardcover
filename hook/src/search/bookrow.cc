#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>

#include <NickelHook.h>

#include "../files.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "bookrow.h"

BookRow::BookRow(QJsonObject json, bool last, QWidget *parent) : QWidget(parent), id(json.value("id").toString()) {
  if (last) {
    setObjectName("last");
  }

  setStyleSheet(R"(
    #blank-label {
      background-color: #d9d9d9;
    }

    #title {
      font-size: 48px;
    }

    #series {
      font-family: Avenir, sans-serif;
      font-size: 36px;
    }

    #author, #meta {
      font-size: 34px;
    }

    SettingContainer {
      border-bottom: 1px solid #666666;
    }

    #last SettingContainer {
      border-bottom-width: 0px;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  SettingContainer *row = reinterpret_cast<SettingContainer *>(calloc(1, 128));
  SettingContainer__constructor(row, nullptr);
  row->setContentsMargins(26, 22, 26, 22);
  layout->addWidget(row);
  QObject::connect(row, SIGNAL(tapped()), this, SLOT(rowTapped()));

  QHBoxLayout *rowLayout = new QHBoxLayout(row);

  cover = buildCover(json);
  rowLayout->addWidget(cover);

  rowLayout->addSpacing(26);

  QVBoxLayout *textLayout = new QVBoxLayout();
  rowLayout->addLayout(textLayout, 1);
  textLayout->setSpacing(2);
  textLayout->addStretch(1);

  if (QWidget* widget = buildTitle(json)) {
    textLayout->addWidget(widget);
  }

  if (QWidget* widget = buildSeries(json)) {
    textLayout->addWidget(widget);
  }

  if (QWidget* widget = buildAuthor(json)) {
    textLayout->addWidget(widget);
  }

  if (QWidget* widget = buildMeta(json)) {
    textLayout->addWidget(widget);
  }

  textLayout->addStretch(1);
}

QLabel* BookRow::buildCover(QJsonObject json) {
  QLabel* label = new QLabel(this);
  label->setFixedSize(138, 212);
  label->setScaledContents(true);

  QJsonValue imageUrl = json.value("image");
  if (imageUrl.isString()) {
    label->setPixmap(QPixmap(Files::loading));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imageUrl.toString())));
    QObject::connect(reply, &QNetworkReply::finished, this, &BookRow::loadCover);
  } else {
    label->setObjectName("blank-label");
  }

  return label;
}

QWidget* BookRow::buildTitle(QJsonObject json) {
  QJsonValue title = json.value("title");
  if (!title.isString()) return nullptr;

  ElidedLabel *label = new ElidedLabel(title.toString());
  label->setObjectName("title");
  return label;
}

QWidget* BookRow::buildSeries(QJsonObject json) {
  QJsonObject series = json.value("series").toObject();
  QJsonValue seriesName = series.value("name");
  if (!seriesName.isString()) return nullptr;

  QString seriesString = seriesName.toString().toUpper();

  QJsonValue seriesPosition = series.value("position");
  if (seriesPosition.isDouble()) {
    seriesString.append(" - ").append(QString::number(seriesPosition.toDouble()));
  }

  ElidedLabel *label = new ElidedLabel(seriesString);
  label->setObjectName("series");
  return label;
}

QWidget* BookRow::buildAuthor(QJsonObject json) {
  QJsonValue author = json.value("authors");
  if (!author.isArray()) return nullptr;

  QStringList authorList = author.toVariant().toStringList();
  if (authorList.isEmpty()) return nullptr;

  ElidedLabel *label = new ElidedLabel(authorList.join(", "));
  label->setObjectName("author");
  return label;
}

QWidget* BookRow::buildMeta(QJsonObject json) {
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
  label->setObjectName("meta");
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

void BookRow::rowTapped() {
  tapped(id);
}
