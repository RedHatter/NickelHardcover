#include <QDateTime>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../settings.h"
#include "../synccontroller.h"
#include "checkboxrow.h"
#include "menurow.h"
#include "settingsdialog.h"
#include "staticrow.h"

void SettingsDialog::show() { new SettingsDialog(); }

SettingsDialog::SettingsDialog() : Dialog("Settings") {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] QLabel#title {
      font-size: 30px;
      margin: 24px 36px 12px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#title {
      font-size: 36px;
      margin: 32px 48px 16px;
    }
    [qApp_deviceIsDragon=true] QLabel#title {
      font-size: 46px;
      margin: 44px 66px 22px;
    }
    [qApp_deviceIsAlyssum=true] QLabel#title,
    [qApp_deviceIsNova=true] QLabel#title {
      font-size: 50px;
    }
    [qApp_deviceIsStorm=true] QLabel#title {
      font-size: 54px;
      margin: 50px 75px 25px;
    }
    [qApp_deviceIsDaylight=true] QLabel#title {
      font-size: 60px;
      margin: 56px 84px 28px;
    }

    [qApp_deviceIsTrilogy=true] QStackedWidget {
      margin: 0 36px;
    }
    [qApp_deviceIsPhoenix=true] QStackedWidget {
      margin: 0 48px;
    }
    [qApp_deviceIsDragon=true] QStackedWidget {
      margin: 0 66px;
    }
    [qApp_deviceIsStorm=true] QStackedWidget {
      margin: 0 75px;
    }
    [qApp_deviceIsDaylight=true] QStackedWidget {
      margin: 0 84px;
    }

    [qApp_deviceIsTrilogy=true] QLabel#metaData,
    [qApp_deviceIsTrilogy=true] StaticRow {
      padding-left: 12px;
      padding-right: 12px;
    }
    [qApp_deviceIsPhoenix=true] QLabel#metaData,
    [qApp_deviceIsPhoenix=true] StaticRow {
      padding-left: 16px;
      padding-right: 16px;
    }
    [qApp_deviceIsDragon=true] QLabel#metaData,
    [qApp_deviceIsDragon=true] StaticRow{
      padding-left: 22px;
      padding-right: 22px;
    }
    [qApp_deviceIsStorm=true] QLabel#metaData,
    [qApp_deviceIsStorm=true] StaticRow {
      padding-left: 25px;
      padding-right: 25px;
    }
    [qApp_deviceIsDaylight=true] QLabel#metaData,
    [qApp_deviceIsDaylight=true] StaticRow {
      padding-left: 28px;
      padding-right: 28px;
    }

    [qApp_deviceIsTrilogy=true] SettingContainer {
      qproperty-leftMargin: 12px;
      qproperty-rightMargin: 12px;
      qproperty-spacing: 12px;
    }
    [qApp_deviceIsPhoenix=true] SettingContainer {
      qproperty-leftMargin: 16px;
      qproperty-rightMargin: 16px;
      qproperty-spacing: 16px;
    }
    [qApp_deviceIsDragon=true] SettingContainer {
      qproperty-leftMargin: 22px;
      qproperty-rightMargin: 22px;
      qproperty-spacing: 22px;
    }
    [qApp_deviceIsStorm=true] SettingContainer {
      qproperty-leftMargin: 25px;
      qproperty-rightMargin: 25px;
      qproperty-spacing: 25px;
    }
    [qApp_deviceIsDaylight=true] SettingContainer {
      qproperty-leftMargin: 28px;
      qproperty-rightMargin: 28px;
      qproperty-spacing: 28px;
    }

    [qApp_deviceIsTrilogy=true] StaticRow {
      padding-top: 12px;
      padding-bottom: 12px;
    }
    [qApp_deviceIsPhoenix=true] StaticRow {
      padding-top: 16px;
      padding-bottom: 16px;
    }
    [qApp_deviceIsDragon=true] StaticRow {
      padding-top: 22px;
      padding-bottom: 22px;
    }
    [qApp_deviceIsStorm=true] StaticRow {
      padding-top: 25px;
      padding-bottom: 25px;
    }
    [qApp_deviceIsDaylight=true] StaticRow {
      padding-top: 28px;
      padding-bottom: 28px;
    }

    SettingContainer QCheckBox {
      padding: 0px;
    }

    QLabel {
      qproperty-indent: 0;
    }

    QLabel#header, SettingContainer, StaticRow {
      border-top: 1px solid black;
    }

    #first SettingContainer, StaticRow#first {
      border-top-width: 0px;
    }

    QLabel#metaData {
      padding-top: 5px;
      padding-bottom: 5px;
      background-color: #d9d9d9;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  QLabel *label = new QLabel("Settings");
  label->setObjectName("title");
  layout->addWidget(label);

  pages = new PagedStack();
  layout->addWidget(pages);
  QObject::connect(pages, &PagedStack::afterLayout, this, &SettingsDialog::buildPages);
}

void SettingsDialog::buildPages() {
  QObject::disconnect(pages, &PagedStack::afterLayout, this, &SettingsDialog::buildPages);

  QWidget *page = new QWidget();
  QVBoxLayout *rows = new QVBoxLayout(page);
  rows->setSpacing(0);
  rows->setContentsMargins(0, 0, 0, 0);

  QList<QFrame *> sections = {buildGeneral(), buildAutoSync(), buildInformation(), buildAdvanced()};
  int availableHeight = pages->getAvailableHeight();
  int pageHeight = 0;

  for (int i = 0; i < sections.size(); i++) {
    QFrame *section = sections.at(i);
    int height = section->sizeHint().height();
    pageHeight += height;

    if (pageHeight >= availableHeight) {
      rows->addStretch(1);
      pages->addPage(page);
      page = new QWidget();
      rows = new QVBoxLayout(page);
      rows->setSpacing(0);
      rows->setContentsMargins(0, 0, 0, 0);
      pageHeight = height;
    }

    rows->addWidget(section);
  }

  rows->addStretch(1);
  pages->addPage(page);
  pages->setTotal(pages->countPages());
  pages->next();
}

QFrame *SettingsDialog::buildGeneral() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  QLabel *label = new QLabel("General");
  label->setObjectName("metaData");
  layout->addWidget(label);

  StaticRow *row = new StaticRow("Version", NH_VERSION, false);
  layout->addWidget(row);
  row->setObjectName("first");

  username = new StaticRow("Authorized user", "Unknown", false);
  layout->addWidget(username);

  CLI *cli = CLI::getUser();
  QObject::connect(cli, &CLI::response, this, &SettingsDialog::setUsername);

  CheckboxRow *checkboxRow = new CheckboxRow("Auto-sync by default", Settings::getInstance()->getAutoSyncDefault());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, this, &SettingsDialog::setAutoSyncDefault);
  layout->addWidget(checkboxRow);

  MenuRow *menuRow =
      new MenuRow("Sync annotations to reading journal", MenuRowType::Menu,
                  {Item{"Always", "always"}, Item{"Never", "never"}, Item{"Once the book is finished", "finished"}}, {},
                  Settings::getInstance()->getSyncBookmarks());
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setSyncBookmarks);
  layout->addWidget(menuRow);

  return frame;
}

QFrame *SettingsDialog::buildAutoSync() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  QLabel *label = new QLabel("Auto-sync");
  label->setObjectName("metaData");
  layout->addWidget(label);

  QList<Item> hours;
  for (int hour = 1; hour <= 24; hour++) {
    QString text;

    if (hour < 12) {
      text = QString::number(hour).append(" AM");
    } else if (hour == 12) {
      text = "12 PM";
    } else if (hour == 24) {
      text = "12 AM";
    } else {
      text = QString::number(hour - 12).append(" PM");
    }

    hours.append(Item{text, hour});
  }

  Settings *settings = Settings::getInstance();
  MenuRow *menuRow =
      new MenuRow("Once per day", MenuRowType::Menu, {Item{"Never", 0}, Item{"Set time of day", MenuRow::OPEN_DIALOG}},
                  hours, settings->getSyncDaily());
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setSyncDaily);
  layout->addWidget(menuRow);
  menuRow->setObjectName("first");

  QList<Item> thresholdItems;
  for (int i = 1; i < 100; i++) {
    thresholdItems.append(Item{QString::number(i).append("%"), i});
  }

  menuRow = new MenuRow("After closing a book or the Kobo is put to sleep", MenuRowType::Menu,
                        {Item{"Always", 1}, Item{"Never", 0}, Item{"Set a threshold", MenuRow::OPEN_DIALOG}},
                        thresholdItems, QVariant(settings->getCloseThreshold()));
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setCloseThreshold);
  layout->addWidget(menuRow);

  menuRow = new MenuRow("Periodically by read percentage", MenuRowType::Menu,
                        {Item{"Never", 0}, Item{"Set a threshold", MenuRow::OPEN_DIALOG}}, thresholdItems,
                        QVariant(settings->getPageThreshold()));
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setPageThreshold);
  layout->addWidget(menuRow);

  return frame;
}

QFrame *SettingsDialog::buildInformation() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  QLabel *label = new QLabel("Book information");
  label->setObjectName("metaData");
  layout->addWidget(label);

  SyncController *ctl = SyncController::getInstance();
  QDateTime alarm = ctl->getAlarm();
  StaticRow *row =
      new StaticRow("Auto-sync scheduled for", alarm.isValid() ? alarm.toLocalTime().toString() : "Never", false);
  layout->addWidget(row);
  row->setObjectName("first");

  row = new StaticRow("Current progress", QString::number(ctl->getReadProgress()).append("%"), false);
  layout->addWidget(row);

  QString lastSynced = Settings::getInstance()->getLastSynced(ctl->contentId);
  lastSynced = lastSynced.isEmpty() ? "Never" : QDateTime::fromString(lastSynced, Qt::ISODate).toLocalTime().toString();
  row = new StaticRow(
      "Last synced",
      QString::number(Settings::getInstance()->getLastProgress(ctl->contentId)).append("% at ").append(lastSynced),
      true);
  layout->addWidget(row);
  QObject::connect(row, &StaticRow::clear, this, &SettingsDialog::clearLastSynced);

  return frame;
}

QFrame *SettingsDialog::buildAdvanced() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  QLabel *label = new QLabel("Advanced");
  label->setObjectName("metaData");
  layout->addWidget(label);

  CheckboxRow *checkboxRow = new CheckboxRow("Debug logs", Settings::getInstance()->getDebug());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, this, &SettingsDialog::setAutoSyncDefault);
  layout->addWidget(checkboxRow);
  checkboxRow->setObjectName("first");

  MenuRow *menuRow = new MenuRow("Save system logs", MenuRowType::Tap, {Item{"Save", true}}, {}, true);
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::saveLogs);
  layout->addWidget(menuRow);

  return frame;
}

void SettingsDialog::setUsername(QJsonObject doc) { username->setValue(doc.value("username").toString().prepend("@")); }

void SettingsDialog::setAutoSyncDefault(bool value) { Settings::getInstance()->setAutoSyncDefault(value); }

void SettingsDialog::setSyncBookmarks(QVariant value) { Settings::getInstance()->setSyncBookmarks(value.toString()); }

void SettingsDialog::setSyncDaily(QVariant value) { Settings::getInstance()->setSyncDaily(value.toInt()); }

void SettingsDialog::setCloseThreshold(QVariant value) { Settings::getInstance()->setCloseThreshold(value.toInt()); }

void SettingsDialog::setPageThreshold(QVariant value) { Settings::getInstance()->setPageThreshold(value.toInt()); }

void SettingsDialog::clearLastSynced() {
  QString contentId = SyncController::getInstance()->contentId;
  Settings *settings = Settings::getInstance();
  settings->setLastSynced(contentId, QString());
  settings->setLastProgress(contentId, 0);

  StaticRow *row = qobject_cast<StaticRow *>(sender());
  QString lastSynced = settings->getLastSynced(contentId);
  lastSynced = lastSynced.isEmpty() ? "Never" : QDateTime::fromString(lastSynced, Qt::ISODate).toLocalTime().toString();
  row->setValue(QString::number(settings->getLastProgress(contentId)).append("% at ").append(lastSynced));
}

void SettingsDialog::setDebug(bool value) { Settings::getInstance()->setDebug(value); }

void SettingsDialog::saveLogs() { nh_dump_log(); }
