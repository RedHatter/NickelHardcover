#pragma once

#include "../messages.h"
#include "../widgets/dialog.h"

class SignInDialog : public Dialog {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(Messages message);
  void continueTapped();

private:
  SignInDialog();

  QImage buildQrCode(const QString &text);

  QString deviceCode;
};
