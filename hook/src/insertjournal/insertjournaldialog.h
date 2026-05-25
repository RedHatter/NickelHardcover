#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../widgets/buttongroup.h"
#include "../widgets/dialog.h"

class InsertJournalDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

  void commit() override;

private:
  InsertJournalDialog();

  ButtonGroup *privacy;
};
