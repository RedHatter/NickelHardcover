#include <QFrame>
#include <QJsonObject>

#include "../nickelhardcover.h"

class AnnotationsRow : public QFrame {
  Q_OBJECT

public:
  AnnotationsRow(QJsonObject doc, QWidget *parent = nullptr);

public Q_SLOTS:
  void tapped();
  void success();
  void closeDialog();

private:
  QJsonObject doc;
  ConfirmationDialog *dialog = nullptr;
};
