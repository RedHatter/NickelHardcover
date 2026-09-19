#pragma once

#include <QFrame>

#include "label.h"

class ElidedLabel : public QFrame {
  Q_OBJECT
  Q_PROPERTY(QString textSize MEMBER textSize)

public:
  explicit ElidedLabel(const QString &textSize, const QString &text, int maxLines = 1, QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  int heightForWidth(int w) const override;
  QSize sizeHint() const override { return QSize(width(), heightForWidth(width())); }

private:
  QString text;
  int maxLines;
  QString textSize;
};
