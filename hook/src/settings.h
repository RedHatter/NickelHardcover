#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QVariant>

#include "files.h"

class Settings : public QObject {
  Q_OBJECT

public:
  static Settings *getInstance();

  void setEnabled(QString contentId, bool value);
  bool isEnabled(QString contentId);

  void setLinkedBook(QString contentId, QString value);
  QString getLinkedBook(QString contentId);

  void setLastProgress(QString contentId, int value);
  int getLastProgress(QString contentId);

  void setLastSynced(QString contentId, QString value);
  QString getLastSynced(QString contentId);

  int getCloseThreshold();
  int getPageThreshold();
  int getSyncDaily();

public Q_SLOTS:
  void currentViewChanged(QString name);

private:
  Settings(QObject *parent = nullptr);

  static Settings *instance;

  QSettings *internal = nullptr;
  QSettings *config = nullptr;

  QString getPath(QString contentId, QString key);
  void setValue(QString contentId, QString key, QVariant value);
  QVariant getValue(QString contentId, QString key, QVariant defaultValue = QVariant());
};
