#pragma once

#include "../messages.h"
#include "../widgets/dialog.h"

class SignInDialog : public Dialog {
  Q_OBJECT

public:
  static void show() { new SignInDialog(); }

public Q_SLOTS:
  void continueTapped();

private:
  SignInDialog();

  void response(const Messages &message);

  QImage buildQrCode(const QString &text);

  QString deviceCode;
};
