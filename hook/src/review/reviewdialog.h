#pragma once

#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../messages.h"
#include "../widgets/dialog.h"

class ReviewDialog : public Dialog {
  Q_OBJECT

public:
  static void show() { new ReviewDialog(); }

  void commit() override;

private:
  ReviewDialog();

  void response(const Messages &message);

  void setRating(float value);
  void setSpoilers(int state);
  void setSponsored(int state);

  float rating = 0;
  bool spoilers = false;
  bool sponsored = false;
};
