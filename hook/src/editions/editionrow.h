#include <QFrame>
#include <QJsonObject>
#include <QLabel>

class EditionRow : public QFrame {
  Q_OBJECT

public:
  EditionRow(QJsonObject json, QWidget *parent = nullptr);

public Q_SLOTS:
  void tapped();
  void loadCover();

Q_SIGNALS:
  void selected(QString id);

private:
  QString id;
  QLabel *cover = nullptr;

  QLabel *buildCover(QJsonObject json);
  QWidget *buildTitle(QJsonObject json);
  QWidget *buildAuthor(QJsonObject json);
  QWidget *buildPublisher(QJsonObject json);
  QWidget *buildDetails(QJsonObject json);
};
