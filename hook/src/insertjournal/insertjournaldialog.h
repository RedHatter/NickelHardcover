#pragma once

#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../messages.h"
#include "../widgets/buttongroup.h"
#include "../widgets/dialog.h"

class InsertJournalDialog : public Dialog {
  Q_OBJECT

public:
  static void show() { new InsertJournalDialog(); }

  void commit() override;

private:
  InsertJournalDialog();

  ButtonGroup *privacy;

  void response(Messages message);
};
