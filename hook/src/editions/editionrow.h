#include <QFrame>
#include <QGridLayout>
#include <QJsonObject>
#include <QLabel>

#include "../messages.h"

class EditionRow : public QFrame {
  Q_OBJECT
  Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)

public:
  EditionRow(Edition edition, QWidget *parent = nullptr);

  QGridLayout *layout() const;

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

  QLabel *buildCover(Edition edition);
};
