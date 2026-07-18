#include <QHBoxLayout>
#include <QWidgetAction>

#include <NickelHook.h>

#include "../widgets/label.h"
#include "checkboxrow.h"

CheckboxRow::CheckboxRow(QString heading, bool checked, QWidget *parent) : QWidget(parent), checked(checked) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  SettingContainer *row = construct_SettingContainer(this);
  layout->addWidget(row);

  QHBoxLayout *rowLayout = new QHBoxLayout(row);
  rowLayout->setContentsMargins(0, 0, 0, 0);
  Label *headingLabel = new Label(Label::Medium, heading);
  headingLabel->setWordWrap(true);
  rowLayout->addWidget(headingLabel, 1);

  checkbox = construct_TouchCheckBox(this);
  checkbox->setText("On");
  checkbox->setChecked(checked);
  checkbox->setAttribute(Qt::WA_TransparentForMouseEvents);
  rowLayout->addWidget(checkbox);
  QObject::connect(row, SIGNAL(tapped()), this, SLOT(tapped()));
}

void CheckboxRow::tapped() {
  nh_log("CheckboxRow::tapped()");

  checked = !checked;
  checkbox->setChecked(checked);
  triggered(checked);
}
