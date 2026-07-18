#include <QDateTime>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../settings.h"
#include "../synccontroller.h"
#include "../widgets/label.h"
#include "checkboxrow.h"
#include "menurow.h"
#include "settingsdialog.h"
#include "staticrow.h"

void SettingsDialog::show() { new SettingsDialog(); }

SettingsDialog::SettingsDialog() : Dialog("Settings") {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] Label[textSize="ExtraLarge"] {
      margin: 24px 36px 12px;
    }
    [qApp_deviceIsPhoenix=true] Label[textSize="ExtraLarge"] {
      margin: 32px 48px 16px;
    }
    [qApp_deviceIsDragon=true] Label[textSize="ExtraLarge"] {
      margin: 44px 66px 22px;
    }
    [qApp_deviceIsStorm=true] Label[textSize="ExtraLarge"] {
      margin: 50px 75px 25px;
    }
    [qApp_deviceIsDaylight=true] Label[textSize="ExtraLarge"] {
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

    [qApp_deviceIsTrilogy=true] Label[textSize="Avenir"],
    [qApp_deviceIsTrilogy=true] StaticRow {
      padding-left: 12px;
      padding-right: 12px;
    }
    [qApp_deviceIsPhoenix=true] Label[textSize="Avenir"],
    [qApp_deviceIsPhoenix=true] StaticRow {
      padding-left: 16px;
      padding-right: 16px;
    }
    [qApp_deviceIsDragon=true] Label[textSize="Avenir"],
    [qApp_deviceIsDragon=true] StaticRow{
      padding-left: 22px;
      padding-right: 22px;
    }
    [qApp_deviceIsStorm=true] Label[textSize="Avenir"],
    [qApp_deviceIsStorm=true] StaticRow {
      padding-left: 25px;
      padding-right: 25px;
    }
    [qApp_deviceIsDaylight=true] Label[textSize="Avenir"],
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

    Label {
      qproperty-indent: 0;
    }

    Label[textSize="Avenir"], SettingContainer, StaticRow {
      border-top: 1px solid black;
    }

    [noBorder=true] SettingContainer, StaticRow[noBorder=true] {
      border-top-width: 0px;
    }

    Label[textSize="Avenir"] {
      padding-top: 5px;
      padding-bottom: 5px;
      background-color: #d9d9d9;
    }
  )");

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  layout->addWidget(new Label(Label::ExtraLarge, "Settings"));

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

  for (QFrame *section : sections) {
    int height = section->sizeHint().height();
    pageHeight += height;

    if (pageHeight >= availableHeight) {
      rows->addStretch(1);
      pages->addPage(page);

      pageHeight = height;
      page = new QWidget();
      rows = new QVBoxLayout(page);
      rows->setSpacing(0);
      rows->setContentsMargins(0, 0, 0, 0);
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

  layout->addWidget(new Label(Label::Avenir, "General"));

  StaticRow *row = new StaticRow("Version", NH_VERSION, false);
  layout->addWidget(row);
  row->setProperty("noBorder", true);

  username = new StaticRow("Authorized user", "Unknown", false);
  layout->addWidget(username);

  CLI::Options options;
  options.silent = true;

  CLI *cli = CLI::getUser(options);
  QObject::connect(cli, &CLI::response, this, &SettingsDialog::setUsername);

  CheckboxRow *checkboxRow =
      new CheckboxRow("Enable auto-sync by default", Settings::getInstance()->getAutoSyncDefault());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, this, &SettingsDialog::setAutoSyncDefault);
  layout->addWidget(checkboxRow);

  checkboxRow = new CheckboxRow(
      "If syncing fails due to lack of internet connection automatically retry next time you connect to WiFi",
      Settings::getInstance()->isRetryOnNetwork());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, this, &SettingsDialog::setRetryOnNetwork);
  layout->addWidget(checkboxRow);

  MenuRow *menuRow = new MenuRow("Sync annotations to reading journal", MenuRowType::Menu,
                                 {{"Always", "always"}, {"Never", "never"}, {"Once the book is finished", "finished"}},
                                 {}, Settings::getInstance()->getSyncBookmarks());
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setSyncBookmarks);
  layout->addWidget(menuRow);

  menuRow = new MenuRow(
      "Reading journal privacy", MenuRowType::Menu,
      {{"Account default", "account"}, {"Public", "public"}, {"Follows", "follows"}, {"Private", "private"}}, {},
      Settings::getInstance()->getJournalPrivacy());
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setJournalPrivacy);
  layout->addWidget(menuRow);

  return frame;
}

QFrame *SettingsDialog::buildAutoSync() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(new Label(Label::Avenir, "Auto-sync"));

  Settings *settings = Settings::getInstance();

  bool is24HourClock = settings->is24HourClock();

  QList<Item> hours;
  for (int hour = 1; hour <= 24; hour++) {
    QString text;

    if (is24HourClock) {
      text = QString("%1:00").arg(hour == 24 ? 0 : hour, 2, 10, QChar('0'));
    } else if (hour < 12) {
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

  MenuRow *menuRow =
      new MenuRow("Once per day", MenuRowType::Menu, {{"Never", 0}, {"Set time of day", MenuRow::OPEN_DIALOG}}, hours,
                  settings->getSyncDaily());
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setSyncDaily);
  layout->addWidget(menuRow);
  menuRow->setProperty("noBorder", true);

  QList<Item> thresholdItems;
  for (int i = 1; i < 100; i++) {
    thresholdItems.append({QString::number(i).append("%"), i});
  }

  menuRow = new MenuRow("After closing a book or the Kobo is put to sleep", MenuRowType::Menu,
                        {{"Always", 1}, {"Never", 0}, {"Set a threshold", MenuRow::OPEN_DIALOG}}, thresholdItems,
                        QVariant(settings->getCloseThreshold()));
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::setCloseThreshold);
  layout->addWidget(menuRow);

  menuRow = new MenuRow("Periodically by read percentage", MenuRowType::Menu,
                        {{"Never", 0}, {"Set a threshold", MenuRow::OPEN_DIALOG}}, thresholdItems,
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

  layout->addWidget(new Label(Label::Avenir, "Book information"));

  SyncController *ctl = SyncController::getInstance();
  QDateTime alarm = ctl->getAlarm();
  StaticRow *row =
      new StaticRow("Auto-sync scheduled for", alarm.isValid() ? alarm.toLocalTime().toString() : "Never", false);
  layout->addWidget(row);
  row->setProperty("noBorder", true);

  row = new StaticRow("Current progress", QString::number(ctl->getReadProgress()).append("%"), false);
  layout->addWidget(row);

  int progress = Settings::getInstance()->getLastProgress(ctl->contentId);
  row = new StaticRow("Last synced", progress <= 0 ? "Never" : QString::number(progress).append("%"), true);
  layout->addWidget(row);
  QObject::connect(row, &StaticRow::clear, this, &SettingsDialog::clearLastSynced);

  return frame;
}

QFrame *SettingsDialog::buildAdvanced() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(new Label(Label::Avenir, "Advanced"));

  CheckboxRow *checkboxRow = new CheckboxRow("Debug logs", Settings::getInstance()->getDebug());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, this, &SettingsDialog::setDebug);
  layout->addWidget(checkboxRow);
  checkboxRow->setProperty("noBorder", true);

  MenuRow *menuRow = new MenuRow("Save system logs", MenuRowType::Tap, {{"Save", true}}, {}, true);
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::saveLogs);
  layout->addWidget(menuRow);

  return frame;
}

void SettingsDialog::setUsername(QJsonObject doc) { username->setValue(doc.value("username").toString().prepend("@")); }

void SettingsDialog::setAutoSyncDefault(bool value) { Settings::getInstance()->setAutoSyncDefault(value); }

void SettingsDialog::setSyncBookmarks(QVariant value) { Settings::getInstance()->setSyncBookmarks(value.toString()); }

void SettingsDialog::setJournalPrivacy(QVariant value) { Settings::getInstance()->setJournalPrivacy(value.toString()); }

void SettingsDialog::setRetryOnNetwork(bool value) { Settings::getInstance()->setRetryOnNetwork(value); }

void SettingsDialog::setSyncDaily(QVariant value) { Settings::getInstance()->setSyncDaily(value.toInt()); }

void SettingsDialog::setCloseThreshold(QVariant value) { Settings::getInstance()->setCloseThreshold(value.toInt()); }

void SettingsDialog::setPageThreshold(QVariant value) { Settings::getInstance()->setPageThreshold(value.toInt()); }

void SettingsDialog::clearLastSynced() {
  QString contentId = SyncController::getInstance()->contentId;
  Settings *settings = Settings::getInstance();
  settings->setLastProgress(contentId, 0);

  StaticRow *row = qobject_cast<StaticRow *>(sender());
  int progress = settings->getLastProgress(contentId);
  row->setValue(progress <= 0 ? "Never" : QString::number(progress).append("%"));
}

void SettingsDialog::setDebug(bool value) { Settings::getInstance()->setDebug(value); }

void SettingsDialog::saveLogs() { nh_dump_log(); }
