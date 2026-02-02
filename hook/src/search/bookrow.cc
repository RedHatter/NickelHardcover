#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>

#include "NickelHook.h"

#include "../synccontroller.h"
#include "bookrow.h"
#include "elidedlabel.h"

BookCover::BookCover(QJsonObject json, QWidget *parent) : QLabel(parent) {
  setContentsMargins(26, 26, 26, 26);
  setFixedSize(194, 260);
  setScaledContents(true);

  QJsonValue imageUrl = json.value("image");
  if (imageUrl.isString()) {
    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imageUrl.toString())));
    QObject::connect(reply, &QNetworkReply::finished, this, &BookCover::finished);
  } else {
    setPixmap(QPixmap("/usr/share/NickelHardcover/cover" + QString::number(qrand() % 4 + 1) + ".png"));
  }
}

void BookCover::finished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

  if (reply->error() == QNetworkReply::NoError) {
    QPixmap pixmap;
    pixmap.loadFromData(reply->readAll());
    setPixmap(pixmap);
  } else {
    nh_log("Error loading image %s", qPrintable(reply->errorString()));
  }

  reply->deleteLater();
}

QLayout *buildBookText(QJsonObject json) {
  QVBoxLayout *textLayout = new QVBoxLayout();

  textLayout->addStretch(1);

  QJsonValue title = json.value("title");
  if (title.isString()) {
    ElidedLabel *titleLabel = new ElidedLabel(title.toString());
    textLayout->addWidget(titleLabel);
  }

  QJsonObject series = json.value("series").toObject();
  QJsonValue seriesName = series.value("name");
  if (seriesName.isString()) {
    QString seriesString = "";

    QJsonValue seriesPosition = series.value("position");
    if (seriesPosition.isDouble()) {
      seriesString.append("#").append(QString::number(seriesPosition.toDouble()));

      QJsonValue seriesCount = series.value("primary_books_count");
      if (seriesCount.isDouble()) {
        seriesString.append(" of ").append(QString::number(seriesCount.toDouble()));
      }

      seriesString.append(" in ");
    }

    seriesString.append(seriesName.toString().toUpper());

    ElidedLabel *seriesLabel = new ElidedLabel(seriesString);
    seriesLabel->setFontSize(8);
    textLayout->addWidget(seriesLabel, 0);
  }

  QJsonValue author = json.value("authors");
  if (author.isArray()) {
    QJsonArray authorArray = author.toArray();

    if (!authorArray.isEmpty()) {
      QString authorString = "by ";

      QJsonArray::const_iterator i;
      for (i = authorArray.constBegin(); i != authorArray.constEnd(); ++i) {
        if ((*i).isString()) {
          authorString.append((*i).toString()).append(", ");
        }
      }

      authorString.chop(2);

      ElidedLabel *authorLabel = new ElidedLabel(authorString);
      authorLabel->setFontSize(8);
      textLayout->addWidget(authorLabel);
    }
  }

  QString metaString = "";

  QJsonValue year = json.value("release_year");
  if (year.isDouble()) {
    metaString.append(QString::number(year.toDouble()));
  }

  double usersCount = json.value("users_count").toDouble(0);
  if (usersCount > 0) {
    if (metaString.length() != 0) {
      metaString.append(" • ");
    }

    metaString.append(QString::number(usersCount)).append(" Readers");
  }

  double rating = json.value("rating").toDouble(0);
  if (rating > 0) {
    if (metaString.length() != 0) {
      metaString.append(" • ");
    }

    metaString.append(QString::number(rating, 'f', 1)).append(" ★");
  }

  ElidedLabel *metaLabel = new ElidedLabel(metaString);
  metaLabel->setFontSize(8);
  textLayout->addWidget(metaLabel);

  textLayout->addStretch(1);

  return textLayout;
}

SettingContainer *buildBookRow(QJsonObject json, bool showBottomBorder) {
  SettingContainer *container = reinterpret_cast<SettingContainer *>(calloc(1, 128));
  SettingContainer__constructor(container, nullptr);

  if (showBottomBorder) {
    SettingContainer__setShowBottomLine(container, true);
  }

  QHBoxLayout *layout = new QHBoxLayout(container);
  layout->addWidget(new BookCover(json, container));
  layout->addLayout(buildBookText(json), 1);

  return container;
}
