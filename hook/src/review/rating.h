#include <QVBoxLayout>
#include <QWidget>

#include "../nickelhardcover.h"

class Rating : public QWidget {
  Q_OBJECT

public:
  Rating(float value, QWidget *parent = nullptr);

public Q_SLOTS:
  void mouseDown();

Q_SIGNALS:
  void tapped(float value);
};
