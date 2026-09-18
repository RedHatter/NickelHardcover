#include <QImage>
#include <QPixmap>
#include <QVBoxLayout>

#include <NickelHook.h>
#include <qrcodegen.h>

#include "../cli.h"
#include "../widgets/label.h"
#include "qboxlayout.h"
#include "qnamespace.h"
#include "signindialog.h"

void SignInDialog::show() { new SignInDialog(); }

SignInDialog::SignInDialog() : Dialog("Sign in to Hardcover.app") {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  Label *status = new Label(Label::Small, "Loading. Please wait...");
  layout->addWidget(status, 1, Qt::AlignCenter);

  CLI *cli = CLI::oauthRequest();
  QObject::connect(cli, &CLI::response, this, &SignInDialog::response);
  QObject::connect(cli, &CLI::failure, this, &SignInDialog::closeDialog);
}

void SignInDialog::response(Messages message) {
  if (!message.isOAuthDevice()) {
    ConfirmationDialogFactory__showErrorDialog("Hardcover.app", "Unexpected CLI response for <i>oauthRequest</i>");
    return;
  }

  deviceCode = message.o_auth_device->device_code;

  QVBoxLayout *box = qobject_cast<QVBoxLayout *>(layout());
  box->itemAt(0)->widget()->deleteLater();

  box->addStretch(1);

  QLabel *qrLabel = new QLabel(this);
  qrLabel->setPixmap(QPixmap::fromImage(buildQrCode(message.o_auth_device->verification_uri_complete)));
  box->addWidget(qrLabel, 0, Qt::AlignCenter);

  QString text = QString("<br>Please scan this QR code<br><br>— or —<br><br>Visit <b>%1</b> and enter<br>")
                     .arg(message.o_auth_device->verification_uri);
  QLabel *label = new Label(Label::Medium, text);
  label->setAlignment(Qt::AlignCenter);
  box->addWidget(label, 0, Qt::AlignCenter);

  box->addWidget(new Label(Label::AvenirExtraLarge, message.o_auth_device->user_code + "<br>"), 0, Qt::AlignCenter);

  N3ButtonLabel *button = construct_N3ButtonLabel(this);
  button->setProperty("primaryButton", true);
  button->setText("Continue");
  box->addWidget(button, 0, Qt::AlignCenter);
  QObject::connect(button, SIGNAL(tapped(bool)), this, SLOT(continueTapped()));

  box->addStretch(1);
}

QImage SignInDialog::buildQrCode(const QString &text) {
  constexpr int maxVersion = 4;
  constexpr int scale = 16;
  constexpr size_t bufLen = qrcodegen_BUFFER_LEN_FOR_VERSION(maxVersion);

  uint8_t qrcode[bufLen];
  uint8_t tempBuffer[bufLen];

  QByteArray utf8 = text.toUtf8();

  bool ok = qrcodegen_encodeText(utf8.constData(), tempBuffer, qrcode, qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN,
                                 maxVersion, qrcodegen_Mask_AUTO, true);

  if (!ok) {
    return QImage();
  }

  int size = qrcodegen_getSize(qrcode);
  int imgSize = size * scale;

  QImage image(imgSize, imgSize, QImage::Format_RGB32);
  image.fill(Qt::white);

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      if (qrcodegen_getModule(qrcode, x, y)) {
        for (int dy = 0; dy < scale; dy++) {
          for (int dx = 0; dx < scale; dx++) {
            image.setPixel(x * scale + dx, y * scale + dy, qRgb(0, 0, 0));
          }
        }
      }
    }
  }
  return image;
}

void SignInDialog::continueTapped() {
  CLI *cli = CLI::oauthSet(deviceCode);
  QObject::connect(cli, &CLI::success, this, &Dialog::closeDialog);
  QObject::connect(cli, &CLI::failure, this, &Dialog::closeDialog);
}
