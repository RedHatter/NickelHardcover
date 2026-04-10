#include "loadinglabel.h"

LoadingLabel::LoadingLabel(const QString &text, QWidget *parent) : QLabel(text, parent) {
  setAlignment(Qt::AlignCenter);
  setStyleSheet("font-size: 34px;");
}

LoadingLabel::LoadingLabel(QWidget *parent) : LoadingLabel("Loading. Please wait...", parent) {}
