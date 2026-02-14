#include <QVBoxLayout>
#include <QWidget>
#include <QJsonObject>

#include "../nickelhardcover.h"

class ReviewDialogContent : public QWidget {
  Q_OBJECT

public:
  static void showReviewDialog();

public Q_SLOTS:
  void networkConnected();
  void commit();
  void buildDialog();
  void buildContent(QJsonObject doc);

  void setRating(float value);
  void setSpoilers(int state);
  void setSponsored(int state);

Q_SIGNALS:
  void close();

private:
  ReviewDialogContent(QWidget *parent = nullptr);

  float rating = 0;
  bool spoilers = false;
  bool sponsored = false;
};
