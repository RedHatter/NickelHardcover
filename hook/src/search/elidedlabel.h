#include <QLabel>

class ElidedLabel : public QLabel {
  Q_OBJECT

public:
  explicit ElidedLabel(QWidget *parent = nullptr);
  explicit ElidedLabel(QString text, QWidget *parent = nullptr);
  void setText(QString);
  void setFontSize(int pointSize);

protected:
  void resizeEvent(QResizeEvent *);

private:
  void updateText();
  QString m_text;
};
