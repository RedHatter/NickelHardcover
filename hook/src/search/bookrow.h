#include <QJsonObject>
#include <QLabel>

class BookRow : public QWidget {
  Q_OBJECT

public:
  BookRow(QJsonObject json, bool last, QWidget *parent = nullptr);

public Q_SLOTS:
  void rowTapped();
  void loadCover();

Q_SIGNALS:
  void tapped(QString id);

private:
  QString id;
  QLabel* cover = nullptr;

  QLabel* buildCover(QJsonObject json);
  QWidget* buildTitle(QJsonObject json);
  QWidget* buildSeries(QJsonObject json);
  QWidget* buildAuthor(QJsonObject json);
  QWidget* buildMeta(QJsonObject json);
};
