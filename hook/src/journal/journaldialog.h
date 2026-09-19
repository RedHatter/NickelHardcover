#pragma once

#include <QJsonObject>
#include <QStackedLayout>
#include <QWidget>

#include "../messages.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class JournalDialog : public Dialog {
  Q_OBJECT

public:
  static void show() { new JournalDialog(); }

public Q_SLOTS:
  void annotations();
  void newEntry();

private:
  JournalDialog();

  void response(const Messages &message);
  void requestPage(int index);

  int offset = 0;
  PagedStack *pages;
};
