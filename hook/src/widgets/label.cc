#include "label.h"

const QString Label::Avenir = QStringLiteral("Avenir");
const QString Label::ExtraSmall = QStringLiteral("ExtraSmall");
const QString Label::Small = QStringLiteral("Small");
const QString Label::Medium = QStringLiteral("Medium");
const QString Label::Large = QStringLiteral("Large");
const QString Label::ExtraLarge = QStringLiteral("ExtraLarge");
const QString Label::Stylesheet = QStringLiteral(R"(
  [textSize="Avenir"] {
    font-family: Avenir, sans-serif;
    text-transform: uppercase;
  }

  [qApp_deviceIsTrilogy=true] [textSize="Avenir"] {
    font-size: 14px;
  }
  [qApp_deviceIsPhoenix=true] [textSize="Avenir"] {
    font-size: 17px;
  }
  [qApp_deviceIsDragon=true] [textSize="Avenir"] {
    font-size: 25px;
  }
  [qApp_deviceIsStorm=true] [textSize="Avenir"] {
    font-size: 29px;
  }
  [qApp_deviceIsDaylight=true] [textSize="Avenir"] {
    font-size: 32px;
  }

  [qApp_deviceIsTrilogy="true"] [textSize="ExtraSmall"] {
    font-size: 14px;
  }
  [qApp_deviceIsPhoenix="true"] [textSize="ExtraSmall"] {
    font-size: 18px;
  }
  [qApp_deviceIsDragon="true"] [textSize="ExtraSmall"] {
    font-size: 21px;
  }
  [qApp_deviceIsAlyssum="true"] [textSize="ExtraSmall"],
  [qApp_deviceIsNova="true"] [textSize="ExtraSmall"],
  [qApp_deviceIsStorm="true"] [textSize="ExtraSmall"] {
    font-size: 25px;
  }
  [qApp_deviceIsDaylight="true"] [textSize="ExtraSmall"] {
    font-size: 28px;
  }

  [qApp_deviceIsTrilogy="true"] [textSize="Small"] {
    font-size: 17px;
  }
  [qApp_deviceIsPhoenix="true"] [textSize="Small"] {
    font-size: 22px;
  }
  [qApp_deviceIsDragon="true"] [textSize="Small"] {
    font-size: 26px;
  }
  [qApp_deviceIsAlyssum="true"] [textSize="Small"],
  [qApp_deviceIsNova="true"] [textSize="Small"],
  [qApp_deviceIsStorm="true"] [textSize="Small"] {
    font-size: 30px;
  }
  [qApp_deviceIsDaylight="true"] [textSize="Small"] {
    font-size: 34px;
  }

  [qApp_deviceIsTrilogy="true"] [textSize="Medium"] {
    font-size: 19px;
  }
  [qApp_deviceIsPhoenix="true"] [textSize="Medium"] {
    font-size: 23px;
  }
  [qApp_deviceIsDragon="true"] [textSize="Medium"] {
    font-size: 29px;
  }
  [qApp_deviceIsAlyssum="true"] [textSize="Medium"],
  [qApp_deviceIsNova="true"] [textSize="Medium"] {
    font-size: 32px;
  }
  [qApp_deviceIsStorm="true"] [textSize="Medium"] {
    font-size: 34px;
  }
  [qApp_deviceIsDaylight="true"] [textSize="Medium"] {
    font-size: 37px;
  }

  [qApp_deviceIsTrilogy="true"] [textSize="Large"] {
    font-size: 23px;
  }
  [qApp_deviceIsPhoenix="true"] [textSize="Large"] {
    font-size: 28px;
  }
  [qApp_deviceIsDragon="true"] [textSize="Large"] {
    font-size: 36px;
  }
  [qApp_deviceIsAlyssum="true"] [textSize="Large"],
  [qApp_deviceIsNova="true"] [textSize="Large"] {
    font-size: 39px;
  }
  [qApp_deviceIsStorm="true"] [textSize="Large"] {
    font-size: 42px;
  }
  [qApp_deviceIsDaylight="true"] [textSize="Large"] {
    font-size: 47px;
  }

  [qApp_deviceIsTrilogy=true] [textSize="ExtraLarge"] {
    font-size: 30px;
  }
  [qApp_deviceIsPhoenix=true] [textSize="ExtraLarge"] {
    font-size: 36px;
  }
  [qApp_deviceIsDragon=true] [textSize="ExtraLarge"] {
    font-size: 46px;
  }
  [qApp_deviceIsAlyssum=true] [textSize="ExtraLarge"],
  [qApp_deviceIsNova=true] [textSize="ExtraLarge"] {
    font-size: 50px;
  }
  [qApp_deviceIsStorm=true] [textSize="ExtraLarge"] {
    font-size: 54px;
  }
  [qApp_deviceIsDaylight=true] [textSize="ExtraLarge"] {
    font-size: 60px;
  }

  [style="italic"] {
    font-style: italic;
  }
)");

Label::Label(const QString &textSize, const QString &text, QWidget *parent) : QLabel(text, parent), textSize(textSize) {
  setStyleSheet(Label::Stylesheet);
}
