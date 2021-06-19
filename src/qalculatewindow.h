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
class QSpinBox;
class QTimer;
class PreferencesDialog;
class FunctionsDialog;

class QalculateWindow : public QMainWindow {

	Q_OBJECT

	public:

		QalculateWindow();
		virtual ~QalculateWindow();

		void setCommandLineParser(QCommandLineParser*);
		void fetchExchangeRates(int timeout, int n = -1);
		static bool displayMessages(QWidget *parent = NULL);
		bool updateWindowTitle(const QString &str = QString(), bool is_result = false);

	protected:

		QLocalSocket *socket;
		QLocalServer *server;
		QCommandLineParser *parser;

		ExpressionEdit *expressionEdit;
		HistoryView *historyView;
		QSplitter *ehSplitter;
		QLabel *statusLabel, *statusIconLabel;
		PreferencesDialog *preferencesDialog;
		FunctionsDialog *functionsDialog;

		KeypadWidget *keypad;
		QDockWidget *keypadDock, *basesDock;
		QLabel *binEdit, *octEdit, *decEdit, *hexEdit;
		QLabel *binLabel, *octLabel, *decLabel, *hexLabel;
		QToolBar *tb;
		QToolButton *menuAction, *modeAction;
		QAction *toAction, *storeAction, *functionsAction, *keypadAction, *basesAction, *customOutputBaseAction, *customInputBaseAction;
		QAction *assumptionTypeActions[5], *assumptionSignActions[6];
		QSpinBox *customOutputBaseEdit, *customInputBaseEdit;
		QTimer *ecTimer, *rfTimer;
		QFont saved_app_font;

		bool send_event, bases_shown;

		void calculateExpression(bool force = true, bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, std::string execute_str = std::string(), std::string str = std::string(), bool check_exrates = true);
		void setResult(Prefix *prefix = NULL, bool update_history = true, bool update_parse = false, bool force = false, std::string transformation = "", size_t stack_index = 0, bool register_moved = false, bool supress_dialog = false);
		void executeCommand(int command_type, bool show_result = true, std::string ceu_str = "", Unit *u = NULL, int run = 1);
		void changeEvent(QEvent *e) override;
		bool checkExchangeRates();
		bool askTC(MathStructure&);
		bool askDot(const std::string&);
		void keyPressEvent(QKeyEvent*) override;
		void closeEvent(QCloseEvent*) override;
		void setPreviousExpression();
		void setOption(std::string);
		void updateResultBases();

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
		void simpleActivated();
		void onPrecisionChanged(int);
		void onMinDecimalsChanged(int);
		void onMaxDecimalsChanged(int);
		void outputBaseActivated();
		void onCustomOutputBaseChanged(int);
		void inputBaseActivated();
		void onCustomInputBaseChanged(int);
		void editPreferences();
		void openFunctions();
		void assumptionsTypeActivated();
		void assumptionsSignActivated();
		void approximationActivated();
		void applyFunction(MathFunction*);

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

	signals:

};

#endif //QALCULATE_WINDOW_H

