#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

#include <NickelHook.h>

#include "../cli.h"
#include "../settings.h"
#include "../synccontroller.h"
#include "../widgets/buttongroup.h"
#include "editionrow.h"
#include "editionsdialog.h"

EditionsDialog *EditionsDialog::show(QString bookId) { return new EditionsDialog(bookId); }

EditionsDialog::EditionsDialog(QString bookId) : Dialog("Manually link book"), bookId(bookId) {
  setStyleSheet(R"(
    [qApp_deviceIsTrilogy=true] QStackedWidget {
      margin: 0 20px;
    }
    [qApp_deviceIsPhoenix=true] QStackedWidget {
      margin: 0 24px;
    }
    [qApp_deviceIsDragon=true] QStackedWidget {
      margin: 0 37px;
    }
    [qApp_deviceIsStorm=true] QStackedWidget {
      margin: 0 42px;
    }
    [qApp_deviceIsDaylight=true] QStackedWidget {
      margin: 0 48px;
    }

    [qApp_deviceIsTrilogy=true] ButtonGroup {
      margin: 0 20px;
    }
    [qApp_deviceIsPhoenix=true] ButtonGroup {
      margin: 0 24px;
    }
    [qApp_deviceIsDragon=true] ButtonGroup {
      margin: 0 37px;
    }
    [qApp_deviceIsStorm=true] ButtonGroup {
      margin: 0 42px;
    }
    [qApp_deviceIsDaylight=true] ButtonGroup {
      margin: 0 48px;
    }
  )");

  QObject::connect(dialog, SIGNAL(closeTapped()), this, SIGNAL(closed()));
  QObject::connect(dialog, SIGNAL(backTapped()), dialog, SLOT(deleteLater()));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  QHBoxLayout *hbox = new QHBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addLayout(hbox);

  ButtonGroup *formatGroup = new ButtonGroup({{"All", 0}, {"Print", 1}, {"E-Book", 4}}, readingFormat);
  hbox->addWidget(formatGroup);
  QObject::connect(formatGroup, &ButtonGroup::valueChanged, this, &EditionsDialog::readingFormatChanged);

  langButton = construct_N3ButtonLabel(this);
  langButton->setText("Any language");
  langButton->setProperty("primaryButton", true);
  hbox->addWidget(langButton);
  QObject::connect(langButton, SIGNAL(tapped(bool)), this, SLOT(showLangMenu()));

  hbox->addStretch(1);

  pages = new PagedStack(this);
  layout->addWidget(pages, 1);
  QObject::connect(pages, &PagedStack::requestPage, this, &EditionsDialog::requestPage);

  request();
}

void EditionsDialog::request() {
  offset = 0;
  editionsInitialized = false;
  pages->clear();

  CLI *cli = CLI::listEditions(bookId, readingFormat.toInt(), lang);
  QObject::connect(cli, &CLI::response, this, &EditionsDialog::response);
}

void EditionsDialog::response(QJsonObject doc) {
  languages = doc.value("languages").toArray();
  editions = doc.value("editions").toArray();
  editionsInitialized = true;
  pages->next();
}

void EditionsDialog::requestPage(int index) {
  if (!editionsInitialized) {
    return;
  }

  int length = editions.size();

  if (length < 1) {
    pages->setStatusText("No editions found.");
    return;
  }

  QWidget *page = new QWidget(pages);
  QVBoxLayout *vbox = new QVBoxLayout(page);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);

  int availableHeight = pages->getAvailableHeight();
  bool isFirst = true;
  for (; offset < length; offset++) {
    QJsonObject obj = editions.at(offset).toObject();
    EditionRow *row = new EditionRow(obj, this);

    if (isFirst) {
      row->setProperty("noBorder", true);
      isFirst = false;
    }

    availableHeight -= row->sizeHint().height();
    if (availableHeight < 0) {
      row->deleteLater();
      break;
    }

    vbox->addWidget(row);

    QObject::connect(row, &EditionRow::selected, this, &EditionsDialog::selected);
    QObject::connect(row, SIGNAL(selected(QString)), dialog, SLOT(deleteLater()));
  }

  vbox->addStretch(1);
  pages->addPage(page);

  if (index == 1 && offset > 0) {
    pages->setTotal(qCeil((float)length / offset));
  }
}

void EditionsDialog::showLangMenu() {
  QList<Item> items = {{"Any language", ""}};

  for (QJsonValue value : languages) {
    QString languages = value.toString();
    items.append({languages, languages});
  }

  NickelTouchMenu *menu = MenuController::showMenu(items, langButton, 0);
  QWidget::connect(menu, &QMenu::triggered, this, &EditionsDialog::langTriggered);
}

void EditionsDialog::langTriggered(QAction *action) {
  lang = action->data().toString();
  langButton->setText(lang.isEmpty() ? "Any language" : lang);
  request();
}

void EditionsDialog::readingFormatChanged(QVariant value) {
  readingFormat = value;
  request();
}
