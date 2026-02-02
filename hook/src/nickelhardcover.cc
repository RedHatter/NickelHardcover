#include <QCoreApplication>
#include <QStackedWidget>
#include <QString>

#include <syslog.h>

#include <NickelHook.h>

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
WirelessManager *(*WirelessManager__sharedInstance)();

void (*ConfirmationDialogFactory__showErrorDialog)(QString const &title, QString const &content);
ConfirmationDialog *(*ConfirmationDialogFactory__getConfirmationDialog)(QWidget *parent);
void (*ConfirmationDialog__setTitle)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__setText)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__showCloseButton)(ConfirmationDialog *_this, bool enabled);

int (*ReadingView__getCalculatedReadProgressEv)(QWidget *_this);

WirelessWorkflowManager *(*WirelessWorkflowManager__sharedInstance)();
void (*WirelessWorkflowManager__connectWirelessSilently)(WirelessWorkflowManager *_this);
void (*WirelessWorkflowManager__connectWireless)(WirelessWorkflowManager *_this, bool, bool);
bool (*WirelessWorkflowManager__isInternetAccessible)(WirelessWorkflowManager *_this);

void (*ComboButton__addItem)(ComboButton *_this, QString const &label, QVariant const &data, bool);
void (*ComboButton__renameItem)(ComboButton *_this, int index, QString const &label);

typedef QWidget ReadingMenuView;
void (*ReadingMenuView__constructor)(ReadingMenuView *_this, QWidget *parent, bool unkown);

N3Dialog *(*N3DialogFactory__getDialog)(QWidget *content, bool idk);
void (*N3Dialog__disableCloseButton)(N3Dialog *__this);
void (*N3Dialog__enableBackButton)(N3Dialog *__this, bool enable);
void (*N3Dialog__setTitle)(N3Dialog *__this, QString const &);
KeyboardFrame *(*N3Dialog__keyboardFrame)(N3Dialog *__this);

PagingFooter *(*PagingFooter__constructor)(PagingFooter *__this, QWidget *parent);
void (*PagingFooter__setTotalPages)(PagingFooter *__this, int current);
void (*PagingFooter__setCurrentPage)(PagingFooter *__this, int current);

KeyboardReceiver *(*KeyboardReceiver__constructor)(KeyboardReceiver *__this, QLineEdit *parent, bool idk);
SearchKeyboardController *(*KeyboardFrame__createKeyboard)(KeyboardFrame *__this, int keyboardScript, QLocale locale);
void (*SearchKeyboardController__setReceiver)(SearchKeyboardController *__this, KeyboardReceiver *receiver, bool idk);
TouchLineEdit *(*TouchLineEdit__constructor)(TouchLineEdit *__this, QWidget *parent);

SettingContainer *(*SettingContainer__constructor)(SettingContainer *__this, QWidget *parent);
void (*SettingContainer__setShowBottomLine)(SettingContainer *__this, bool enabled);

static struct nh_info NickelHardcover = (struct nh_info){
    .name = "NickelHardcover",
    .desc = "Updates reading progress on Hardcover.app",
    .uninstall_flag = "/mnt/onboard/nickelhardcover_uninstall",
};

// clang-format off
static struct nh_hook NickelHardcoverHook[] = {
  { .sym = "_ZN17ReadingController9setVolumeERK6VolumeRK8Bookmark", .sym_new = "_nh_set_volume",                    .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingController__setVolume), .desc = "The main entry point" },
  { .sym = "_ZN15ReadingMenuViewC1EP7QWidgetb",                     .sym_new = "_nh_reading_menu_view_constructor", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingMenuView__constructor), .desc = "Used to inject menu items" },
  {0},
};

static struct nh_dlsym NickelHardcoverDlsym[] = {
  { .name = "_ZNK7Content5getIdEv",                                            .out = nh_symoutptr(Content__getId) },
  { .name = "_ZNK7Content8getTitleEv",                                         .out = nh_symoutptr(Content__getTitle) },
  { .name = "_ZNK7Content14getAttributionEv",                                  .out = nh_symoutptr(Content__getAttribution) },

  { .name = "_ZN20MainWindowController14sharedInstanceEv",                     .out = nh_symoutptr(MainWindowController__sharedInstance) },
  { .name = "_ZNK20MainWindowController11currentViewEv",                       .out = nh_symoutptr(MainWindowController__currentView) },
  { .name = "_ZN11ReadingView25getCalculatedReadProgressEv",                   .out = nh_symoutptr(ReadingView__getCalculatedReadProgressEv) },

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

  { .name = "_ZN11ComboButton7addItemERK7QStringRK8QVariantb",                 .out = nh_symoutptr(ComboButton__addItem) },
  { .name = "_ZN11ComboButton10renameItemEiRK7QString",                        .out = nh_symoutptr(ComboButton__renameItem) },

  { .name = "_ZN15N3DialogFactory9getDialogEP7QWidgetb",                       .out = nh_symoutptr(N3DialogFactory__getDialog) },
  { .name = "_ZN8N3Dialog18disableCloseButtonEv",                              .out = nh_symoutptr(N3Dialog__disableCloseButton) },
  { .name = "_ZN8N3Dialog16enableBackButtonEb",                                .out = nh_symoutptr(N3Dialog__enableBackButton) },
  { .name = "_ZN8N3Dialog8setTitleERK7QString",                                .out = nh_symoutptr(N3Dialog__setTitle) },
  { .name = "_ZN8N3Dialog13keyboardFrameEv",                                   .out = nh_symoutptr(N3Dialog__keyboardFrame) },

  { .name = "_ZN12PagingFooterC1EP7QWidget",                                   .out = nh_symoutptr(PagingFooter__constructor) },
  { .name = "_ZN12PagingFooter13setTotalPagesEi",                              .out = nh_symoutptr(PagingFooter__setTotalPages) },
  { .name = "_ZN12PagingFooter14setCurrentPageEi",                             .out = nh_symoutptr(PagingFooter__setCurrentPage) },

  { .name = "_ZN13TouchLineEditC1EP7QWidget",                                  .out = nh_symoutptr(TouchLineEdit__constructor) },
  { .name = "_ZN16KeyboardReceiverC1EP9QLineEditb",                            .out = nh_symoutptr(KeyboardReceiver__constructor) },
  { .name = "_ZN13KeyboardFrame14createKeyboardE14KeyboardScriptRK7QLocale",   .out = nh_symoutptr(KeyboardFrame__createKeyboard) },
  { .name = "_ZN24SearchKeyboardController11setReceiverEP16KeyboardReceiverb", .out = nh_symoutptr(SearchKeyboardController__setReceiver) },

  { .name = "_ZN16SettingContainerC1EP7QWidget",                               .out = nh_symoutptr(SettingContainer__constructor) },
  { .name = "_ZN16SettingContainer17setShowBottomLineEb",                      .out = nh_symoutptr(SettingContainer__setShowBottomLine) },

  {0},
};
// clang-format on

NickelHook(.init = nullptr, .info = &NickelHardcover, .hook = NickelHardcoverHook, .dlsym = NickelHardcoverDlsym);

QStackedWidget *stackedWidget = nullptr;

void handleStackedWidgetDestroyed() { stackedWidget = nullptr; }

extern "C" __attribute__((visibility("default"))) void _nh_set_volume(ReadingController *_this, Volume *volume, Bookmark *bookmark) {
  nh_log("ReadingController::setVolume(%p, %p, %p)", _this, volume, bookmark);

  SyncController *syncController = SyncController::getInstance();
  syncController->setContentId(Content__getId(volume));
  syncController->query = Content__getTitle(volume) + " " + Content__getAttribution(volume);

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

QObject *findChild(QObject *parent, const QString className) {
  QObjectList children = parent->children();

  for (int i = 0; i < children.size(); ++i) {
    QString child = QString(children[i]->metaObject()->className());
    if (child == className) {
      return children[i];
    }

    QObject *res = findChild(children[i], className);
    if (res) {
      return res;
    }
  }

  return nullptr;
}

extern "C" __attribute__((visibility("default"))) void _nh_reading_menu_view_constructor(ReadingMenuView *_this, QWidget *parent,
                                                                                         bool unknown) {
  nh_log("ReadingMenuView::ReadingMenuView(%p, %p, %s)", _this, parent, unknown ? "true" : "false");
  ReadingMenuView__constructor(_this, parent, unknown);

  QObject *child = findChild(_this, "ComboButton");

  if (child) {
    MenuController::getInstance()->setupItems(qobject_cast<ComboButton *>(child));
  } else {
    nh_log("Error: unable to find ComboButton");
  }
}
