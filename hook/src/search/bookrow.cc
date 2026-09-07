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

BookRow::BookRow(SearchResult result, QWidget *parent) : QFrame(parent), id(result.id.value_or_default()) {
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

  cover = buildCover(result);
  layout->addWidget(cover);

  QVBoxLayout *textLayout = new QVBoxLayout();
  textLayout->setContentsMargins(0, 0, 0, 0);
  textLayout->setSpacing(1);
  layout->addLayout(textLayout, 1);
  textLayout->addStretch(1);

  textLayout->addWidget(new ElidedLabel(Label::Large, result.title.value_or("Unknown")));
  textLayout->addWidget(new ElidedLabel(Label::Avenir, getSeries(result)));
  textLayout->addWidget(new ElidedLabel(Label::Small, join(result.authors, ", ")));
  textLayout->addWidget(new ElidedLabel(Label::Small, getMeta(result)));

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

QLabel *BookRow::buildCover(SearchResult result) {
  QLabel *label = new QLabel();
  label->setObjectName("cover");
  label->setScaledContents(true);

  if (result.image) {
    label->setPixmap(QPixmap(Files::loading_cover));

    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(*result.image)));
    QObject::connect(reply, &QNetworkReply::finished, this, &BookRow::loadCover);
  } else {
    label->setProperty("blank", true);
  }

  return label;
}

QString BookRow::getSeries(SearchResult result) {
  if (!result.series || !result.series->name) {
    return "";
  }

  QString seriesName = *result.series->name;

  if (!seriesName.isEmpty() && result.series->position) {
    seriesName.append(" - ").append(QString::number(*result.series->position));
  }

  return seriesName;
}

QString BookRow::getMeta(SearchResult result) {
  QStringList meta;

  if (result.release_year) {
    meta.append(QString::number(*result.release_year));
  }

  if (result.users_count && *result.users_count > 0) {
    meta.append(QString::number(*result.users_count).append(" Readers"));
  }

  if (result.rating && *result.rating > 0) {
    meta.append(QString::number(*result.rating, 'f', 1).append(" ★"));
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
