#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QTextEdit>
#include <QWidget>

typedef void MainWindowController;
extern MainWindowController *(*MainWindowController__sharedInstance)();
extern QWidget *(*MainWindowController__currentView)(MainWindowController *mwc);

typedef QDialog N3Dialog;

typedef QDialog ConfirmationDialog;
extern void (*ConfirmationDialogFactory__showErrorDialog)(QString const &title, QString const &body);
extern ConfirmationDialog *(*ConfirmationDialogFactory__getConfirmationDialog)(QWidget *parent);
extern void (*ConfirmationDialog__setTitle)(ConfirmationDialog *_this, QString const &);
extern void (*ConfirmationDialog__setText)(ConfirmationDialog *_this, QString const &);

typedef QObject WirelessWorkflowManager;
extern WirelessWorkflowManager *(*WirelessWorkflowManager__sharedInstance)();
extern bool (*WirelessWorkflowManager__isInternetAccessible)(WirelessWorkflowManager *);
extern void (*WirelessWorkflowManager__connectWirelessSilently)(WirelessWorkflowManager *);
extern void (*WirelessWorkflowManager__connectWireless)(WirelessWorkflowManager *_this, bool, bool);
extern void (*ConfirmationDialog__showCloseButton)(ConfirmationDialog *_this, bool enabled);

typedef QObject WirelessManager;
extern WirelessManager *(*WirelessManager__sharedInstance)();

extern int (*ReadingView__getCalculatedReadProgress)(QWidget *_this);

typedef QFrame ComboButton;
extern void (*ComboButton__addItem)(ComboButton *_this, QString const &label, QVariant const &data, bool);
extern void (*ComboButton__renameItem)(ComboButton *_this, int index, QString const &label);

typedef QWidget KeyboardReceiver;
extern void (*KeyboardReceiver__constructor)(KeyboardReceiver *__this, QLineEdit *parent, bool idk);
extern void (*KeyboardReceiver__TextEdit_constructor)(KeyboardReceiver *__this, QTextEdit *parent,
                                                                   bool idk);

typedef QWidget KeyboardFrame;
typedef QObject SearchKeyboardController;
extern SearchKeyboardController *(*KeyboardFrame__createKeyboard)(KeyboardFrame *__this, int keyboardScript,
                                                                  QLocale locale);
extern void (*SearchKeyboardController__setReceiver)(SearchKeyboardController *__this, KeyboardReceiver *receiver,
                                                     bool idk);
extern void (*SearchKeyboardController__setGoText)(SearchKeyboardController *__this, QString const &text);

extern N3Dialog *(*N3DialogFactory__getDialog)(QWidget *content, bool idk);
extern void (*N3Dialog__disableCloseButton)(N3Dialog *__this);
extern void (*N3Dialog__enableBackButton)(N3Dialog *__this, bool enable);
extern void (*N3Dialog__setTitle)(N3Dialog *__this, QString const &);
extern KeyboardFrame *(*N3Dialog__keyboardFrame)(N3Dialog *__this);

typedef QWidget PagingFooter;
extern void (*PagingFooter__constructor)(PagingFooter *__this, QWidget *parent);
extern void (*PagingFooter__setTotalPages)(PagingFooter *__this, int current);
extern void (*PagingFooter__setCurrentPage)(PagingFooter *__this, int current);

typedef QLineEdit TouchLineEdit;
extern void (*TouchLineEdit__constructor)(TouchLineEdit *__this, QWidget *parent);

typedef QWidget SettingContainer;
extern void (*SettingContainer__constructor)(SettingContainer *__this, QWidget *parent);
extern void (*SettingContainer__setShowBottomLine)(SettingContainer *__this, bool enabled);

typedef QLabel TouchLabel;
extern void (*TouchLabel__constructor)(TouchLabel *_this, QWidget *parent, QFlags<Qt::WindowType>);
extern void (*TouchLabel__setHitStateEnabled)(TouchLabel *_this, bool enabled);

typedef TouchLabel N3ButtonLabel;
extern void (*N3ButtonLabel__constructor)(N3ButtonLabel *_this, QWidget *parent);
extern void (*N3ButtonLabel__setPrimaryButton)(N3ButtonLabel *_this, bool enabled);

typedef QCheckBox TouchCheckBox;
extern void (*TouchCheckBox__constructor)(TouchCheckBox *_this, QWidget *parent);

typedef QMenu NickelTouchMenu;
extern void (*NickelTouchMenu__constructor)(NickelTouchMenu *_this, QWidget *parent, int pos);
extern void (*NickelTouchMenu__showDecoration)(NickelTouchMenu *_this, bool show);

typedef QWidget MenuTextItem;
extern void (*MenuTextItem__constructor)(MenuTextItem *_this, QWidget *parent, bool checkable, bool italic);
extern void (*MenuTextItem__setText)(MenuTextItem *_this, QString const &text);
extern void (*MenuTextItem__setSelected)(MenuTextItem *_this, bool selected);
extern void (*MenuTextItem__registerForTapGestures)(MenuTextItem *_this);

typedef QFrame TouchTextEdit;
extern void (*TouchTextEdit__constructor)(TouchTextEdit *__this, QWidget *parent);
extern void (*TouchTextEdit__setCustomPlaceholderText)(TouchTextEdit *__this, QString const &text);
