#include <QFrame>

class ElidedLabel : public QFrame {
  Q_OBJECT

public:
  explicit ElidedLabel(const QString &text, int maxLines = 1, QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  int heightForWidth(int w) const override;
  QSize sizeHint() const override;

private:
  QString text;
  int maxLines;
};
