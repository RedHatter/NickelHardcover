#include <QVBoxLayout>

#include <NickelHook.h>

#include "../settings.h"
#include "settingsdialog.h"
#include "settingsrow.h"

void SettingsDialog::show() { new SettingsDialog(); }

SettingsDialog::SettingsDialog() : Dialog("Settings") {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(QMargins(84, 56, 84, 0));

  setStyleSheet(R"(
    QLabel {
      font-size: 38px;
    }

    QLabel#header {
      font-size: 60px;
      border-bottom: 1px solid black;
      padding: 0 10px 26px;
    }

    QLabel#section {
      font-family: Avenir, sans-serif;
      font-size: 32px;
      padding: 5px 26px;
      background-color: #d9d9d9;
    }

    QLabel#value {
      font-style: italic;
    }
  )");

  QLabel *label = new QLabel("Settings");
  label->setObjectName("header");
  layout->addWidget(label);

  label = new QLabel("GENERAL");
  label->setObjectName("section");
  layout->addWidget(label);

  SettingsRow *menuRow =
      new SettingsRow("Enable auto-sync by default", SettingsRowType::Toggle, {Item{"Yes", true}, Item{"No", false}},
                      {}, Settings::getInstance()->getAutoSyncDefault());
  QObject::connect(menuRow, &SettingsRow::triggered, this, &SettingsDialog::setAutoSyncDefault);
  layout->addWidget(menuRow);

  menuRow =
      new SettingsRow("Sync annotations to reading journal", SettingsRowType::Menu,
                      {Item{"Always", "always"}, Item{"Never", "never"}, Item{"Once the book is finished", "finished"}},
                      {}, Settings::getInstance()->getSyncBookmarks());
  QObject::connect(menuRow, &SettingsRow::triggered, this, &SettingsDialog::setSyncBookmarks);
  layout->addWidget(menuRow);

  label = new QLabel("AUTO-SYNC");
  label->setObjectName("section");
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

  menuRow = new SettingsRow("Once per day", SettingsRowType::Menu,
                            {Item{"Never", 0}, Item{"Set time of day", SettingsRow::OPEN_DIALOG}}, hours,
                            Settings::getInstance()->getSyncDaily());
  QObject::connect(menuRow, &SettingsRow::triggered, this, &SettingsDialog::setSyncDaily);
  layout->addWidget(menuRow);

  QList<Item> thresholdItems;
  for (int i = 1; i < 100; i++) {
    thresholdItems.append(Item{QString::number(i).append("%"), i});
  }

  menuRow = new SettingsRow("After closing a book or the Kobo is put to sleep", SettingsRowType::Menu,
                            {Item{"Always", 1}, Item{"Never", 0}, Item{"Set a threshold", SettingsRow::OPEN_DIALOG}},
                            thresholdItems, QVariant(Settings::getInstance()->getCloseThreshold()));
  QObject::connect(menuRow, &SettingsRow::triggered, this, &SettingsDialog::setCloseThreshold);
  layout->addWidget(menuRow);

  menuRow = new SettingsRow("Periodically by read percentage", SettingsRowType::Menu,
                            {Item{"Never", 0}, Item{"Set a threshold", SettingsRow::OPEN_DIALOG}}, thresholdItems,
                            QVariant(Settings::getInstance()->getPageThreshold()), false);
  QObject::connect(menuRow, &SettingsRow::triggered, this, &SettingsDialog::setPageThreshold);
  layout->addWidget(menuRow);

  dialog->show();
}

void SettingsDialog::setAutoSyncDefault(QVariant value) { Settings::getInstance()->setAutoSyncDefault(value.toBool()); }

void SettingsDialog::setSyncBookmarks(QVariant value) { Settings::getInstance()->setSyncBookmarks(value.toString()); }

void SettingsDialog::setSyncDaily(QVariant value) { Settings::getInstance()->setSyncDaily(value.toInt()); }

void SettingsDialog::setCloseThreshold(QVariant value) { Settings::getInstance()->setCloseThreshold(value.toInt()); }

void SettingsDialog::setPageThreshold(QVariant value) { Settings::getInstance()->setPageThreshold(value.toInt()); }
