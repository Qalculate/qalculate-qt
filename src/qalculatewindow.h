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
#include <QList>
#include <QSpinBox>
#include <QTranslator>
#include <QDockWidget>
#include <QToolButton>
#include <libqalculate/qalculate.h>

class QLocalSocket;
class QLocalServer;
class QCommandLineParser;
class ExpressionEdit;
class HistoryView;
class QSplitter;
class QLabel;
class KeypadWidget;
class QAction;
class QToolBar;
class QTextEdit;
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
		void updateWindowTitleResult(const std::string &str);
		bool updateWindowTitle(const QString &str = QString(), bool is_result = false, bool type_change = false);
		void executeFromFile(const QString&);
		void initFinished();

	protected:

		QLocalSocket *socket;
		QLocalServer *server;
		QCommandLineParser *parser;

		Thread *viewThread, *commandThread;

		ExpressionEdit *expressionEdit;
		HistoryView *historyView;
		QSplitter *ehSplitter;
		QLabel *statusLabelLeft, *statusLabelRight;
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
		QComboBox *shortcutActionValueEdit; QTreeWidget *shortcutActionList; QLabel *shortcutActionValueLabel;
		QTreeWidget *shortcutList; QPushButton *addShortcutButton, *editShortcutButton, *removeShortcutButton, *shortcutActionOKButton, *shortcutActionAddButton; keyboard_shortcut *edited_keyboard_shortcut;
		QList<FunctionDialog*> functionDialogs;

		KeypadWidget *keypad;
		QDockWidget *keypadDock, *basesDock, *rpnDock;
		QLabel *binEdit, *octEdit, *decEdit, *hexEdit;
		QLabel *binLabel, *octLabel, *decLabel, *hexLabel;
		QToolBar *tb;
		QToolButton *menuAction_t, *modeAction_t, *keypadAction_t, *storeAction_t, *functionsAction_t, *unitsAction_t, *toAction_t;
		QMenu *tmenu, *basesMenu;
		QAction *plotAction_t, *basesAction, *customOutputBaseAction, *customInputBaseAction, *newVariableAction, *newFunctionAction, *variablesAction, *functionsAction, *unitsAction, *datasetsAction, *plotAction, *fpAction, *calendarsAction, *percentageAction, *periodicTableAction, *exratesAction, *quitAction, *helpAction, *keypadAction, *rpnAction, *chainAction, *gKeypadAction, *pKeypadAction, *xKeypadAction, *cKeypadAction, *nKeypadAction, *showNumpadAction, *showCustomKeypadColumnAction, *resetKeypadPositionAction, *radAction, *degAction, *graAction, *normalAction, *sciAction, *engAction, *simpleAction, *copyBasesAction, *tbAction;
		QMenu *recentVariablesMenu, *favouriteVariablesMenu, *variablesMenu, *functionsMenu, *recentFunctionsMenu, *favouriteFunctionsMenu, *unitsMenu, *recentUnitsMenu, *favouriteUnitsMenu, *angleMenu, *toMenu;
		QAction *assumptionTypeActions[5], *assumptionSignActions[6];
		QMenu *recentWSMenu;
		QAction *recentWSSeparator, *openWSAction, *defaultWSAction, *saveWSAction, *saveWSAsAction;
		QAction *firstFunctionsMenuOptionAction, *firstVariablesMenuOptionAction, *firstUnitsMenuOptionAction;
		QList<QAction*> recentWSAction, recentVariableActions, favouriteVariableActions, recentFunctionActions, favouriteFunctionActions, recentUnitActions, favouriteUnitActions;
		QSpinBox *customOutputBaseEdit, *customInputBaseEdit, *minDecimalsEdit, *maxDecimalsEdit;
		QTimer *ecTimer, *rfTimer, *autoCalculateTimer, *decimalsTimer, *resizeTimer, *emhTimer, *testTimer;
		qint64 prev_test_time;
		int prev_test_type;

		QTableWidget *rpnView;
		QAction *rpnUpAction, *rpnDownAction, *rpnSwapAction, *rpnCopyAction, *rpnLastxAction, *rpnDeleteAction, *rpnClearAction;

		bool send_event;
		bool workspace_changed;
		bool init_in_progress;

		void calculateExpression(bool force = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, std::string execute_str = std::string(), std::string str = std::string(), bool check_exrates = true, bool calculate_selection = false);
		void setResult(Prefix *prefix = NULL, bool update_history = true, bool update_parse = false, bool force = false, std::string transformation = "", bool do_stack = false, size_t stack_index = 0, bool register_moved = false, bool supress_dialog = false, bool calculate_selection = false);
		void executeCommand(int command_type, bool show_result = true, std::string ceu_str = "", Unit *u = NULL, int run = 1);
		void changeEvent(QEvent *e) override;
		void resizeEvent(QResizeEvent *e) override;
		bool warnAssumptions(MathStructure&, bool = false);
		bool askTC(MathStructure&, bool = false);
		bool askSinc(MathStructure&, bool = false);
		bool askDot(const std::string&, bool = false);
		bool askImplicit();
		bool askPercent(bool = false);
		void keyPressEvent(QKeyEvent*) override;
		bool eventFilter(QObject*, QEvent*) override;
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
		void beforeShowDockCleanUp(QDockWidget*);
		void beforeShowDock(QDockWidget*, bool);
		void afterShowDock(QDockWidget*);
		void updateInsertFunctionDialogs();
		void updateStatusText();

	protected slots:

		void testTimeout();
		void updateBinEditSize(QFont* = NULL);
		void onBinaryBitsChanged();
		void onTwosChanged();
		void onSymbolClicked(const QString&);
		void onOperatorClicked(const QString&);
		void onFunctionClicked(MathFunction*);
		void onVariableClicked(Variable*);
		void onUnitClicked(Unit*);
		void onPrefixClicked(Prefix*);
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
		void onBaseClicked(int, bool, bool);
		void onFactorizeClicked();
		void onExpandClicked();
		void onExpandPartialFractionsClicked();
		void onToActivated(bool = true);
		void onToActivatedAlt();
		void onStoreActivated();
		void keypadTypeActivated();
		void showNumpad(bool);
		void showCustomKeypadColumn(bool);
		void showSeparateKeypadMenuButtons(bool);
		void resetKeypadPosition();
		void onEMHTimeout();
		void onResizeTimeout();
		void onKeypadVisibilityChanged(bool);
		void onToolbarVisibilityChanged(bool);
		void onBasesActivated(bool);
		void onBasesVisibilityChanged(bool);
		void onRPNVisibilityChanged(bool);
		void onRPNClosed();
		void onExpressionChanged();
		void onHistoryReloaded();
		void onStatusChanged(QString, bool, bool, bool, bool);
		void autoCalculateTimeout();
		void onToConversionRequested(std::string);
		void onInsertTextRequested(std::string);
		void onInsertValueRequested(int);
		void onAlwaysOnTopChanged();
		void onEnableTooltipsChanged();
		void onPreferencesClosed();
		void onTitleTypeChanged();
		void onResultFontChanged();
		void onExpressionFontChanged();
		void onStatusFontChanged();
		void onKeypadFontChanged();
		void onAppFontChanged();
		void onAppFontTimer();
		void fixSplitterPos();
		void angleUnitActivated();
		void normalActivated();
		void scientificActivated();
		void engineeringActivated();
		void simpleActivated();
		void onPrecisionChanged(int);
		void syncDecimals();
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
		void onInsertFunctionInsertRPN();
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
		void toggleResultFraction();
		void onExpressionStatusModeChanged(bool = true);
		void functionActivated();
		void unitActivated();
		void prefixActivated();
		void variableActivated();
		void updateAngleUnitsMenu();
		void initializeFunctionsMenu();
		void initializeVariablesMenu();
		void initializeUnitsMenu();
		void updateFunctionsMenu();
		void useFunctionDialog(bool);
		void showAllFunctions(bool);
		void updateFavouriteFunctions();
		void updateRecentFunctions();
		void addToRecentFunctions(MathFunction*);
		void updateUnitsMenu();
		void addCurrencyFlagsToMenu();
		void showAllUnits(bool);
		void updateFavouriteUnits();
		void updateRecentUnits();
		void addToRecentUnits(Unit*);
		void updateVariablesMenu();
		void showAllVariables(bool);
		void updateFavouriteVariables();
		void updateRecentVariables();
		void addToRecentVariables(Variable*);
		void shortcutActivated();
		void shortcutClicked(int, const QString&);
		void keyboardShortcutAdded(keyboard_shortcut *ks);
		void keyboardShortcutRemoved(keyboard_shortcut *ks);
		void addShortcutClicked();
		void editShortcutClicked();
		void removeShortcutClicked();
		void shortcutActionOKClicked();
		void shortcutActionAddClicked();
		void updateShortcutActionOK();
		void currentShortcutActionChanged();
		void currentShortcutChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void shortcutDoubleClicked(QTreeWidgetItem*, int);
		void saveWorkspace();
		void saveWorkspaceAs();
		void openWorkspace();
		void openDefaultWorkspace();
		void openRecentWorkspace();
		void onColorSchemeChanged();
		void showToolbarContextMenu(const QPoint&);
		void setToolbarStyle();
		void showKeypadContextMenu(const QPoint&);
		void updateKeypadTitle();
		void keypadPreferencesChanged();
		void resultBasesLinkActivated(const QString&);
		void showBasesContextMenu(const QPoint&);
		void copyBases();
		void onExpressionPositionChanged();
		void onShowStatusBarChanged();

	public slots:

		void serverNewConnection();
		void socketReadyRead();
		void calculate();
		void calculate(const QString&);
		void calculateSelection();
		void onActivateRequested(const QStringList&, const QString&);
		void abort();
		void abortCommand();
		void fetchExchangeRates();
		void showAbout();
		void expressionCalculationUpdated(int delay = 0);
		void resultFormatUpdated(int delay = 0);
		void resultDisplayUpdated();
		void expressionObjectsUpdated();
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
		void openSettingsFolder();
		void onDatasetsChanged();
		void onFunctionsChanged();
		void insertProperty(DataObject*, DataProperty*);
		void openDatasets();
		void openFunctions();
		void resetUnitsAndVariablesMenus();
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
		void startTest();

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
		void keyPressEvent(QKeyEvent *event) override;

	signals:

		void returnPressed();

};

class QalculateDockWidget : public QDockWidget {

	Q_OBJECT;

	public:

		QalculateDockWidget(const QString &name, QWidget *parent, ExpressionEdit *editwidget);
		QalculateDockWidget(QWidget *parent, ExpressionEdit *editwidget);
		virtual ~QalculateDockWidget();

	protected:

		ExpressionEdit *expressionEdit;

		void keyPressEvent(QKeyEvent *e) override;
		void closeEvent(QCloseEvent *e) override;

	signals:

		void dockClosed();

};

class QalculateToolButton : public QToolButton {

	Q_OBJECT

	public:

		QalculateToolButton(QWidget *parent = NULL);
		~QalculateToolButton();

	protected:

		QTimer *longPressTimer;
		bool b_longpress;

		void mouseReleaseEvent(QMouseEvent*) override;
		void mousePressEvent(QMouseEvent*) override;

	protected slots:

		void longPressTimeout();

	signals:

		void middleButtonClicked();

};

#endif //QALCULATE_WINDOW_H

