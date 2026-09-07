#include <QFrame>
#include <QJsonObject>

#include "../messages.h"

class JournalEntry : public QFrame {
  Q_OBJECT

public:
  JournalEntry(Journal doc, QWidget *parent = nullptr);
};
