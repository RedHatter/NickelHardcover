#include <QJsonObject>
#include <QVBoxLayout>
#include <QWidget>

#include "../widgets/dialog.h"

class InsertJournalDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public:
  void commit() override;

private:
  InsertJournalDialog(QWidget *parent = nullptr);

protected:
  void build() override;
};
