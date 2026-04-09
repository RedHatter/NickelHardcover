#include <QLabel>

class LoadingLabel : public QLabel {
  Q_OBJECT

public:
  LoadingLabel(const QString &text, QWidget *parent = nullptr);
  LoadingLabel(QWidget *parent = nullptr);
};
