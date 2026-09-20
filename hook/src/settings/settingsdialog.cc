#include <QDateTime>
#include <QTimer>
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

  QList<QFrame *> sections = {buildGeneral(), buildAutoSync(), buildQueuedProgress(), buildAdvanced()};
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

  username = new MenuRow("Loading…", MenuRowType::Tap, {{"Sign out", true}}, {}, true);
  QObject::connect(username, &MenuRow::triggered, this, &SettingsDialog::signOut);
  layout->addWidget(username);

  CLI *cli = CLI::getUser(true);
  QObject::connect(cli, &CLI::response, this, &SettingsDialog::setUsername);

  Settings *settings = Settings::getInstance();

  CheckboxRow *checkboxRow =
      new CheckboxRow("Enable auto-sync by default for new books", settings->getAutoSyncDefault());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, settings, &Settings::setAutoSyncDefault);
  layout->addWidget(checkboxRow);

  checkboxRow = new CheckboxRow("Retry sync when Wi-Fi reconnects", settings->getRetryOnNetwork());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, settings, &Settings::setRetryOnNetwork);
  layout->addWidget(checkboxRow);

  checkboxRow = new CheckboxRow("Sync Kobo annotations to the Hardcover.app journal", settings->getSyncAnnotations());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, settings, &Settings::setSyncAnnotations);
  layout->addWidget(checkboxRow);

  MenuRow *menuRow = new MenuRow(
      "Journal entry privacy", MenuRowType::Menu,
      {{"Account default", "account"}, {"Public", "public"}, {"Follows", "follows"}, {"Private", "private"}}, {},
      settings->getJournalPrivacy());
  QObject::connect(menuRow, &MenuRow::triggered, settings, &Settings::setJournalPrivacy);
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

  QList<Item> thresholdItems;
  for (int i = 1; i < 100; i++) {
    thresholdItems.append({QString::number(i).append("%"), i});
  }

  MenuRow *menuRow = new MenuRow("After closing a book or the Kobo is put to sleep", MenuRowType::Menu,
                                 {{"Every time", 1}, {"Never", -1}, {"Set a threshold", MenuRow::OPEN_DIALOG}},
                                 thresholdItems, settings->getSyncOnClose());
  QObject::connect(menuRow, &MenuRow::triggered, settings, &Settings::setSyncOnClose);
  menuRow->setProperty("noBorder", true);
  layout->addWidget(menuRow);

  menuRow = new MenuRow("Periodically by read percentage", MenuRowType::Menu,
                        {{"Never", -1}, {"Set a threshold", MenuRow::OPEN_DIALOG}}, thresholdItems,
                        settings->getSyncOnRead());
  QObject::connect(menuRow, &MenuRow::triggered, settings, &Settings::setSyncOnRead);
  layout->addWidget(menuRow);

  bool is24HourClock = settings->is24HourClock();

  QList<Item> hours;
  for (int hour = 0; hour <= 23; hour++) {
    QString text = is24HourClock ? QString("%1:00").arg(hour, 2, 10, QChar('0'))
                   : hour == 0   ? "12 AM"
                   : hour < 12   ? QString::number(hour).append(" AM")
                   : hour == 12  ? "12 PM"
                                 : QString::number(hour - 12).append(" PM");
    hours.append(Item{text, hour});
  }

  menuRow =
      new MenuRow("At a scheduled time each day", MenuRowType::Menu,
                  {{"Never", -1}, {"Set time of day", MenuRow::OPEN_DIALOG}}, hours, settings->getSyncOnSchedule());
  QObject::connect(menuRow, &MenuRow::triggered, settings, &Settings::setSyncOnSchedule);
  layout->addWidget(menuRow);

  QDateTime alarm = SyncController::getInstance()->getAlarm();
  layout->addWidget(
      new StaticRow("Next scheduled sync", alarm.isValid() ? alarm.toLocalTime().toString() : "Never", false));

  return frame;
}

QFrame *SettingsDialog::buildQueuedProgress() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  SyncController *ctl = SyncController::getInstance();
  const QHash<QString, int> &progress = ctl->getProgress();

  if (progress.isEmpty()) {
    return frame;
  }

  layout->addWidget(new Label(Label::Avenir, "Queued progress"));

  for (QHash<QString, int>::const_iterator it = progress.constBegin(); it != progress.constEnd(); ++it) {
    QString contentId = it.key();
    QString title = ctl->getBookInfo(contentId).title;

    int lastSynced = Settings::getInstance()->getLastProgress(contentId);
    QString value = lastSynced <= 0 ? QString("%1% • never synced").arg(it.value())
                                    : QString("%1% • last synced %2%").arg(it.value()).arg(lastSynced);

    StaticRow *row = new StaticRow(title.isEmpty() ? "Unknown" : title, value, true);
    QObject::connect(row, &StaticRow::clear, this, [contentId, row]() {
      SyncController::getInstance()->clearReadProgress(contentId);
      row->deleteLater();
    });

    if (it == progress.constBegin()) {
      row->setProperty("noBorder", true);
    }

    layout->addWidget(row);
  }

  return frame;
}

QFrame *SettingsDialog::buildAdvanced() {
  QFrame *frame = new QFrame(this);
  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->addWidget(new Label(Label::Avenir, "Advanced"));

  Settings *settings = Settings::getInstance();

  CheckboxRow *checkboxRow = new CheckboxRow("Enable debug logs", settings->getDebug());
  QObject::connect(checkboxRow, &CheckboxRow::triggered, settings, &Settings::setDebug);
  layout->addWidget(checkboxRow);
  checkboxRow->setProperty("noBorder", true);

  MenuRow *menuRow = new MenuRow("Save system logs", MenuRowType::Tap, {{"Save", true}}, {}, true);
  QObject::connect(menuRow, &MenuRow::triggered, this, &SettingsDialog::saveLogs);
  layout->addWidget(menuRow);

  return frame;
}

void SettingsDialog::setUsername(const Messages &message) {
  if (!message.isUser()) {
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Unexpected CLI response for <i>getUser</i>");
    return;
  }

  Optional<QString> usernameValue = message.user->username;

  if (usernameValue) {
    username->setHeading(usernameValue->prepend("@"));
  }
}

void SettingsDialog::signOut() {
  Settings::getInstance()->clearAccessToken();
  QTimer::singleShot(0, this, &SettingsDialog::closeDialog);
}
