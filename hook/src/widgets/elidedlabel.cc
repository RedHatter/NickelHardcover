#include <QPainter>
#include <QTextLayout>

#include "elidedlabel.h"

ElidedLabel::ElidedLabel(const QString &text, int maxLines, QWidget *parent) : QFrame(parent) {
  this->text = text;
  this->maxLines = maxLines;
  QSizePolicy policy = QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  policy.setHeightForWidth(true);
  setSizePolicy(policy);
}

int ElidedLabel::heightForWidth(int width) const {
  QFontMetrics fontMetrics = this->fontMetrics();

  int lineSpacing = fontMetrics.lineSpacing();
  int y = 0;
  int lineIndex = 0;

  QTextLayout textLayout(text, this->font());
  textLayout.beginLayout();

  forever {
    QTextLine line = textLayout.createLine();

    if (!line.isValid())
      break;

    line.setLineWidth(width);

    if (lineIndex < maxLines - 1) {
      y += lineSpacing;
      lineIndex++;
    } else {
      y += lineSpacing;
      line = textLayout.createLine();
      break;
    }
  }

  textLayout.endLayout();

  return y;
}

QSize ElidedLabel::sizeHint() const { return QSize(width(), heightForWidth(width())); }

void ElidedLabel::paintEvent(QPaintEvent *event) {
  QFrame::paintEvent(event);

  QPainter painter(this);
  QFontMetrics fontMetrics = painter.fontMetrics();

  int lineSpacing = fontMetrics.lineSpacing();
  int y = 0;
  int lineIndex = 0;

  QTextLayout textLayout(text, painter.font());
  textLayout.beginLayout();

  forever {
    QTextLine line = textLayout.createLine();

    if (!line.isValid())
      break;

    line.setLineWidth(width());

    if (lineIndex < maxLines - 1) {
      line.draw(&painter, QPoint(0, y));
      y += lineSpacing;
      lineIndex++;
    } else {
      QString lastLine = text.mid(line.textStart());
      QString elidedLastLine = fontMetrics.elidedText(lastLine, Qt::ElideRight, width());

      y += fontMetrics.ascent();
      painter.drawText(QPoint(0, y), elidedLastLine);
      line = textLayout.createLine();
      break;
    }
  }

  textLayout.endLayout();
}
