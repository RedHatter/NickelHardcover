#include <QFrame>
#include <QJsonObject>

class JournalEntry : public QFrame {
  Q_OBJECT

public:
  JournalEntry(QJsonObject doc, QWidget *parent = nullptr);
};
