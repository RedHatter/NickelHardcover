#include <QObject>
#include <QSettings>

class Settings : public QObject {
  Q_OBJECT

public:
  static Settings *getInstance();

public:
  QString getContentId();
  void setContentId(QString value);

  bool isEnabled();
  void setEnabled(bool value);

private:
  Settings(QObject *parent = nullptr);

  static Settings *instance;

  QSettings *internal = nullptr;
  QString contentId = nullptr;
  QString key = nullptr;
};
