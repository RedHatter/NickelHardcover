#include <QCoreApplication>
#include <QStackedWidget>
#include <QString>

#include <syslog.h>

#include <NickelHook.h>

#include "cli.h"
#include "menucontroller.h"
#include "settings.h"

const QString PATH = "/mnt/onboard/.adds/NickelHardcover/";

typedef void ReadingController;
typedef void Volume;
typedef void Bookmark;
static void (*ReadingController__setVolume)(ReadingController *_this, Volume *volume, Bookmark *bookmark);
static QString (*Content__getId)(Volume *_this);

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
  { .name = "_ZNK7Content5getIdEv", .out = nh_symoutptr(Content__getId)},

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

  {0},
};
// clang-format on

NickelHook(.init = nullptr, .info = &NickelHardcover, .hook = NickelHardcoverHook, .dlsym = NickelHardcoverDlsym);

QStackedWidget *stackedWidget = nullptr;

void handleStackedWidgetDestroyed() { stackedWidget = nullptr; }

extern "C" __attribute__((visibility("default"))) void _nh_set_volume(ReadingController *_this, Volume *volume, Bookmark *bookmark) {
  nh_log("ReadingController::setVolume(%p, %p, %p)", _this, volume, bookmark);

  Settings::getInstance()->setContentId(Content__getId(volume));

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);

  if (!stackedWidget) {
    if (QString(cv->parentWidget()->metaObject()->className()) == "QStackedWidget") {
      stackedWidget = static_cast<QStackedWidget *>(cv->parentWidget());
      QObject::connect(stackedWidget, &QStackedWidget::currentChanged, CLI::getInstance(), &CLI::currentViewChanged);
      QObject::connect(stackedWidget, &QObject::destroyed, handleStackedWidgetDestroyed);
    } else {
      nh_log("expected QStackedWidget, got %s", cv->parentWidget()->metaObject()->className());
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
