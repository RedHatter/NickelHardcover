#pragma once

#include <QVBoxLayout>
#include <QWidget>

#include "../messages.h"
#include "../nickelhardcover.h"
#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"

class SearchDialog : public Dialog {
  Q_OBJECT

public:
  static void show(const QString &contentId) { new SearchDialog(contentId); }

  void commit() override;

private:
  SearchDialog(const QString &contentId);

  void requestPage(int index);
  void response(const Messages &message);
  void selected(const QString &id);
  void editions(const QString &id);

  PagedStack *pages = nullptr;
  TouchLineEdit *lineEdit = nullptr;
  QString contentId;

  void clear();
};
