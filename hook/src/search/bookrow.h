#pragma once

#include <QFrame>
#include <QJsonObject>
#include <QLabel>

#include "../messages.h"

class BookRow : public QFrame {
  Q_OBJECT

public:
  BookRow(const SearchResult &result, QWidget *parent = nullptr);

public Q_SLOTS:
  void selectTapped() { selected(id); }
  void editionsTapped() { editions(id); }

Q_SIGNALS:
  void selected(QString id);
  void editions(QString id);

private:
  QString id;
  QLabel *cover = nullptr;

  void loadCover();

  QLabel *buildCover(const SearchResult &result);
  QString getSeries(const SearchResult &result);
  QString getMeta(const SearchResult &result);
};
