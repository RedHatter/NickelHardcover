#pragma once

#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../messages.h"
#include "../widgets/dialog.h"

class ReviewDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

  void commit() override;

public Q_SLOTS:
  void response(Messages message);

  void setRating(float value);
  void setSpoilers(int state);
  void setSponsored(int state);

private:
  ReviewDialog();

  float rating = 0;
  bool spoilers = false;
  bool sponsored = false;
};
