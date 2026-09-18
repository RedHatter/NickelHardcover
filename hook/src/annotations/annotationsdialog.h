#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QStackedLayout>

#include "../messages.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class AnnotationsDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(Messages message);
  void requestPage(int index);

private:
  AnnotationsDialog();

  int offset = 0;
  Optional<AnnotationList> annotationList;
  PagedStack *pages;
};
