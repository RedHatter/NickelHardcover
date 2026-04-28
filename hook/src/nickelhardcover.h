#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QTextEdit>
#include <QWidget>

typedef QObject MainWindowController;
extern MainWindowController *(*MainWindowController__sharedInstance)();
extern QWidget *(*MainWindowController__currentView)(MainWindowController *mwc);
extern QWidget *(*MainWindowController__pushView)(MainWindowController *mwc, QWidget *view);

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
KeyboardReceiver *construct_KeyboardReceiver(QLineEdit *lineEdit);
KeyboardReceiver *construct_KeyboardReceiver(QTextEdit *textEdit);

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
extern void (*N3Dialog__enableFullViewMode)(N3Dialog *__this);
extern void (*N3Dialog__showKeyboard)(N3Dialog *__this);
extern void (*N3Dialog__hideKeyboard)(N3Dialog *__this);

typedef QLineEdit TouchLineEdit;
TouchLineEdit *construct_TouchLineEdit(QWidget *parent);

typedef QWidget SettingContainer;
SettingContainer *construct_SettingContainer(QWidget *parent);

typedef QLabel TouchLabel;
TouchLabel *construct_TouchLabel(QWidget *parent);
extern void (*TouchLabel__setHitStateEnabled)(TouchLabel *_this, bool enabled);

typedef TouchLabel N3ButtonLabel;
N3ButtonLabel *construct_N3ButtonLabel(QWidget *parent);
extern void (*N3ButtonLabel__setPrimaryButton)(N3ButtonLabel *_this, bool enabled);

typedef QCheckBox TouchCheckBox;
TouchCheckBox *construct_TouchCheckBox(QWidget *parent);

typedef QMenu NickelTouchMenu;
NickelTouchMenu *construct_NickelTouchMenu(QWidget *parent);
extern void (*NickelTouchMenu__showDecoration)(NickelTouchMenu *_this, bool show);

typedef QWidget MenuTextItem;
MenuTextItem *construct_MenuTextItem(QWidget *parent, bool checkable, bool italic);
extern void (*MenuTextItem__setText)(MenuTextItem *_this, QString const &text);
extern void (*MenuTextItem__setSelected)(MenuTextItem *_this, bool selected);
extern void (*MenuTextItem__registerForTapGestures)(MenuTextItem *_this);

typedef QFrame TouchTextEdit;
TouchTextEdit *construct_TouchTextEdit(QWidget *parent);
extern void (*TouchTextEdit__setCustomPlaceholderText)(TouchTextEdit *__this, QString const &text);

typedef QObject PowerTimer;
PowerTimer *construct_PowerTimer(QString const &name, QObject *parent);
extern void (*PowerTimer__fireAt)(PowerTimer *__this, QDateTime const &datatime);
extern int (*PowerTimer__timeRemaining)(PowerTimer *__this);
