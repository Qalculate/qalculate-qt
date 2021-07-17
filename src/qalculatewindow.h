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
class VariablesDialog;
class UnitsDialog;
class FPConversionDialog;
class PlotDialog;
class CalendarConversionDialog;
class QTableWidget;
struct FunctionDialog;

class QalculateWindow : public QMainWindow {

	Q_OBJECT

	public:

		QalculateWindow();
		virtual ~QalculateWindow();

		void setCommandLineParser(QCommandLineParser*);
		bool displayMessages();
		bool updateWindowTitle(const QString &str = QString(), bool is_result = false);
		void executeFromFile(const QString&);
		void loadInitialHistory();

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
		FunctionsDialog *functionsDialog;
		VariablesDialog *variablesDialog;
		UnitsDialog *unitsDialog;
		FPConversionDialog *fpConversionDialog;
		PlotDialog *plotDialog;
		CalendarConversionDialog *calendarConversionDialog;

		KeypadWidget *keypad;
		QDockWidget *keypadDock, *basesDock, *rpnDock;
		QLabel *binEdit, *octEdit, *decEdit, *hexEdit;
		QLabel *binLabel, *octLabel, *decLabel, *hexLabel;
		QToolBar *tb;
		QToolButton *menuAction, *modeAction;
		QAction *toAction, *storeAction, *functionsAction, *keypadAction, *basesAction, *customOutputBaseAction, *customInputBaseAction;
		QAction *assumptionTypeActions[5], *assumptionSignActions[6];
		QSpinBox *customOutputBaseEdit, *customInputBaseEdit;
		QTimer *ecTimer, *rfTimer;
		QFont saved_app_font;

		QTableWidget *rpnView;
		QAction *rpnUpAction, *rpnDownAction, *rpnSwapAction, *rpnCopyAction, *rpnLastxAction, *rpnDeleteAction, *rpnEditAction, *rpnClearAction;

		bool send_event;

		void calculateExpression(bool force = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, std::string execute_str = std::string(), std::string str = std::string(), bool check_exrates = true);
		void setResult(Prefix *prefix = NULL, bool update_history = true, bool update_parse = false, bool force = false, std::string transformation = "", bool do_stack = false, size_t stack_index = 0, bool register_moved = false, bool supress_dialog = false);
		void executeCommand(int command_type, bool show_result = true, std::string ceu_str = "", Unit *u = NULL, int run = 1);
		void changeEvent(QEvent *e) override;
		bool askTC(MathStructure&);
		bool askDot(const std::string&);
		void keyPressEvent(QKeyEvent*) override;
		void closeEvent(QCloseEvent*) override;
		void setPreviousExpression();
		void setOption(std::string);
		void updateResultBases();
		void calculateRPN(MathFunction*);
		void RPNRegisterAdded(std::string, int = 0);
		void RPNRegisterRemoved(int);
		void RPNRegisterChanged(std::string, int);

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
		void onToActivated();
		void onStoreActivated();
		void onKeypadActivated(bool);
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
		void newMatrix();
		void newUnknown();
		void newFunction();
		void convertToUnit(Unit*);
		void importCSV();
		void exportCSV();
		void editPreferences();
		void openFunctions();
		void openUnits();
		void openVariables();
		void applyFunction(MathFunction*);
		void openFPConversion();
		void openPlot();
		void negate();
		void checkVersion();
		void reportBug();
		void help();

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

