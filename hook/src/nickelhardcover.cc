#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStackedWidget>
#include <QString>

#include <NickelHook.h>

#include "files.h"
#include "menucontroller.h"
#include "synccontroller.h"

typedef void ReadingController;
typedef void Volume;
typedef void Bookmark;
static void (*ReadingController__setVolume)(ReadingController *_this, Volume *volume, Bookmark *bookmark);
static QString (*Content__getId)(Volume *_this);
static QString (*Content__getTitle)(Volume *_this);
static QString (*Content__getAttribution)(Volume *_this);

MainWindowController *(*MainWindowController__sharedInstance)();
QWidget *(*MainWindowController__currentView)(MainWindowController *mwc);
QWidget *(*MainWindowController__pushView)(MainWindowController *mwc, QWidget *view);
WirelessManager *(*WirelessManager__sharedInstance)();

void (*ConfirmationDialogFactory__showErrorDialog)(QString const &title, QString const &content);
ConfirmationDialog *(*ConfirmationDialogFactory__getConfirmationDialog)(QWidget *parent);
void (*ConfirmationDialog__setTitle)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__setText)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__showCloseButton)(ConfirmationDialog *_this, bool enabled);

int (*ReadingView__getCalculatedReadProgress)(QWidget *_this);

WirelessWorkflowManager *(*WirelessWorkflowManager__sharedInstance)();
void (*WirelessWorkflowManager__connectWirelessSilently)(WirelessWorkflowManager *_this);
void (*WirelessWorkflowManager__connectWireless)(WirelessWorkflowManager *_this, bool, bool);
bool (*WirelessWorkflowManager__isInternetAccessible)(WirelessWorkflowManager *_this);

void (*TouchLabel__constructor)(TouchLabel *_this, QWidget *parent, QFlags<Qt::WindowType>);
void (*TouchLabel__setHitStateEnabled)(TouchLabel *_this, bool enabled);

TouchLabel *construct_TouchLabel(QWidget *parent) {
  TouchLabel *label = reinterpret_cast<TouchLabel *>(calloc(1, 128));
  TouchLabel__constructor(label, parent, 0);
  return label;
}

void (*N3ButtonLabel__constructor)(N3ButtonLabel *_this, QWidget *parent);
void (*N3ButtonLabel__setPrimaryButton)(N3ButtonLabel *_this, bool enabled);

N3ButtonLabel *construct_N3ButtonLabel(QWidget *parent) {
  N3ButtonLabel *button = reinterpret_cast<N3ButtonLabel *>(calloc(1, 512));
  N3ButtonLabel__constructor(button, parent);
  return button;
}

void (*TouchCheckBox__constructor)(TouchCheckBox *_this, QWidget *parent);

TouchCheckBox *construct_TouchCheckBox(QWidget *parent) {
  TouchCheckBox *checkbox = reinterpret_cast<TouchCheckBox *>(calloc(1, 128));
  TouchCheckBox__constructor(checkbox, parent);
  return checkbox;
}

void (*NickelTouchMenu__constructor)(NickelTouchMenu *_this, QWidget *parent, int pos);
void (*NickelTouchMenu__showDecoration)(NickelTouchMenu *_this, bool show);

NickelTouchMenu *construct_NickelTouchMenu(QWidget *parent) {
  NickelTouchMenu *menu = reinterpret_cast<NickelTouchMenu *>(calloc(1, 512));
  NickelTouchMenu__constructor(menu, parent, 0);
  return menu;
}

void (*MenuTextItem__constructor)(MenuTextItem *_this, QWidget *parent, bool checkable, bool italic);
void (*MenuTextItem__setText)(MenuTextItem *_this, QString const &text);
void (*MenuTextItem__setSelected)(MenuTextItem *_this, bool selected);
void (*MenuTextItem__registerForTapGestures)(MenuTextItem *_this);

MenuTextItem *construct_MenuTextItem(QWidget *parent, bool checkable, bool italic) {
  MenuTextItem *item = reinterpret_cast<MenuTextItem *>(calloc(1, 256));
  MenuTextItem__constructor(item, parent, checkable, italic);
  return item;
}

typedef QWidget ReadingMenuView;
void (*ReadingMenuView__constructor)(ReadingMenuView *_this, QWidget *parent, bool unknown);
void (*ReadingMenuView__constructor_2)(ReadingMenuView *_this, QWidget *, QByteArray const &unknownArray,
                                       bool unknownBool);

N3Dialog *(*N3DialogFactory__getDialog)(QWidget *content, bool unknown);
void (*N3Dialog__disableCloseButton)(N3Dialog *__this);
void (*N3Dialog__enableBackButton)(N3Dialog *__this, bool enable);
void (*N3Dialog__setTitle)(N3Dialog *__this, QString const &);
KeyboardFrame *(*N3Dialog__keyboardFrame)(N3Dialog *__this);
void (*N3Dialog__enableFullViewMode)(N3Dialog *__this);
void (*N3Dialog__showKeyboard)(N3Dialog *__this);
void (*N3Dialog__hideKeyboard)(N3Dialog *__this);

void (*KeyboardReceiver__constructor)(KeyboardReceiver *__this, QLineEdit *lineEdit, bool unknown);
void (*KeyboardReceiver__TextEdit_constructor)(KeyboardReceiver *__this, QTextEdit *parent, bool unknown);

KeyboardReceiver *construct_KeyboardReceiver(QLineEdit *lineEdit) {
  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__constructor(receiver, lineEdit, false);
  return receiver;
}

KeyboardReceiver *construct_KeyboardReceiver(QTextEdit *textEdit) {
  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 128));
  KeyboardReceiver__TextEdit_constructor(receiver, textEdit, false);
  return receiver;
}

SearchKeyboardController *(*KeyboardFrame__createKeyboard)(KeyboardFrame *__this, int keyboardScript, QLocale locale);
void (*SearchKeyboardController__setReceiver)(SearchKeyboardController *__this, KeyboardReceiver *receiver, bool idk);
void (*SearchKeyboardController__setGoText)(SearchKeyboardController *__this, QString const &text);

void (*TouchLineEdit__constructor)(TouchLineEdit *__this, QWidget *parent);

TouchLineEdit *construct_TouchLineEdit(QWidget *parent) {
  TouchLineEdit *lineEdit = reinterpret_cast<TouchLineEdit *>(calloc(1, 128));
  TouchLineEdit__constructor(lineEdit, parent);
  return lineEdit;
}

void (*SettingContainer__constructor)(SettingContainer *__this, QWidget *parent);

SettingContainer *construct_SettingContainer(QWidget *parent) {
  SettingContainer *container = reinterpret_cast<SettingContainer *>(calloc(1, 128));
  SettingContainer__constructor(container, parent);
  return container;
}

void (*TouchTextEdit__constructor)(TouchTextEdit *__this, QWidget *parent);
void (*TouchTextEdit__setCustomPlaceholderText)(TouchTextEdit *__this, QString const &text);

TouchTextEdit *construct_TouchTextEdit(QWidget *parent) {
  TouchTextEdit *touchText = reinterpret_cast<TouchTextEdit *>(calloc(1, 128));
  TouchTextEdit__constructor(touchText, parent);
  return touchText;
}

void (*PowerTimer__constructor)(PowerTimer *__this, QString const &name, QObject *parent);
void (*PowerTimer__fireAt)(PowerTimer *__this, QDateTime const &datatime);
int (*PowerTimer__timeRemaining)(PowerTimer *__this);

PowerTimer *construct_PowerTimer(QString const &name, QObject *parent) {
  PowerTimer *timer = reinterpret_cast<PowerTimer *>(calloc(1, 128));
  PowerTimer__constructor(timer, name, parent);
  return timer;
}

static struct nh_info NickelHardcover = (struct nh_info){.name = "NickelHardcover",
                                                         .desc = "Updates reading progress on Hardcover.app",
                                                         .uninstall_flag = "/mnt/onboard/nickelhardcover_uninstall",
                                                         .uninstall_xflag = "/mnt/onboard/.adds/NickelHardcover"};

// clang-format off
static struct nh_hook NickelHardcoverHook[] = {
  { .sym = "_ZN17ReadingController9setVolumeERK6VolumeRK8Bookmark", .sym_new = "_nh_set_volume",                      .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingController__setVolume),   .desc = "The main entry point" },
  { .sym = "_ZN15ReadingMenuViewC1EP7QWidgetb",                     .sym_new = "_nh_reading_menu_view_constructor",   .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingMenuView__constructor),   .desc = "Used to inject menu items", .optional = true },
  { .sym = "_ZN15ReadingMenuViewC1EP7QWidgetRK10QByteArrayb",       .sym_new = "_nh_reading_menu_view_constructor_2", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingMenuView__constructor_2), .desc = "Used to inject menu items", .optional = true }, // Version 4.44+
  {0},
};

static struct nh_dlsym NickelHardcoverDlsym[] = {
  { .name = "_ZNK7Content5getIdEv",                                            .out = nh_symoutptr(Content__getId) },
  { .name = "_ZNK7Content8getTitleEv",                                         .out = nh_symoutptr(Content__getTitle) },
  { .name = "_ZNK7Content14getAttributionEv",                                  .out = nh_symoutptr(Content__getAttribution) },

  { .name = "_ZN20MainWindowController14sharedInstanceEv",                     .out = nh_symoutptr(MainWindowController__sharedInstance) },
  { .name = "_ZNK20MainWindowController11currentViewEv",                       .out = nh_symoutptr(MainWindowController__currentView) },
  { .name = "_ZN20MainWindowController8pushViewEP7QWidget",                       .out = nh_symoutptr(MainWindowController__pushView) },
  { .name = "_ZN11ReadingView25getCalculatedReadProgressEv",                   .out = nh_symoutptr(ReadingView__getCalculatedReadProgress) },

  { .name = "_ZN25ConfirmationDialogFactory15showErrorDialogERK7QStringS2_",   .out = nh_symoutptr(ConfirmationDialogFactory__showErrorDialog) },
  { .name = "_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", .out = nh_symoutptr(ConfirmationDialogFactory__getConfirmationDialog) },
  { .name = "_ZN18ConfirmationDialog8setTitleERK7QString",                     .out = nh_symoutptr(ConfirmationDialog__setTitle) },
  { .name = "_ZN18ConfirmationDialog7setTextERK7QString",                      .out = nh_symoutptr(ConfirmationDialog__setText) },
  { .name = "_ZN18ConfirmationDialog15showCloseButtonEb",                      .out = nh_symoutptr(ConfirmationDialog__showCloseButton) },

  { .name = "_ZN23WirelessWorkflowManager14sharedInstanceEv",                  .out = nh_symoutptr(WirelessWorkflowManager__sharedInstance) },
  { .name = "_ZN23WirelessWorkflowManager20isInternetAccessibleEv",            .out = nh_symoutptr(WirelessWorkflowManager__isInternetAccessible) },
  { .name = "_ZN23WirelessWorkflowManager15connectWirelessEbb",                .out = nh_symoutptr(WirelessWorkflowManager__connectWireless) },
  { .name = "_ZN23WirelessWorkflowManager23connectWirelessSilentlyEv",         .out = nh_symoutptr(WirelessWorkflowManager__connectWirelessSilently) },
  { .name = "_ZN15WirelessManager14sharedInstanceEv",                          .out = nh_symoutptr(WirelessManager__sharedInstance) },

  { .name = "_ZN10TouchLabelC1EP7QWidget6QFlagsIN2Qt10WindowTypeEE",           .out = nh_symoutptr(TouchLabel__constructor) },
  { .name = "_ZN10TouchLabel18setHitStateEnabledEb",                           .out = nh_symoutptr(TouchLabel__setHitStateEnabled) },

  { .name = "_ZN13N3ButtonLabelC1EP7QWidget",                                  .out = nh_symoutptr(N3ButtonLabel__constructor) },
  { .name = "_ZN13N3ButtonLabel16setPrimaryButtonEb",                          .out = nh_symoutptr(N3ButtonLabel__setPrimaryButton) },

  { .name = "_ZN13TouchCheckBoxC1EP7QWidget",                                  .out = nh_symoutptr(TouchCheckBox__constructor) },

  { .name = "_ZN15NickelTouchMenuC1EP7QWidget18DecorationPosition",            .out = nh_symoutptr(NickelTouchMenu__constructor) },
  { .name = "_ZN15NickelTouchMenu14showDecorationEb",                          .out = nh_symoutptr(NickelTouchMenu__showDecoration) },
  { .name = "_ZN12MenuTextItemC1EP7QWidgetbb",                                 .out = nh_symoutptr(MenuTextItem__constructor) },
  { .name = "_ZN12MenuTextItem7setTextERK7QString",                            .out = nh_symoutptr(MenuTextItem__setText) },
  { .name = "_ZN12MenuTextItem11setSelectedEb",                                .out = nh_symoutptr(MenuTextItem__setSelected) },
  { .name = "_ZN12MenuTextItem22registerForTapGesturesEv",                     .out = nh_symoutptr(MenuTextItem__registerForTapGestures) },

  { .name = "_ZN15N3DialogFactory9getDialogEP7QWidgetb",                       .out = nh_symoutptr(N3DialogFactory__getDialog) },
  { .name = "_ZN8N3Dialog18disableCloseButtonEv",                              .out = nh_symoutptr(N3Dialog__disableCloseButton) },
  { .name = "_ZN8N3Dialog16enableBackButtonEb",                                .out = nh_symoutptr(N3Dialog__enableBackButton) },
  { .name = "_ZN8N3Dialog8setTitleERK7QString",                                .out = nh_symoutptr(N3Dialog__setTitle) },
  { .name = "_ZN8N3Dialog13keyboardFrameEv",                                   .out = nh_symoutptr(N3Dialog__keyboardFrame) },
  { .name = "_ZN8N3Dialog18enableFullViewModeEv",                              .out = nh_symoutptr(N3Dialog__enableFullViewMode) },
  { .name = "_ZN8N3Dialog12hideKeyboardEv",                                    .out = nh_symoutptr(N3Dialog__hideKeyboard) },
  { .name = "_ZN8N3Dialog12showKeyboardEv",                                    .out = nh_symoutptr(N3Dialog__showKeyboard) },

  { .name = "_ZN13TouchLineEditC1EP7QWidget",                                  .out = nh_symoutptr(TouchLineEdit__constructor) },
  { .name = "_ZN16KeyboardReceiverC1EP9QLineEditb",                            .out = nh_symoutptr(KeyboardReceiver__constructor) },
  { .name = "_ZN16KeyboardReceiverC1EP9QTextEditb",                            .out = nh_symoutptr(KeyboardReceiver__TextEdit_constructor) },
  { .name = "_ZN13KeyboardFrame14createKeyboardE14KeyboardScriptRK7QLocale",   .out = nh_symoutptr(KeyboardFrame__createKeyboard) },
  { .name = "_ZN24SearchKeyboardController11setReceiverEP16KeyboardReceiverb", .out = nh_symoutptr(SearchKeyboardController__setReceiver) },
  { .name = "_ZN24SearchKeyboardController9setGoTextERK7QString",              .out = nh_symoutptr(SearchKeyboardController__setGoText) },

  { .name = "_ZN16SettingContainerC1EP7QWidget",                               .out = nh_symoutptr(SettingContainer__constructor) },

  { .name = "_ZN13TouchTextEditC1EP7QWidget",                                  .out = nh_symoutptr(TouchTextEdit__constructor) },
  { .name = "_ZN13TouchTextEdit24setCustomPlaceholderTextERK7QString",         .out = nh_symoutptr(TouchTextEdit__setCustomPlaceholderText) },

  { .name = "_ZN10PowerTimerC1ERK7QStringP7QObject",                           .out = nh_symoutptr(PowerTimer__constructor) },
  { .name = "_ZN10PowerTimer6fireAtERK9QDateTime",                             .out = nh_symoutptr(PowerTimer__fireAt) },
  { .name = "_ZNK10PowerTimer13timeRemainingEv",                               .out = nh_symoutptr(PowerTimer__timeRemaining) },

  {0},
};
// clang-format on

bool hardcover_uninstall() {
  nh_delete_file(Files::config);
  nh_delete_file(Files::settings);
  nh_delete_file(Files::cli);
  nh_delete_dir(Files::adds_directory);

  return true;
}

NickelHook(.init = nullptr, .info = &NickelHardcover, .hook = NickelHardcoverHook, .dlsym = NickelHardcoverDlsym,
           .uninstall = &hardcover_uninstall);

QStackedWidget *stackedWidget = nullptr;

void handleStackedWidgetDestroyed() { stackedWidget = nullptr; }

extern "C" __attribute__((visibility("default"))) void _nh_set_volume(ReadingController *_this, Volume *volume,
                                                                      Bookmark *bookmark) {
  nh_log("ReadingController::setVolume(%p, %p, %p)", _this, volume, bookmark);

  SyncController *syncController = SyncController::getInstance();
  syncController->contentId = Content__getId(volume);
  syncController->title = Content__getTitle(volume);
  syncController->author = Content__getAttribution(volume);

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);

  if (!stackedWidget) {
    if (QString(cv->parentWidget()->metaObject()->className()) == "QStackedWidget") {
      stackedWidget = static_cast<QStackedWidget *>(cv->parentWidget());
      QObject::connect(stackedWidget, &QStackedWidget::currentChanged, SyncController::getInstance(),
                       &SyncController::currentViewIndexChanged);
      QObject::connect(stackedWidget, &QObject::destroyed, handleStackedWidgetDestroyed);
    } else {
      nh_log("Error: expected QStackedWidget, got %s", cv->parentWidget()->metaObject()->className());
    }
  }

  return ReadingController__setVolume(_this, volume, bookmark);
}

void injectMenuWidget(ReadingMenuView *parent) {
  QHBoxLayout *childLayout = parent->findChild<QHBoxLayout *>("bottomHorizontalLayout");

  if (childLayout) {
    MenuController *ctl = new MenuController(parent);
    childLayout->insertWidget(childLayout->count() - 1, ctl->icon);
  } else {
    nh_log("Error: unable to find bottomHorizontalLayout");
  }
}

extern "C" __attribute__((visibility("default"))) void
_nh_reading_menu_view_constructor(ReadingMenuView *_this, QWidget *parent, bool unknown) {
  nh_log("ReadingMenuView::ReadingMenuView(%p, %p, %s)", _this, parent, unknown ? "true" : "false");
  ReadingMenuView__constructor(_this, parent, unknown);
  injectMenuWidget(parent);
}

extern "C" __attribute__((visibility("default"))) void
_nh_reading_menu_view_constructor_2(ReadingMenuView *_this, QWidget *parent, QByteArray const &unknownArray,
                                    bool unknownBool) {
  nh_log("ReadingMenuView::ReadingMenuView(%p, %p, %s)", _this, parent, unknownBool ? "true" : "false");
  ReadingMenuView__constructor_2(_this, parent, unknownArray, unknownBool);
  injectMenuWidget(parent);
}
