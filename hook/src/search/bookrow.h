#pragma once

#include <QFrame>
#include <QJsonObject>
#include <QLabel>

#include "../messages.h"

class BookRow : public QFrame {
  Q_OBJECT

public:
  BookRow(SearchResult result, QWidget *parent = nullptr);

public Q_SLOTS:
  void selectTapped();
  void editionsTapped();
  void loadCover();

Q_SIGNALS:
  void selected(QString id);
  void editions(QString id);

private:
  QString id;
  QLabel *cover = nullptr;

  QLabel *buildCover(SearchResult result);
  QString getSeries(SearchResult result);
  QString getMeta(SearchResult result);
};
