#include <QFrame>
#include <QJsonObject>
#include <QLabel>

class BookRow : public QFrame {
  Q_OBJECT

public:
  BookRow(QJsonObject json, QWidget *parent = nullptr);

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

  QLabel *buildCover(QJsonObject json);
  QString getSeries(QJsonObject json);
  QString getMeta(QJsonObject json);
};
