#pragma once

#include <QFrame>
#include <QJsonObject>

#include "../messages.h"
#include "../nickelhardcover.h"

class AnnotationsRow : public QFrame {
  Q_OBJECT

public:
  AnnotationsRow(Annotation annotation, QWidget *parent = nullptr);

public Q_SLOTS:
  void tapped();
  void success();
  void closeDialog();

private:
  Annotation annotation;
  ConfirmationDialog *dialog = nullptr;
};
