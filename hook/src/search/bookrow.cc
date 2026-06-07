#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>

#include <NickelHook.h>

#include "../files.h"
#include "../nickelhardcover.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "bookrow.h"

BookRow::BookRow(QJsonObject json, QWidget *parent) : QFrame(parent), id(json.value("id").toString()) {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] BookRow {
      padding: 12px;
    }
    [qApp_deviceIsPhoenix=true] BookRow {
      padding: 15px;
    }
    [qApp_deviceIsDragon=true] BookRow {
      padding: 20px;
    }
    [qApp_deviceIsStorm=true] BookRow {
      padding: 22px;
    }
    [qApp_deviceIsDaylight=true] BookRow {
      padding: 26px;
    }

    [qApp_deviceIsTrilogy=true] QLabel#cover {
      max-width: 60px;
      min-width: 60px;
      max-height: 90px;
      min-height: 90px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#cover {
      max-width: 70px;
      min-width: 70px;
      max-height: 110px;
      min-height: 110px;
    }
    [qApp_deviceIsDragon=true] QLabel#cover {
      max-width: 108px;
      min-width: 108px;
      max-height: 168px;
      min-height: 168px;
    }
    [qApp_deviceIsStorm=true] QLabel#cover {
      max-width: 126px;
      min-width: 126px;
      max-height: 196px;
      min-height: 196px;
    }
    [qApp_deviceIsDaylight=true] QLabel#cover {
      max-width: 140px;
      min-width: 140px;
      max-height: 218px;
      min-height: 218px;
    }

    QLabel#cover[blank=true] {
      background-color: #d9d9d9;
    }

    BookRow {
      border-top: 1px solid #666666;
    }

    BookRow[noBorder=true] {
      border-top-width: 0;
    }
  )");

  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  cover = buildCover(json);
  layout->addWidget(cover);

  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setContentsMargins(0, 0, 0, 0);
  textLayout->setSpacing(1);
  layout->addLayout(textLayout, 1);
  textLayout->addStretch(1);

  textLayout->addWidget(new ElidedLabel(Label::Large, json.value("title").toString()));
  textLayout->addWidget(new ElidedLabel(Label::Avenir, getSeries(json)));
  textLayout->addWidget(new ElidedLabel(Label::Small, json.value("authors").toVariant().toStringList().join(", ")));
  textLayout->addWidget(new ElidedLabel(Label::Small, getMeta(json)));

  textLayout->addStretch(1);

  QVBoxLayout *buttons = new QVBoxLayout();
  buttons->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(buttons);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setProperty("borderedButton", true);
  button->setText("Select book");
  buttons->addWidget(button);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(selectTapped()));

  button = construct_N3ButtonLabel(this);
  button->setText("Editions");
  buttons->addWidget(button, 0, Qt::AlignLeft);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(editionsTapped()));

  buttons->addStretch(1);
}

QLabel *BookRow::buildCover(QJsonObject json) {
  QLabel *label = new QLabel();
  label->setObjectName("cover");
  label->setScaledContents(true);

  QString imageUrl = json.value("image").toString();
  if (imageUrl.isEmpty()) {
    label->setProperty("blank", true);
  } else {
    label->setPixmap(QPixmap(Files::loading));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imageUrl)));
    QObject::connect(reply, &QNetworkReply::finished, this, &BookRow::loadCover);
  }

  return label;
}

QString BookRow::getSeries(QJsonObject json) {
  QJsonObject series = json.value("series").toObject();
  QString seriesName = series.value("name").toString();

  QJsonValue position = series.value("position");
  if (!seriesName.isEmpty() && position.isDouble()) {
    seriesName.append(" - ").append(QString::number(position.toDouble()));
  }

  return seriesName;
}

QString BookRow::getMeta(QJsonObject json) {
  QStringList meta;

  QJsonValue year = json.value("release_year");
  if (year.isDouble()) {
    meta.append(QString::number(year.toDouble()));
  }

  double usersCount = json.value("users_count").toDouble();
  if (usersCount > 0) {
    meta.append(QString::number(usersCount).append(" Readers"));
  }

  double rating = json.value("rating").toDouble();
  if (rating > 0) {
    meta.append(QString::number(rating, 'f', 1).append(" ★"));
  }

  return meta.join(" • ");
}

void BookRow::loadCover() {
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

void BookRow::selectTapped() { selected(id); }

void BookRow::editionsTapped() { editions(id); }
