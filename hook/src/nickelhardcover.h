#include <QDialog>
#include <QFrame>
#include <QWidget>

typedef void MainWindowController;
extern MainWindowController *(*MainWindowController__sharedInstance)();
extern QWidget *(*MainWindowController__currentView)(MainWindowController *mwc);

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

extern int (*ReadingView__getCalculatedReadProgressEv)(QWidget *_this);

typedef QFrame ComboButton;
extern void (*ComboButton__addItem)(ComboButton *_this, QString const &label, QVariant const &data, bool);
extern void (*ComboButton__renameItem)(ComboButton *_this, int index, QString const &label);

extern const QString PATH;
