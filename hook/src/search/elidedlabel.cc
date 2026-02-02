#include "elidedlabel.h"
#include <NickelHook.h>

ElidedLabel::ElidedLabel(QWidget *parent) : ElidedLabel("", parent) {}

ElidedLabel::ElidedLabel(QString text, QWidget *parent) : QLabel(parent) { setText(text); }

void ElidedLabel::setFontSize(int pointSize) {
  QFont font = this->font();
  font.setPointSize(pointSize);
  setFont(font);
}

void ElidedLabel::setText(QString text) {
  m_text = text;
  updateText();
}

void ElidedLabel::resizeEvent(QResizeEvent *event) {
  QLabel::resizeEvent(event);
  updateText();
}

void ElidedLabel::updateText() {
  QFontMetrics metrics(font());
  QString elided = metrics.elidedText(m_text, Qt::ElideRight, width());
  QLabel::setText(elided);
}
