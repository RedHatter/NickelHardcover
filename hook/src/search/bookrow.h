#include <QJsonObject>
#include <QLabel>

#include "../nickelhardcover.h"

SettingContainer *buildBookRow(QJsonObject json);

class BookCover : public QLabel {
  Q_OBJECT

public:
  BookCover(QJsonObject json, QWidget *parent = nullptr);

public Q_SLOTS:
  void finished();
};
