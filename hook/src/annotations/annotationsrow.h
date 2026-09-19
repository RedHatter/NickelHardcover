#pragma once

#include <QFrame>
#include <QJsonObject>

#include "../messages.h"
#include "../nickelhardcover.h"

class AnnotationsRow : public QFrame {
  Q_OBJECT

public:
  AnnotationsRow(const Annotation &annotation, QWidget *parent = nullptr);

public Q_SLOTS:
  void tapped();

private:
  void success();
  void closeDialog();

  Annotation annotation;
  ConfirmationDialog *dialog = nullptr;
};
