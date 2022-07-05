/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef QALCULATE_WINDOW_H
#define QALCULATE_WINDOW_H

#include <QMainWindow>
#include <QFont>
#include <QSpinBox>
#include <QTranslator>
#include <libqalculate/qalculate.h>

class QLocalSocket;
class QLocalServer;
class QCommandLineParser;
class ExpressionEdit;
class HistoryView;
class QSplitter;
class QLabel;
class KeypadWidget;
class QDockWidget;
class QAction;
class QToolBar;
class QTextEdit;
class QToolButton;
class QTimer;
class PreferencesDialog;
class FunctionsDialog;
class DataSetsDialog;
class VariablesDialog;
class UnitsDialog;
class FPConversionDialog;
class PercentageCalculationDialog;
class PlotDialog;
class PeriodicTableDialog;
class CalendarConversionDialog;
class QTableWidget;
class QMenu;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QLabel;
class QDialog;
class QComboBox;
struct FunctionDialog;
struct keyboard_shortcut;

class QalculateWindow : public QMainWindow {

	Q_OBJECT

	public:

		QalculateWindow();
		virtual ~QalculateWindow();

		void setCommandLineParser(QCommandLineParser*);
		bool displayMessages();
		bool updateWindowTitle(const QString &str = QString(), bool is_result = false, bool type_change = false);
		void executeFromFile(const QString&);

	protected:

		QLocalSocket *socket;
		QLocalServer *server;
		QCommandLineParser *parser;

		Thread *viewThread, *commandThread;

		ExpressionEdit *expressionEdit;
		HistoryView *historyView;
		QSplitter *ehSplitter;
		QLabel *statusLabel, *statusIconLabel;
		PreferencesDialog *preferencesDialog;
		DataSetsDialog *datasetsDialog;
		FunctionsDialog *functionsDialog;
		VariablesDialog *variablesDialog;
		UnitsDialog *unitsDialog;
		FPConversionDialog *fpConversionDialog;
		PercentageCalculationDialog *percentageDialog;
		PlotDialog *plotDialog;
		PeriodicTableDialog *periodicTableDialog;
		CalendarConversionDialog *calendarConversionDialog;
		QDialog *shortcutsDialog, *shortcutActionDialog;
		QComboBox *shortcutActionValueEdit; QListWidget *shortcutActionList; QLabel *shortcutActionValueLabel;
		QTreeWidget *shortcutList; QPushButton *addShortcutButton, *editShortcutButton, *removeShortcutButton, *shortcutActionOKButton;

		KeypadWidget *keypad;
		QDockWidget *keypadDock, *basesDock, *rpnDock;
		QLabel *binEdit, *octEdit, *decEdit, *hexEdit;
		QLabel *binLabel, *octLabel, *decLabel, *hexLabel;
		QToolBar *tb;
		QToolButton *menuAction_t, *modeAction_t, *keypadAction_t;
		QAction *toAction, *storeAction, *functionsAction_t, *unitsAction_t, *plotAction_t, *basesAction, *customOutputBaseAction, *customInputBaseAction, *newVariableAction, *newFunctionAction, *variablesAction, *functionsAction, *unitsAction, *datasetsAction, *plotAction, *fpAction, *calendarsAction, *percentageAction, *periodicTableAction, *exratesAction, *quitAction, *helpAction, *keypadAction, *rpnAction, *chainAction, *gKeypadAction, *pKeypadAction, *xKeypadAction, *cKeypadAction, *hideNumpadAction, *radAction, *degAction, *graAction, *normalAction, *sciAction, *engAction, *simpleAction;
		QMenu *variablesMenu, *functionsMenu, *unitsMenu;
		QAction *assumptionTypeActions[5], *assumptionSignActions[6];
		QMenu *recentWSMenu;
		QAction *recentWSSeparator, *openWSAction, *defaultWSAction, *saveWSAction, *saveWSAsAction;
		QList<QAction*> recentWSAction;
		QSpinBox *customOutputBaseEdit, *customInputBaseEdit;
		QTimer *ecTimer, *rfTimer;
		QFont saved_app_font;

		QTableWidget *rpnView;
		QAction *rpnUpAction, *rpnDownAction, *rpnSwapAction, *rpnCopyAction, *rpnLastxAction, *rpnDeleteAction, *rpnClearAction;

		bool send_event;
		bool workspace_changed;

		void calculateExpression(bool force = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, std::string execute_str = std::string(), std::string str = std::string(), bool check_exrates = true);
		void setResult(Prefix *prefix = NULL, bool update_history = true, bool update_parse = false, bool force = false, std::string transformation = "", bool do_stack = false, size_t stack_index = 0, bool register_moved = false, bool supress_dialog = false);
		void executeCommand(int command_type, bool show_result = true, std::string ceu_str = "", Unit *u = NULL, int run = 1);
		void changeEvent(QEvent *e) override;
		bool askTC(MathStructure&);
		bool askSinc(MathStructure&);
		bool askDot(const std::string&);
		bool askImplicit();
		void keyPressEvent(QKeyEvent*) override;
		void closeEvent(QCloseEvent*) override;
		void setPreviousExpression();
		void setOption(std::string);
		void updateResultBases();
		void calculateRPN(MathFunction*);
		void RPNRegisterAdded(std::string, int = 0);
		void RPNRegisterRemoved(int);
		void RPNRegisterChanged(std::string, int);
		void triggerShortcut(int, const std::string&);
		void loadShortcuts();
		bool editKeyboardShortcut(keyboard_shortcut*, keyboard_shortcut* = NULL, int = 0);
		void loadWorkspace(const QString &filename);
		void updateWSActions();
		int askSaveWorkspace();

	protected slots:

		void onSymbolClicked(const QString&);
		void onOperatorClicked(const QString&);
		void onFunctionClicked(MathFunction*);
		void onVariableClicked(Variable*);
		void onUnitClicked(Unit*);
		void onClearClicked();
		void onDelClicked();
		void onEqualsClicked();
		void onParenthesesClicked();
		void onBracketsClicked();
		void onBackspaceClicked();
		void onLeftClicked();
		void onRightClicked();
		void onStartClicked();
		void onEndClicked();
		void onMSClicked();
		void onMRClicked();
		void onMCClicked();
		void onMPlusClicked();
		void onMMinusClicked();
		void onAnswerClicked();
		void onBaseClicked(int, bool);
		void onFactorizeClicked();
		void onExpandClicked();
		void onToActivated();
		void onStoreActivated();
		void keypadTypeActivated();
		void hideNumpad(bool);
		void onKeypadVisibilityChanged(bool);
		void onBasesActivated(bool);
		void onBasesVisibilityChanged(bool);
		void onRPNVisibilityChanged(bool);
		void onExpressionChanged();
		void onToConversionRequested(std::string);
		void onInsertTextRequested(std::string);
		void onInsertValueRequested(int);
		void onAlwaysOnTopChanged();
		void onPreferencesClosed();
		void onTitleTypeChanged();
		void onResultFontChanged();
		void onExpressionFontChanged();
		void onKeypadFontChanged();
		void onAppFontChanged();
		void gradiansActivated();
		void radiansActivated();
		void degreesActivated();
		void normalActivated();
		void scientificActivated();
		void engineeringActivated();
		void simpleActivated();
		void onPrecisionChanged(int);
		void onMinDecimalsChanged(int);
		void onMaxDecimalsChanged(int);
		void outputBaseActivated();
		void onCustomOutputBaseChanged(int);
		void inputBaseActivated();
		void onCustomInputBaseChanged(int);
		void assumptionsTypeActivated();
		void assumptionsSignActivated();
		void approximationActivated();
		void openCalendarConversion();
		void onInsertFunctionExec();
		void onInsertFunctionRPN();
		void onInsertFunctionInsert();
		void onInsertFunctionKeepOpen(bool);
		void onInsertFunctionClosed();
		void onInsertFunctionChanged();
		void onInsertFunctionEntryActivated();
		void insertFunctionDo(FunctionDialog*);
		void onEntrySelectFile();
		void onEntryEditMatrix();
		void onCalculateFunctionRequested(MathFunction*);
		void onInsertFunctionRequested(MathFunction*);
		void onUnitActivated(Unit *u);
		void onUnitRemoved(Unit*);
		void onVariableRemoved(Variable*);
		void onUnitDeactivated(Unit*);
		void onVariableDeactivated(Variable*);
		void normalModeActivated();
		void rpnModeActivated();
		void chainModeActivated();
		void registerUp();
		void registerDown();
		void registerSwap();
		void copyRegister();
		void rpnLastX();
		void deleteRegister();
		void clearStack();
		void registerChanged(int);
		void calculateRPN(int);
		void approximateResult();
		void onExpressionStatusModeChanged();
		void functionActivated();
		void unitActivated();
		void variableActivated();
		void updateFunctionsMenu();
		void updateUnitsMenu();
		void updateVariablesMenu();
		void shortcutActivated();
		void shortcutClicked(int, const QString&);
		void keyboardShortcutAdded(keyboard_shortcut *ks);
		void keyboardShortcutRemoved(keyboard_shortcut *ks);
		void addShortcutClicked();
		void editShortcutClicked();
		void removeShortcutClicked();
		void shortcutActionOKClicked();
		void updateShortcutActionOK();
		void currentShortcutActionChanged(QListWidgetItem*, QListWidgetItem*);
		void currentShortcutChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void shortcutDoubleClicked(QTreeWidgetItem*, int);
		void saveWorkspace();
		void saveWorkspaceAs();
		void openWorkspace();
		void openDefaultWorkspace();
		void openRecentWorkspace();


	public slots:

		void serverNewConnection();
		void socketReadyRead();
		void calculate();
		void calculate(const QString&);
		void onActivateRequested(const QStringList&, const QString&);
		void abort();
		void abortCommand();
		void fetchExchangeRates();
		void showAbout();
		void expressionCalculationUpdated(int delay = 0);
		void resultFormatUpdated(int delay = 0);
		void resultDisplayUpdated();
		void expressionFormatUpdated(bool);
		void insertFunction(MathFunction*, QWidget* = NULL);
		void newVariable();
		void newUnit();
		void newMatrix();
		void newUnknown();
		void newFunction();
		void convertToUnit(Unit*);
		void importCSV();
		void exportCSV();
		void editKeyboardShortcuts();
		void editPreferences();
		void onDatasetsChanged();
		void onFunctionsChanged();
		void insertProperty(DataObject*, DataProperty*);
		void openDatasets();
		void openFunctions();
		void openUnits();
		void openVariables();
		void applyFunction(MathFunction*);
		void openFPConversion();
		void openPercentageCalculation();
		void openPlot();
		void openPeriodicTable();
		void negate();
		void checkVersion();
		void reportBug();
		void help();
		void loadInitialHistory();

	signals:

};


class QalculateTranslator : public QTranslator {

	Q_OBJECT

	public:

		QalculateTranslator();

		QString	translate(const char *context, const char *sourceText, const char *disambiguation = NULL, int n = -1) const;

};

class MathSpinBox : public QSpinBox {

	Q_OBJECT

	public:

		MathSpinBox(QWidget *parent = NULL);
		virtual ~MathSpinBox();

		QLineEdit *entry() const;

	protected:

		int valueFromText(const QString &text) const override;
		QValidator::State validate(QString &text, int &pos) const override;
		void keyPressEvent(QKeyEvent *event);

	signals:

		void returnPressed();

};

#endif //QALCULATE_WINDOW_H

