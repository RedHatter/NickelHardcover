#include <QFrame>
#include <QJsonObject>
#include <QLabel>

class EditionRow : public QFrame {
  Q_OBJECT
  Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)

public:
  EditionRow(QJsonObject json, QWidget *parent = nullptr);

  void setVerticalSpacing(int value);
  int verticalSpacing() const;

public Q_SLOTS:
  void tapped();
  void loadCover();

Q_SIGNALS:
  void selected(QString id);

private:
  QString id;
  QLabel *cover = nullptr;

  QLabel *buildCover(QJsonObject json);
};
