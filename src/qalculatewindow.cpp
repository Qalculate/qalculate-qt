/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLocalSocket>
#include <QLocalServer>
#include <QCommandLineParser>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QProgressDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDockWidget>
#include <QToolTip>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QDebug>

#include "qalculatewindow.h"
#include "qalculateqtsettings.h"
#include "expressionedit.h"
#include "historyview.h"
#include "keypadwidget.h"

extern QalculateQtSettings *settings;

class ViewThread : public Thread {
protected:
	virtual void run();
};
class CommandThread : public Thread {
protected:
	virtual void run();
};
class FetchExchangeRatesThread : public Thread {
protected:
	virtual void run();
};

enum {
	COMMAND_FACTORIZE,
	COMMAND_EXPAND_PARTIAL_FRACTIONS,
	COMMAND_EXPAND,
	COMMAND_TRANSFORM,
	COMMAND_CONVERT_UNIT,
	COMMAND_CONVERT_STRING,
	COMMAND_CONVERT_BASE,
	COMMAND_CONVERT_OPTIMAL,
	COMMAND_CALCULATE,
	COMMAND_EVAL
};

std::vector<std::string> alt_results;
int b_busy = 0, block_result_update = 0, nr_of_new_expressions = 0;
bool exact_comparison, command_aborted;
std::string original_expression, result_text, parsed_text, exact_text, previous_expression;
ViewThread *view_thread;
CommandThread *command_thread;
MathStructure *mstruct, *parsed_mstruct, *parsed_tostruct, mstruct_exact, lastx;
std::string command_convert_units_string;
Unit *command_convert_unit;
bool to_fraction = false;
char to_prefix = 0;
int to_base = 0;
int to_caf = -1;
unsigned int to_bits = 0;
Number to_nbase;
std::string result_bin, result_oct, result_dec, result_hex;
Number max_bases, min_bases;
bool title_modified = false;

extern void fix_to_struct(MathStructure &m);
extern void print_dual(const MathStructure &mresult, const std::string &original_expression, const MathStructure &mparse, MathStructure &mexact, std::string &result_str, std::vector<std::string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle = false, bool *exact_cmp = NULL, bool b_parsed = true, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1);
extern void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const std::string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs = 0, int max_size = 10);
extern int has_information_unit(const MathStructure &m, bool top = true);

std::string unhtmlize(std::string str, bool replace_all_i = false) {
	size_t i = 0, i2;
	while(true) {
		i = str.find("<", i);
		if(i == std::string::npos) break;
		i2 = str.find(">", i + 1);
		if(i2 == std::string::npos) break;
		if(i2 - i == 4) {
			if(str.substr(i + 1, 3) == "sup") {
				size_t i3 = str.find("</sup>", i2 + 1);
				if(i3 != std::string::npos) {
					std::string str2 = unhtmlize(str.substr(i + 5, i3 - i - 5));
					if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else str.replace(i, i3 - i + 6, std::string("^(") + str2 + ")");
					continue;
				}
			} else if(str.substr(i + 1, 3) == "sub") {
				size_t i3 = str.find("</sub>", i + 4);
				if(i3 != std::string::npos) {
					str.replace(i, i3 - i + 6, std::string("_") + unhtmlize(str.substr(i + 5, i3 - i - 5)));
					continue;
				}
			}
		} else if(i2 - i == 2 && str[i + 1] == 'i') {
			size_t i3 = str.find("</i>", i2 + 1);
			if(i3 != std::string::npos) {
				std::string name = unhtmlize(str.substr(i + 3, i3 - i - 3));
				if(!replace_all_i) {
					Variable *v = CALCULATOR->getActiveVariable(name);
					replace_all_i = (!v || v->isKnown());
				}
				if(replace_all_i && name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, 1, '\\');
				} else if(replace_all_i) {
					name.insert(0, 1, '\"');
					name += '\"';
				}
				str.replace(i, i3 - i + 4, name);
				continue;
			}
		}
		str.erase(i, i2 - i + 1);
	}
	gsub(" " SIGN_DIVISION_SLASH " ", "/", str);
	gsub("&amp;", "&", str);
	gsub("&gt;", ">", str);
	gsub("&lt;", "<", str);
	return str;
}

QalculateWindow::QalculateWindow() : QMainWindow() {

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	send_event = true;

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	QHBoxLayout *hLayout = new QHBoxLayout();
	topLayout->addLayout(hLayout);
	ehSplitter = new QSplitter(Qt::Vertical, this);
	hLayout->addWidget(ehSplitter, 1);

	tb = new QToolBar(this);
	tb->setOrientation(Qt::Vertical);
	menuAction = new QAction(LOAD_ICON("open-menu-symbolic"), tr("Menu"));
	tb->addAction(menuAction);
	toAction = new QAction(LOAD_ICON("go-next"), tr("Convert"));
	connect(toAction, SIGNAL(triggered(bool)), this, SLOT(onToActivated()));
	tb->addAction(toAction);
	storeAction = new QAction(LOAD_ICON("document-save"), tr("Store"));
	connect(storeAction, SIGNAL(triggered(bool)), this, SLOT(onStoreActivated()));
	tb->addAction(storeAction);
	keypadAction = new QAction(LOAD_ICON("input-dialpad"), tr("Keypad"));
	connect(keypadAction, SIGNAL(triggered(bool)), this, SLOT(onKeypadActivated(bool)));
	keypadAction->setCheckable(true);
	tb->addAction(keypadAction);
	basesAction = new QAction(LOAD_ICON("window-new"), tr("Number bases"));
	connect(basesAction, SIGNAL(triggered(bool)), this, SLOT(onBasesActivated(bool)));
	basesAction->setCheckable(true);
	tb->addAction(basesAction);
	hLayout->addWidget(tb, 0);
//∡
	expressionEdit = new ExpressionEdit(this);
	expressionEdit->setFocus();
	ehSplitter->addWidget(expressionEdit);
	historyView = new HistoryView(this);

	ehSplitter->addWidget(historyView);
	ehSplitter->setStretchFactor(0, 0);
	ehSplitter->setStretchFactor(1, 1);

	basesDock = new QDockWidget(tr("Number bases"), this);
	QWidget *basesWidget = new QWidget(this);
	basesDock->setWidget(basesWidget);
	addDockWidget(Qt::BottomDockWidgetArea, basesDock);
	basesDock->hide();

	keypad = new KeypadWidget(this);
	keypadDock = new QDockWidget(tr("Keypad"), this);
	keypadDock->setWidget(keypad);
	addDockWidget(Qt::BottomDockWidgetArea, keypadDock);
	keypadDock->hide();

	QLocalServer::removeServer("qalculate-qt");
	server = new QLocalServer(this);
	server->listen("qalculate-qt");
	connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));
	
	mstruct = new MathStructure();
	settings->current_result = mstruct;
	mstruct_exact.setUndefined();
	parsed_mstruct = new MathStructure();
	parsed_tostruct = new MathStructure();
	view_thread = new ViewThread;
	command_thread = new CommandThread;

	settings->printops.can_display_unicode_string_arg = (void*) historyView;

	b_busy = 0;

	connect(expressionEdit, SIGNAL(returnPressed()), this, SLOT(calculate()));
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onExpressionChanged()));
	connect(expressionEdit, SIGNAL(toConversionRequested(std::string)), this, SLOT(onToConversionRequested(std::string)));
	connect(basesDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onBasesVisibilityChanged(bool)));
	connect(keypadDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onKeypadVisibilityChanged(bool)));
	connect(keypad, SIGNAL(symbolClicked(const QString&)), this, SLOT(onSymbolClicked(const QString&)));
	connect(keypad, SIGNAL(operatorClicked(const QString&)), this, SLOT(onOperatorClicked(const QString&)));
	connect(keypad, SIGNAL(functionClicked(MathFunction*)), this, SLOT(onFunctionClicked(MathFunction*)));
	connect(keypad, SIGNAL(variableClicked(Variable*)), this, SLOT(onVariableClicked(Variable*)));
	connect(keypad, SIGNAL(unitClicked(Unit*)), this, SLOT(onUnitClicked(Unit*)));
	connect(keypad, SIGNAL(delClicked()), this, SLOT(onDelClicked()));
	connect(keypad, SIGNAL(clearClicked()), this, SLOT(onClearClicked()));
	connect(keypad, SIGNAL(equalsClicked()), this, SLOT(onEqualsClicked()));
	connect(keypad, SIGNAL(parenthesesClicked()), this, SLOT(onParenthesesClicked()));
	connect(keypad, SIGNAL(bracketsClicked()), this, SLOT(onBracketsClicked()));
	connect(keypad, SIGNAL(endClicked()), this, SLOT(onEndClicked()));
	connect(keypad, SIGNAL(startClicked()), this, SLOT(onStartClicked()));
	connect(keypad, SIGNAL(leftClicked()), this, SLOT(onLeftClicked()));
	connect(keypad, SIGNAL(rightClicked()), this, SLOT(onRightClicked()));
	connect(keypad, SIGNAL(backspaceClicked()), this, SLOT(onBackspaceClicked()));
	connect(keypad, SIGNAL(MSClicked()), this, SLOT(onMSClicked()));
	connect(keypad, SIGNAL(MRClicked()), this, SLOT(onMRClicked()));
	connect(keypad, SIGNAL(MCClicked()), this, SLOT(onMCClicked()));
	connect(keypad, SIGNAL(MPlusClicked()), this, SLOT(onMPlusClicked()));
	connect(keypad, SIGNAL(MMinusClicked()), this, SLOT(onMMinusClicked()));

}
QalculateWindow::~QalculateWindow() {}

void QalculateWindow::onSymbolClicked(const QString &str) {
	expressionEdit->blockCompletion(true);
	expressionEdit->insertPlainText(str);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onOperatorClicked(const QString &str) {
	bool do_exec = false;
	if(str == "~") {
		expressionEdit->wrapSelection(str, true, false);
	} else if(str == "!") {
		QTextCursor cur = expressionEdit->textCursor();
		do_exec = (str == "!") && cur.hasSelection() && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
		expressionEdit->wrapSelection(str);
	} else if(str == "E") {
		if(expressionEdit->textCursor().hasSelection()) expressionEdit->wrapSelection(SIGN_MULTIPLICATION "10^");
		else expressionEdit->insertPlainText(settings->printops.lower_case_e ? "e" : str);
	} else {
		expressionEdit->wrapSelection(str);
	}
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	if(do_exec) calculate();
}

bool last_is_number(std::string str) {
	str = CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options);
	CALCULATOR->parseSigns(str);
	if(str.empty()) return false;
	return is_not_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1]);
}
void QalculateWindow::onFunctionClicked(MathFunction *f) {
	expressionEdit->blockCompletion(true);
	QTextCursor cur = expressionEdit->textCursor();
	bool do_exec = false;
	if(cur.hasSelection()) {
		do_exec = cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
	} else if(last_is_number(expressionEdit->toPlainText().toStdString())) {
		expressionEdit->selectAll();
		do_exec = true;
	}
	expressionEdit->wrapSelection(f->referenceName() == "neg" ? SIGN_MINUS : QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name), true, true);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
	if(do_exec) calculate();
}
void QalculateWindow::onVariableClicked(Variable *v) {
	expressionEdit->blockCompletion(true);
	expressionEdit->insertPlainText(QString::fromStdString(v->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onUnitClicked(Unit *u) {
	expressionEdit->blockCompletion(true);
	expressionEdit->insertPlainText(QString::fromStdString(u->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onDelClicked() {
	expressionEdit->blockCompletion(true);
	QTextCursor cur = expressionEdit->textCursor();
	if(cur.atEnd()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onBackspaceClicked() {
	expressionEdit->blockCompletion(true);
	QTextCursor cur = expressionEdit->textCursor();
	if(!cur.atStart()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onClearClicked() {
	expressionEdit->clear();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onEqualsClicked() {
	calculate();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onParenthesesClicked() {
	expressionEdit->smartParentheses();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onBracketsClicked() {
	expressionEdit->insertBrackets();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onLeftClicked() {
	expressionEdit->moveCursor(QTextCursor::PreviousCharacter);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onRightClicked() {
	expressionEdit->moveCursor(QTextCursor::NextCharacter);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onStartClicked() {
	expressionEdit->moveCursor(QTextCursor::Start);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onEndClicked() {
	expressionEdit->moveCursor(QTextCursor::End);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMSClicked() {
	if(expressionEdit->expressionHasChanged()) calculate();
	if(!mstruct) return;
	settings->v_memory->set(*mstruct);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMRClicked() {
	onVariableClicked(settings->v_memory);
}
void QalculateWindow::onMCClicked() {
	settings->v_memory->set(m_zero);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMPlusClicked() {
	if(expressionEdit->expressionHasChanged()) calculate();
	if(!mstruct) return;
	MathStructure m = settings->v_memory->get();
	m.calculateAdd(*mstruct, settings->evalops);
	settings->v_memory->set(m);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMMinusClicked() {
	if(expressionEdit->expressionHasChanged()) calculate();
	if(!mstruct) return;
	MathStructure m = settings->v_memory->get();
	m.calculateAdd(*mstruct, settings->evalops);
	settings->v_memory->set(m);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::serverNewConnection() {
	socket = server->nextPendingConnection();
	if(socket) {
		connect(socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
	}
}
void QalculateWindow::socketReadyRead() {
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	raise();
	activateWindow();
	QString command = socket->readAll();
	command = command.mid(1).trimmed();
	if(!command.isEmpty()) {expressionEdit->setPlainText(command); calculate();}
}
void QalculateWindow::onActivateRequested(const QStringList &arguments, const QString&) {
	if(!arguments.isEmpty()) {
		parser->process(arguments);
		QStringList args = parser->positionalArguments();
		QString command;
		for(int i = 0; i < args.count(); i++) {
			if(i > 0) command += " ";
			command += args.at(i);
		}
		command = command.trimmed();
		if(!command.isEmpty()) {expressionEdit->setPlainText(command); calculate();}
		args.clear();
	}
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	activateWindow();
}
void QalculateWindow::setCommandLineParser(QCommandLineParser *p) {
	parser = p;
}
void QalculateWindow::calculate() {
	calculateExpression();
}
void QalculateWindow::calculate(const QString &expression) {
	expressionEdit->setPlainText(expression);
	calculate();
}

void QalculateWindow::setPreviousExpression() {
	if(settings->rpn_mode) {
		expressionEdit->clear();
	} else {
		expressionEdit->blockCompletion(true);
		expressionEdit->blockParseStatus(true);
		expressionEdit->setPlainText(QString::fromStdString(previous_expression));
		expressionEdit->selectAll();
		expressionEdit->blockCompletion(false);
		expressionEdit->blockParseStatus(false);
	}
}

void QalculateWindow::resultFormatUpdated() {
	if(block_result_update) return;
	settings->updateMessagePrintOptions();
	setResult(NULL, true, false, false);
	if(!QToolTip::text().isEmpty()) expressionEdit->displayParseStatus(true);
}
void QalculateWindow::resultDisplayUpdated() {
	if(block_result_update) return;
	settings->updateMessagePrintOptions();
	if(!QToolTip::text().isEmpty()) expressionEdit->displayParseStatus(true);
}
void QalculateWindow::expressionFormatUpdated(bool) {
	if(!QToolTip::text().isEmpty()) expressionEdit->displayParseStatus(true);
	settings->updateMessagePrintOptions();
	/*if(!expression_has_changed && !recalculate && !settings->rpn_mode) {
		expressionEdit->clear();
	} else if(!settings->rpn_mode && parsed_mstruct) {
		for(size_t i = 0; i < 5; i++) {
			if(parsed_mstruct->contains(vans[i])) expressionEdit->clear();
		}
	}
	if(!settings->rpn_mode && recalculate) {
		executeExpression(false);
	}*/
}
void QalculateWindow::expressionCalculationUpdated() {
	if(!QToolTip::text().isEmpty()) expressionEdit->displayParseStatus(true);
	settings->updateMessagePrintOptions();
	/*if(!settings->rpn_mode) {
		if(parsed_mstruct) {
			for(size_t i = 0; i < 5; i++) {
				if(parsed_mstruct->contains(vans[i])) return;
			}
		}
		execute_expression(false);
	}*/
}

int s2b(const std::string &str) {
	if(str.empty()) return -1;
	if(equalsIgnoreCase(str, "yes")) return 1;
	if(equalsIgnoreCase(str, "no")) return 0;
	if(equalsIgnoreCase(str, "true")) return 1;
	if(equalsIgnoreCase(str, "false")) return 0;
	if(equalsIgnoreCase(str, "on")) return 1;
	if(equalsIgnoreCase(str, "off")) return 0;
	if(str.find_first_not_of(SPACES NUMBERS) != std::string::npos) return -1;
	int i = s2i(str);
	if(i > 0) return 1;
	return 0;
}
void base_from_string(std::string str, int &base, Number &nbase, bool input_base = false) {
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") base = BASE_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "roman") || equalsIgnoreCase(str, "roman")) base = BASE_ROMAN_NUMERALS;
	else if(!input_base && (equalsIgnoreCase(str, "time") || equalsIgnoreCase(str, "time"))) base = BASE_TIME;
	else if(str == "b26" || str == "B26") base = BASE_BIJECTIVE_26;
	else if(equalsIgnoreCase(str, "unicode")) base = BASE_UNICODE;
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") base = BASE_SUPER_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "pi") || str == "π") base = BASE_PI;
	else if(str == "e") base = BASE_E;
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") base = BASE_SQRT2;
	else {
		EvaluationOptions eo = settings->evalops;
		eo.parse_options.base = 10;
		MathStructure m;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 350, eo);
		if(CALCULATOR->endTemporaryStopMessages()) {
			base = BASE_CUSTOM;
			nbase.clear();
		} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
			base = m.number().intValue();
		} else {
			base = BASE_CUSTOM;
			nbase = m.number();
		}
	}
}
void set_assumption(const std::string &str, AssumptionType &at, AssumptionSign &as, bool last_of_two = false) {
	if(equalsIgnoreCase(str, "unknown") || str == "0") {
		if(!last_of_two) as = ASSUMPTION_SIGN_UNKNOWN;
		else at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "real")) {
		at = ASSUMPTION_TYPE_REAL;
	} else if(equalsIgnoreCase(str, "number") || str == "num") {
		at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "rational") || str == "rat") {
		at = ASSUMPTION_TYPE_RATIONAL;
	} else if(equalsIgnoreCase(str, "integer") || str == "int") {
		at = ASSUMPTION_TYPE_INTEGER;
	} else if(equalsIgnoreCase(str, "boolean") || str == "bool") {
		at = ASSUMPTION_TYPE_BOOLEAN;
	} else if(equalsIgnoreCase(str, "non-zero") || str == "nz") {
		as = ASSUMPTION_SIGN_NONZERO;
	} else if(equalsIgnoreCase(str, "positive") || str == "pos") {
		as = ASSUMPTION_SIGN_POSITIVE;
	} else if(equalsIgnoreCase(str, "non-negative") || str == "nneg") {
		as = ASSUMPTION_SIGN_NONNEGATIVE;
	} else if(equalsIgnoreCase(str, "negative") || str == "neg") {
		as = ASSUMPTION_SIGN_NEGATIVE;
	} else if(equalsIgnoreCase(str, "non-positive") || str == "npos") {
		as = ASSUMPTION_SIGN_NONPOSITIVE;
	} else {
		CALCULATOR->error(true, "Unrecognized assumption: %s.", str.c_str(), NULL);
	}
}

//#define SET_BOOL_MENU(x)	{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, x)), v);}
#define SET_BOOL_MENU(x)	{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);}}
#define SET_BOOL_D(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; resultDisplayUpdated();}}
//#define SET_BOOL_PREF(x)	{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else {if(!preferences_builder) {get_preferences_dialog();} gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, x)), v);}}
#define SET_BOOL_PREF(x)	{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);}}
#define SET_BOOL_E(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionCalculationUpdated();}}
#define SET_BOOL(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v;}}

void QalculateWindow::setOption(std::string str) {
	remove_blank_ends(str);
	gsub(SIGN_MINUS, "-", str);
	std::string svalue, svar;
	bool empty_value = false;
	size_t i_underscore = str.find("_");
	size_t index;
	if(i_underscore != std::string::npos) {
		index = str.find_first_of(SPACES);
		if(index != std::string::npos && i_underscore > index) i_underscore = std::string::npos;
	}
	if(i_underscore == std::string::npos) index = str.find_last_of(SPACES);
	if(index != std::string::npos) {
		svar = str.substr(0, index);
		remove_blank_ends(svar);
		svalue = str.substr(index + 1);
		remove_blank_ends(svalue);
	} else {
		svar = str;
	}
	if(i_underscore != std::string::npos) gsub("_", " ", svar);
	if(svalue.empty()) {
		empty_value = true;
		svalue = "1";
	}

	set_option_place:
	if(equalsIgnoreCase(svar, "base") || equalsIgnoreCase(svar, "input base") || svar == "inbase" || equalsIgnoreCase(svar, "output base") || svar == "outbase") {
		int v = 0;
		bool b_in = equalsIgnoreCase(svar, "input base") || svar == "inbase";
		bool b_out = equalsIgnoreCase(svar, "output base") || svar == "outbase";
		if(equalsIgnoreCase(svalue, "roman")) v = BASE_ROMAN_NUMERALS;
		else if(equalsIgnoreCase(svalue, "bijective") || str == "b26" || str == "B26") v = BASE_BIJECTIVE_26;
		else if(equalsIgnoreCase(svalue, "fp32") || equalsIgnoreCase(svalue, "binary32") || equalsIgnoreCase(svalue, "float")) {if(b_in) v = 0; else v = BASE_FP32;}
		else if(equalsIgnoreCase(svalue, "fp64") || equalsIgnoreCase(svalue, "binary64") || equalsIgnoreCase(svalue, "double")) {if(b_in) v = 0; else v = BASE_FP64;}
		else if(equalsIgnoreCase(svalue, "fp16") || equalsIgnoreCase(svalue, "binary16")) {if(b_in) v = 0; else v = BASE_FP16;}
		else if(equalsIgnoreCase(svalue, "fp80")) {if(b_in) v = 0; else v = BASE_FP80;}
		else if(equalsIgnoreCase(svalue, "fp128") || equalsIgnoreCase(svalue, "binary128")) {if(b_in) v = 0; else v = BASE_FP128;}
		else if(equalsIgnoreCase(svalue, "time")) {if(b_in) v = 0; else v = BASE_TIME;}
		else if(equalsIgnoreCase(svalue, "hex") || equalsIgnoreCase(svalue, "hexadecimal")) v = BASE_HEXADECIMAL;
		else if(equalsIgnoreCase(svalue, "golden") || equalsIgnoreCase(svalue, "golden ratio") || svalue == "φ") v = BASE_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "supergolden") || equalsIgnoreCase(svalue, "supergolden ratio") || svalue == "ψ") v = BASE_SUPER_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "pi") || svalue == "π") v = BASE_PI;
		else if(svalue == "e") v = BASE_E;
		else if(svalue == "sqrt(2)" || svalue == "sqrt 2" || svalue == "sqrt2" || svalue == "√2") v = BASE_SQRT2;
		else if(equalsIgnoreCase(svalue, "unicode")) v = BASE_UNICODE;
		else if(equalsIgnoreCase(svalue, "duo") || equalsIgnoreCase(svalue, "duodecimal")) v = 12;
		else if(equalsIgnoreCase(svalue, "bin") || equalsIgnoreCase(svalue, "binary")) v = BASE_BINARY;
		else if(equalsIgnoreCase(svalue, "oct") || equalsIgnoreCase(svalue, "octal")) v = BASE_OCTAL;
		else if(equalsIgnoreCase(svalue, "dec") || equalsIgnoreCase(svalue, "decimal")) v = BASE_DECIMAL;
		else if(equalsIgnoreCase(svalue, "sexa") || equalsIgnoreCase(svalue, "sexagesimal")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL;}
		else if(equalsIgnoreCase(svalue, "sexa2") || equalsIgnoreCase(svalue, "sexagesimal2")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_2;}
		else if(equalsIgnoreCase(svalue, "sexa3") || equalsIgnoreCase(svalue, "sexagesimal3")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_3;}
		else if(equalsIgnoreCase(svalue, "latitude")) {if(b_in) v = 0; else v = BASE_LATITUDE;}
		else if(equalsIgnoreCase(svalue, "latitude2")) {if(b_in) v = 0; else v = BASE_LATITUDE_2;}
		else if(equalsIgnoreCase(svalue, "longitude")) {if(b_in) v = 0; else v = BASE_LONGITUDE;}
		else if(equalsIgnoreCase(svalue, "longitude2")) {if(b_in) v = 0; else v = BASE_LONGITUDE_2;}
		else if(!b_in && !b_out && (index = svalue.find_first_of(SPACES)) != std::string::npos) {
			str = svalue;
			svalue = str.substr(index + 1, str.length() - (index + 1));
			remove_blank_ends(svalue);
			svar += " ";
			str = str.substr(0, index);
			remove_blank_ends(str);
			svar += str;
			gsub("_", " ", svar);
			if(equalsIgnoreCase(svar, "base display")) {
				goto set_option_place;
			}
			setOption(std::string("inbase ") + svalue);
			setOption(std::string("outbase ") + str);
			return;
		} else if(!empty_value) {
			MathStructure m;
			EvaluationOptions eo = settings->evalops;
			eo.parse_options.base = 10;
			eo.approximation = APPROXIMATION_TRY_EXACT;
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(svalue, eo.parse_options), 500, eo);
			if(CALCULATOR->endTemporaryStopMessages()) {
				v = 0;
			} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
				v = m.number().intValue();
			} else if(m.isNumber() && (b_in || ((!m.number().isNegative() || m.number().isInteger()) && (m.number() > 1 || m.number() < -1)))) {
				v = BASE_CUSTOM;
				if(b_in) CALCULATOR->setCustomInputBase(m.number());
				else CALCULATOR->setCustomOutputBase(m.number());
			}
		}
		if(v == 0) {
			CALCULATOR->error(true, "Illegal base: %s.", svalue.c_str(), NULL);
		} else if(b_in) {
			if(v == BASE_CUSTOM || v != settings->evalops.parse_options.base) {
				settings->evalops.parse_options.base = v;
				/*input_base_updated_from_menu();
				update_keypad_bases();*/
				expressionFormatUpdated(false);
			}
		} else {
			if(v == BASE_CUSTOM || v != settings->printops.base) {
				settings->printops.base = v;
				to_base = 0;
				to_bits = 0;
				/*update_menu_base();
				output_base_updated_from_menu();
				update_keypad_bases();*/
				resultFormatUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "assumptions") || svar == "ass") {
		size_t i = svalue.find_first_of(SPACES);
		AssumptionType at = CALCULATOR->defaultAssumptions()->type();
		AssumptionSign as = CALCULATOR->defaultAssumptions()->sign();
		if(i != std::string::npos) {
			set_assumption(svalue.substr(0, i), at, as, false);
			set_assumption(svalue.substr(i + 1, svalue.length() - (i + 1)), at, as, true);
		} else {
			set_assumption(svalue, at, as, false);
		}
		//set_assumptions_items(at, as);
	} else if(equalsIgnoreCase(svar, "all prefixes") || svar == "allpref") SET_BOOL_MENU("menu_item_all_prefixes")
	else if(equalsIgnoreCase(svar, "complex numbers") || svar == "cplx") SET_BOOL_MENU("menu_item_allow_complex")
	else if(equalsIgnoreCase(svar, "excessive parentheses") || svar == "expar") SET_BOOL_D(settings->printops.excessive_parenthesis)
	else if(equalsIgnoreCase(svar, "functions") || svar == "func") SET_BOOL_MENU("menu_item_enable_functions")
	else if(equalsIgnoreCase(svar, "infinite numbers") || svar == "inf") SET_BOOL_MENU("menu_item_allow_infinite")
	else if(equalsIgnoreCase(svar, "show negative exponents") || svar == "negexp") SET_BOOL_MENU("menu_item_negative_exponents")
	else if(equalsIgnoreCase(svar, "minus last") || svar == "minlast") SET_BOOL_MENU("menu_item_sort_minus_last")
	else if(equalsIgnoreCase(svar, "assume nonzero denominators") || svar == "nzd") SET_BOOL_MENU("menu_item_assume_nonzero_denominators")
	else if(equalsIgnoreCase(svar, "warn nonzero denominators") || svar == "warnnzd") SET_BOOL_MENU("menu_item_warn_about_denominators_assumed_nonzero")
	else if(equalsIgnoreCase(svar, "prefixes") || svar == "pref") SET_BOOL_MENU("menu_item_prefixes_for_selected_units")
	else if(equalsIgnoreCase(svar, "binary prefixes") || svar == "binpref") SET_BOOL_PREF("preferences_checkbutton_binary_prefixes")
	else if(equalsIgnoreCase(svar, "denominator prefixes") || svar == "denpref") SET_BOOL_MENU("menu_item_denominator_prefixes")
	else if(equalsIgnoreCase(svar, "place units separately") || svar == "unitsep") SET_BOOL_MENU("menu_item_place_units_separately")
	else if(equalsIgnoreCase(svar, "calculate variables") || svar == "calcvar") SET_BOOL_MENU("menu_item_calculate_variables")
	else if(equalsIgnoreCase(svar, "calculate functions") || svar == "calcfunc") SET_BOOL_E(settings->evalops.calculate_functions)
	else if(equalsIgnoreCase(svar, "sync units") || svar == "sync") SET_BOOL_E(settings->evalops.sync_units)
	else if(equalsIgnoreCase(svar, "temperature calculation") || svar == "temp")  {
		int v = -1;
		if(equalsIgnoreCase(svalue, "relative")) v = TEMPERATURE_CALCULATION_RELATIVE;
		else if(equalsIgnoreCase(svalue, "hybrid")) v = TEMPERATURE_CALCULATION_HYBRID;
		else if(equalsIgnoreCase(svalue, "absolute")) v = TEMPERATURE_CALCULATION_ABSOLUTE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(!preferences_builder) get_preferences_dialog();
			switch(v) {
				case TEMPERATURE_CALCULATION_RELATIVE: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel")), TRUE);
					break;
				}
				case TEMPERATURE_CALCULATION_ABSOLUTE: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs")), TRUE);
					break;
				}
				case TEMPERATURE_CALCULATION_HYBRID: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid")), TRUE);
					break;
				}
			}*/
		}
	} else if(equalsIgnoreCase(svar, "round to even") || svar == "rndeven") SET_BOOL_MENU("menu_item_round_halfway_to_even")
	else if(equalsIgnoreCase(svar, "rpn syntax") || svar == "rpnsyn") {
		bool b = (settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN);
		SET_BOOL(b)
		if(b != (settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN)) {
			/*if(b) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
			else gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);*/
		}
	} else if(equalsIgnoreCase(svar, "rpn") && svalue.find(" ") == std::string::npos) SET_BOOL_MENU("menu_item_rpn_mode")
	else if(equalsIgnoreCase(svar, "short multiplication") || svar == "shortmul") SET_BOOL_D(settings->printops.short_multiplication)
	else if(equalsIgnoreCase(svar, "lowercase e") || svar == "lowe") SET_BOOL_PREF("preferences_checkbutton_lower_case_e")
	else if(equalsIgnoreCase(svar, "lowercase numbers") || svar == "lownum") SET_BOOL_PREF("preferences_checkbutton_lower_case_numbers")
	else if(equalsIgnoreCase(svar, "imaginary j") || svar == "imgj") SET_BOOL_PREF("preferences_checkbutton_imaginary_j")
	else if(equalsIgnoreCase(svar, "base display") || svar == "basedisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = BASE_DISPLAY_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "normal")) v = BASE_DISPLAY_NORMAL;
		else if(equalsIgnoreCase(svalue, "alternative")) v = BASE_DISPLAY_ALTERNATIVE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(!preferences_builder) get_preferences_dialog();
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_alternative_base_prefixes")), v == BASE_DISPLAY_ALTERNATIVE);*/
		}
	} else if(equalsIgnoreCase(svar, "two's complement") || svar == "twos") SET_BOOL_PREF("preferences_checkbutton_twos_complement")
	else if(equalsIgnoreCase(svar, "hexadecimal two's") || svar == "hextwos") SET_BOOL_PREF("preferences_checkbutton_hexadecimal_twos_complement")
	else if(equalsIgnoreCase(svar, "digit grouping") || svar =="group") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DIGIT_GROUPING_NONE;
		else if(equalsIgnoreCase(svalue, "none")) v = DIGIT_GROUPING_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "standard") || equalsIgnoreCase(svalue, "on")) v = DIGIT_GROUPING_STANDARD;
		else if(equalsIgnoreCase(svalue, "locale")) v = DIGIT_GROUPING_LOCALE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < DIGIT_GROUPING_NONE || v > DIGIT_GROUPING_LOCALE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(!preferences_builder) get_preferences_dialog();
			if(v == DIGIT_GROUPING_NONE) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_digit_grouping_none")), TRUE);
			else if(v == DIGIT_GROUPING_STANDARD) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_digit_grouping_standard")), TRUE);
			else if(v == DIGIT_GROUPING_LOCALE) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_digit_grouping_locale")), TRUE);*/
		}
	} else if(equalsIgnoreCase(svar, "spell out logical") || svar == "spellout") SET_BOOL_PREF("preferences_checkbutton_spell_out_logical_operators")
	else if((equalsIgnoreCase(svar, "ignore dot") || svar == "nodot") && CALCULATOR->getDecimalPoint() != DOT) SET_BOOL_PREF("preferences_checkbutton_dot_as_separator")
	else if((equalsIgnoreCase(svar, "ignore comma") || svar == "nocomma") && CALCULATOR->getDecimalPoint() != COMMA) SET_BOOL_PREF("preferences_checkbutton_comma_as_separator")
	else if(equalsIgnoreCase(svar, "decimal comma")) {
		int v = -2;
		if(equalsIgnoreCase(svalue, "off")) v = 0;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = 1;
		else if(equalsIgnoreCase(svalue, "locale")) v = -1;
		else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < -1 || v > 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(!preferences_builder) get_preferences_dialog();
			if(v >= 0) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma")), v);
			else b_decimal_comma = v;*/
		}
	} else if(equalsIgnoreCase(svar, "limit implicit multiplication") || svar == "limimpl") SET_BOOL_MENU("menu_item_limit_implicit_multiplication")
	else if(equalsIgnoreCase(svar, "spacious") || svar == "space") SET_BOOL_D(settings->printops.spacious)
	else if(equalsIgnoreCase(svar, "unicode") || svar == "uni") SET_BOOL_PREF("preferences_checkbutton_unicode_signs")
	else if(equalsIgnoreCase(svar, "units") || svar == "unit") SET_BOOL_MENU("menu_item_enable_units")
	else if(equalsIgnoreCase(svar, "unknowns") || svar == "unknown") SET_BOOL_MENU("menu_item_enable_unknown_variables")
	else if(equalsIgnoreCase(svar, "variables") || svar == "var") SET_BOOL_MENU("menu_item_enable_variables")
	else if(equalsIgnoreCase(svar, "abbreviations") || svar == "abbr" || svar == "abbrev") SET_BOOL_MENU("menu_item_abbreviate_names")
	else if(equalsIgnoreCase(svar, "show ending zeroes") || svar == "zeroes") SET_BOOL_MENU("menu_item_show_ending_zeroes")
	else if(equalsIgnoreCase(svar, "repeating decimals") || svar == "repdeci") SET_BOOL_MENU("menu_item_indicate_infinite_series")
	else if(equalsIgnoreCase(svar, "angle unit") || svar == "angle") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rad") || equalsIgnoreCase(svalue, "radians")) v = ANGLE_UNIT_RADIANS;
		else if(equalsIgnoreCase(svalue, "deg") || equalsIgnoreCase(svalue, "degrees")) v = ANGLE_UNIT_DEGREES;
		else if(equalsIgnoreCase(svalue, "gra") || equalsIgnoreCase(svalue, "gradians")) v = ANGLE_UNIT_GRADIANS;
		else if(equalsIgnoreCase(svalue, "none")) v = ANGLE_UNIT_NONE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 3) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(v == ANGLE_UNIT_DEGREES) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_degrees")), TRUE);
			else if(v == ANGLE_UNIT_RADIANS) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_radians")), TRUE);
			else if(v == ANGLE_UNIT_GRADIANS) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_gradians")), TRUE);
			else if(v == ANGLE_UNIT_NONE) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_default_angle_unit")), TRUE);*/
		}
	} else if(equalsIgnoreCase(svar, "caret as xor") || equalsIgnoreCase(svar, "xor^")) SET_BOOL_PREF("preferences_checkbutton_caret_as_xor")
	else if(equalsIgnoreCase(svar, "parsing mode") || svar == "parse" || svar == "syntax") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = PARSING_MODE_ADAPTIVE;
		else if(equalsIgnoreCase(svalue, "implicit first")) v = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
		else if(equalsIgnoreCase(svalue, "conventional")) v = PARSING_MODE_CONVENTIONAL;
		else if(equalsIgnoreCase(svalue, "chain")) v = PARSING_MODE_CHAIN;
		else if(equalsIgnoreCase(svalue, "rpn")) v = PARSING_MODE_RPN;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < PARSING_MODE_ADAPTIVE || v > PARSING_MODE_RPN) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(v == PARSING_MODE_ADAPTIVE) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
			else if(v == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ignore_whitespace")), TRUE);
			else if(v == PARSING_MODE_CONVENTIONAL) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_no_special_implicit_multiplication")), TRUE);
			else if(v == PARSING_MODE_CHAIN) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_chain_syntax")), TRUE);
			else if(v == PARSING_MODE_RPN) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);*/
		}
	} else if(equalsIgnoreCase(svar, "update exchange rates") || svar == "upxrates") {
		int v = -2;
		if(equalsIgnoreCase(svalue, "never")) {
			v = 0;
		} else if(equalsIgnoreCase(svalue, "ask")) {
			v = -1;
		} else {
			v = s2i(svalue);
		}
		if(v < -1) v = -1;
		/*if(!preferences_builder) get_preferences_dialog();
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_update_exchange_rates_spin_button")), v);*/
	} else if(equalsIgnoreCase(svar, "multiplication sign") || svar == "mulsign") {
		int v = -1;
		if(svalue == SIGN_MULTIDOT || svalue == ".") v = MULTIPLICATION_SIGN_DOT;
		else if(svalue == SIGN_MIDDLEDOT) v = MULTIPLICATION_SIGN_ALTDOT;
		else if(svalue == SIGN_MULTIPLICATION || svalue == "x") v = MULTIPLICATION_SIGN_X;
		else if(svalue == "*") v = MULTIPLICATION_SIGN_ASTERISK;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < MULTIPLICATION_SIGN_ASTERISK || v > MULTIPLICATION_SIGN_ALTDOT) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(!preferences_builder) get_preferences_dialog();
			switch(v) {
				case MULTIPLICATION_SIGN_DOT: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_dot")), TRUE);
					break;
				}
				case MULTIPLICATION_SIGN_ALTDOT: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_altdot")), TRUE);
					break;
				}
				case MULTIPLICATION_SIGN_X: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_ex")), TRUE);
					break;
				}
				default: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_asterisk")), TRUE);
					break;
				}
			}*/
		}
	} else if(equalsIgnoreCase(svar, "division sign") || svar == "divsign") {
		int v = -1;
		if(svalue == SIGN_DIVISION_SLASH) v = DIVISION_SIGN_DIVISION_SLASH;
		else if(svalue == SIGN_DIVISION) v = DIVISION_SIGN_DIVISION;
		else if(svalue == "/") v = DIVISION_SIGN_SLASH;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(!preferences_builder) get_preferences_dialog();
			switch(v) {
				case DIVISION_SIGN_DIVISION_SLASH: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division_slash")), TRUE);
					break;
				}
				case DIVISION_SIGN_DIVISION: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_division")), TRUE);
					break;
				}
				default: {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_slash")), TRUE);
					break;
				}
			}*/
		}
	} else if(equalsIgnoreCase(svar, "approximation") || svar == "appr" || svar == "approx") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "exact")) v = APPROXIMATION_EXACT;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(equalsIgnoreCase(svalue, "dual")) v = APPROXIMATION_APPROXIMATE + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "try exact") || svalue == "try") v = APPROXIMATION_TRY_EXACT;
		else if(equalsIgnoreCase(svalue, "approximate") || svalue == "approx") v = APPROXIMATION_APPROXIMATE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v > APPROXIMATION_APPROXIMATE + 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v < APPROXIMATION_EXACT || v > APPROXIMATION_APPROXIMATE) {
			CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(v == APPROXIMATION_EXACT) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_always_exact")), TRUE);
			else if(v == APPROXIMATION_TRY_EXACT) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_try_exact")), TRUE);
			else if(v == APPROXIMATION_APPROXIMATE) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_approximate")), TRUE);*/
		}
	} else if(equalsIgnoreCase(svar, "interval calculation") || svar == "ic" || equalsIgnoreCase(svar, "uncertainty propagation") || svar == "up") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "variance formula") || equalsIgnoreCase(svalue, "variance")) v = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		else if(equalsIgnoreCase(svalue, "interval arithmetic") || svalue == "iv") v = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < INTERVAL_CALCULATION_NONE || v > INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*switch(v) {
				case INTERVAL_CALCULATION_VARIANCE_FORMULA: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_variance")), TRUE);
					break;
				}
				case INTERVAL_CALCULATION_INTERVAL_ARITHMETIC: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_interval_arithmetic")), TRUE);
					break;
				}
				case INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_simple")), TRUE);
					break;
				}
				case INTERVAL_CALCULATION_NONE: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_ic_none")), TRUE);
					break;
				}
			}*/
		}
	} else if(equalsIgnoreCase(svar, "autoconversion") || svar == "conv") {
		int v = -1;
		MixedUnitsConversion muc = MIXED_UNITS_CONVERSION_DEFAULT;
		if(equalsIgnoreCase(svalue, "none")) {v = POST_CONVERSION_NONE;  muc = MIXED_UNITS_CONVERSION_NONE;}
		else if(equalsIgnoreCase(svalue, "best")) v = POST_CONVERSION_OPTIMAL_SI;
		else if(equalsIgnoreCase(svalue, "optimalsi") || svalue == "si") v = POST_CONVERSION_OPTIMAL_SI;
		else if(empty_value || equalsIgnoreCase(svalue, "optimal")) v = POST_CONVERSION_OPTIMAL;
		else if(equalsIgnoreCase(svalue, "base")) v = POST_CONVERSION_BASE;
		else if(equalsIgnoreCase(svalue, "mixed")) v = POST_CONVERSION_OPTIMAL + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v == 1) v = 3;
			else if(v == 3) v = 1;
		}
		if(v == POST_CONVERSION_OPTIMAL + 1) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_DEFAULT;
		} else if(v == 0) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_NONE;
		}
		if(v < 0 || v > POST_CONVERSION_OPTIMAL) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*switch(v) {
				case POST_CONVERSION_OPTIMAL: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_optimal")), TRUE);
					break;
				}
				case POST_CONVERSION_OPTIMAL_SI: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_optimal_si")), TRUE);
					break;
				}
				case POST_CONVERSION_BASE: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_base")), TRUE);
					break;
				}
				default: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_post_conversion_none")), TRUE);
					break;
				}
			}
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_mixed_units_conversion")), muc != MIXED_UNITS_CONVERSION_NONE);*/
		}
	} else if(equalsIgnoreCase(svar, "currency conversion") || svar == "curconv") SET_BOOL_PREF("preferences_checkbutton_local_currency_conversion")
	else if(equalsIgnoreCase(svar, "algebra mode") || svar == "alg") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = STRUCTURING_NONE;
		else if(equalsIgnoreCase(svalue, "simplify") || equalsIgnoreCase(svalue, "expand")) v = STRUCTURING_SIMPLIFY;
		else if(equalsIgnoreCase(svalue, "factorize") || svalue == "factor") v = STRUCTURING_FACTORIZE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > STRUCTURING_FACTORIZE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(v == STRUCTURING_FACTORIZE) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_factorize")), TRUE);
			else if(v == STRUCTURING_SIMPLIFY) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_algebraic_mode_simplify")), TRUE);
			else  {
				settings->evalops.structuring = (StructuringMode) v;
				settings->printops.allow_factorization = false;
				expressionCalculationUpdated();
			}*/
		}
	} else if(equalsIgnoreCase(svar, "exact")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), v > 0);
		}
	} else if(equalsIgnoreCase(svar, "ignore locale")) SET_BOOL_PREF("preferences_checkbutton_ignore_locale")
	else if(equalsIgnoreCase(svar, "save mode")) SET_BOOL_PREF("preferences_checkbutton_mode")
	else if(equalsIgnoreCase(svar, "save definitions") || svar == "save defs") SET_BOOL_PREF("preferences_checkbutton_save_defs")
	else if(equalsIgnoreCase(svar, "scientific notation") || svar == "exp mode" || svar == "exp") {
		int v = -1;
		bool valid = true;
		if(equalsIgnoreCase(svalue, "off")) v = EXP_NONE;
		else if(equalsIgnoreCase(svalue, "auto")) v = EXP_PRECISION;
		else if(equalsIgnoreCase(svalue, "pure")) v = EXP_PURE;
		else if(empty_value || equalsIgnoreCase(svalue, "scientific")) v = EXP_SCIENTIFIC;
		else if(equalsIgnoreCase(svalue, "engineering")) v = EXP_BASE_3;
		else if(svalue.find_first_not_of(SPACES NUMBERS MINUS) == std::string::npos) v = s2i(svalue);
		else valid = false;
		if(valid) {
			/*switch(v) {
				case EXP_PRECISION: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_normal")), TRUE);
					break;
				}
				case EXP_BASE_3: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_engineering")), TRUE);
					break;
				}
				case EXP_SCIENTIFIC: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_scientific")), TRUE);
					break;
				}
				case EXP_PURE: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_purely_scientific")), TRUE);
					break;
				}
				case EXP_NONE: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_display_non_scientific")), TRUE);
					break;
				}
				default: {
					settings->printops.min_exp = v;
					resultFormatUpdated();
				}
			}*/
		} else {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		}
	} else if(equalsIgnoreCase(svar, "precision") || svar == "prec") {
		int v = 0;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		if(v < 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			/*if(precision_builder) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision")), v);
			} else {
				CALCULATOR->setPrecision(v);
				expressionCalculationUpdated();
			}*/
		}
	} else if(equalsIgnoreCase(svar, "interval display") || svar == "ivdisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = 0;
		else if(equalsIgnoreCase(svalue, "significant")) v = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS + 1;
		else if(equalsIgnoreCase(svalue, "interval")) v = INTERVAL_DISPLAY_INTERVAL + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "plusminus")) v = INTERVAL_DISPLAY_PLUSMINUS + 1;
		else if(equalsIgnoreCase(svalue, "midpoint")) v = INTERVAL_DISPLAY_MIDPOINT + 1;
		else if(equalsIgnoreCase(svalue, "upper")) v = INTERVAL_DISPLAY_UPPER + 1;
		else if(equalsIgnoreCase(svalue, "lower")) v = INTERVAL_DISPLAY_LOWER + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v == 0) {
			//gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_adaptive")), TRUE);
		} else {
			v--;
			if(v < INTERVAL_DISPLAY_SIGNIFICANT_DIGITS || v > INTERVAL_DISPLAY_UPPER) {
				CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
			} else {
				/*switch(v) {
					case INTERVAL_DISPLAY_INTERVAL: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_interval")), TRUE); break;}
					case INTERVAL_DISPLAY_PLUSMINUS: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_plusminus")), TRUE); break;}
					case INTERVAL_DISPLAY_MIDPOINT: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_midpoint")), TRUE); break;}
					default: {gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_interval_significant")), TRUE); break;}
				}*/
			}
		}
	} else if(equalsIgnoreCase(svar, "interval arithmetic") || svar == "ia" || svar == "interval") SET_BOOL_MENU("menu_item_interval_arithmetic")
	else if(equalsIgnoreCase(svar, "variable units") || svar == "varunits") SET_BOOL_MENU("menu_item_enable_variable_units")
	else if(equalsIgnoreCase(svar, "color")) CALCULATOR->error(true, "Unsupported option: %s.", svar.c_str(), NULL);
	else if(equalsIgnoreCase(svar, "max decimals") || svar == "maxdeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		/*if(decimals_builder) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max")), v >= 0);
			if(v >= 0) gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), v);
		} else {
			if(v >= 0) settings->printops.max_decimals = v;
			settings->printops.use_max_decimals = v >= 0;
			resultFormatUpdated();
		}*/
	} else if(equalsIgnoreCase(svar, "min decimals") || svar == "mindeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		/*if(decimals_builder) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min")), v >= 0);
			if(v >= 0) gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), v);
		} else {
			if(v >= 0) settings->printops.min_decimals = v;
			settings->printops.use_min_decimals = v >= 0;
			resultFormatUpdated();
		}*/
	} else if(equalsIgnoreCase(svar, "fractions") || svar == "fr") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = FRACTION_DECIMAL;
		else if(equalsIgnoreCase(svalue, "exact")) v = FRACTION_DECIMAL_EXACT;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = FRACTION_FRACTIONAL;
		else if(equalsIgnoreCase(svalue, "combined") || equalsIgnoreCase(svalue, "mixed")) v = FRACTION_COMBINED;
		else if(equalsIgnoreCase(svalue, "long")) v = FRACTION_COMBINED + 1;
		else if(equalsIgnoreCase(svalue, "dual")) v = FRACTION_COMBINED + 2;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v > FRACTION_COMBINED + 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v < 0 || v > FRACTION_COMBINED + 1) {
			CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
		} else {
			/*int dff = default_fraction_fraction;
			switch(v > FRACTION_COMBINED ? FRACTION_FRACTIONAL : v) {
				case FRACTION_DECIMAL: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal")), TRUE);
					break;
				}
				case FRACTION_DECIMAL_EXACT: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_decimal_exact")), TRUE);
					break;
				}
				case FRACTION_COMBINED: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_combined")), TRUE);
					break;
				}
				case FRACTION_FRACTIONAL: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_fraction_fraction")), TRUE);
					if(v > FRACTION_COMBINED) {
						settings->printops.restrict_fraction_length = false;
						resultFormatUpdated();
					}
					break;
				}
			}
			default_fraction_fraction = dff;*/
		}
	} else if(equalsIgnoreCase(svar, "complex form") || svar == "cplxform") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rectangular") || equalsIgnoreCase(svalue, "cartesian") || svalue == "rect") v = COMPLEX_NUMBER_FORM_RECTANGULAR;
		else if(equalsIgnoreCase(svalue, "exponential") || svalue == "exp") v = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		else if(equalsIgnoreCase(svalue, "polar")) v = COMPLEX_NUMBER_FORM_POLAR;
		else if(equalsIgnoreCase(svalue, "angle") || equalsIgnoreCase(svalue, "phasor")) v = COMPLEX_NUMBER_FORM_CIS + 1;
		else if(svar == "cis") v = COMPLEX_NUMBER_FORM_CIS;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 4) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			switch(v) {
				/*case COMPLEX_NUMBER_FORM_RECTANGULAR: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_rectangular")), TRUE);
					break;
				}
				case COMPLEX_NUMBER_FORM_EXPONENTIAL: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_exponential")), TRUE);
					break;
				}
				case COMPLEX_NUMBER_FORM_POLAR: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
					break;
				}
				case COMPLEX_NUMBER_FORM_CIS: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_polar")), TRUE);
					break;
				}
				default: {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_complex_angle")), TRUE);
				}*/
			}
		}
	} else if(equalsIgnoreCase(svar, "read precision") || svar == "readprec") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DONT_READ_PRECISION;
		else if(equalsIgnoreCase(svalue, "always")) v = ALWAYS_READ_PRECISION;
		else if(empty_value || equalsIgnoreCase(svalue, "when decimals") || equalsIgnoreCase(svalue, "on")) v = READ_PRECISION_WHEN_DECIMALS;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == ALWAYS_READ_PRECISION) {
				settings->evalops.parse_options.read_precision = (ReadPrecisionMode) v;
				expressionFormatUpdated(true);
			} else {
				//gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_read_precision")), v != DONT_READ_PRECISION);
			}
		}
	} else {
		if(i_underscore == std::string::npos) {
			if(index != std::string::npos) {
				if((index = svar.find_last_of(SPACES)) != std::string::npos) {
					svar = svar.substr(0, index);
					remove_blank_ends(svar);
					str = str.substr(index + 1);
					remove_blank_ends(str);
					svalue = str;
					gsub("_", " ", svar);
					goto set_option_place;
				}
			}
			if(!empty_value && !svalue.empty()) {
				svar += " ";
				svar += svalue;
				svalue = "1";
				empty_value = true;
				goto set_option_place;
			}
		}
		CALCULATOR->error(true, "Unrecognized option: %s.", svar.c_str(), NULL);
	}
}

void QalculateWindow::calculateExpression(bool force, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, std::string execute_str, std::string str, bool check_exrates) {

	//if(block_expression_execution || exit_in_progress) return;

	std::string saved_execute_str = execute_str;

	if(b_busy) return;

	expressionEdit->hideCompletion();

	b_busy++;

	bool do_factors = false, do_pfe = false, do_expand = false, do_ceu = execute_str.empty(), do_bases = false, do_calendars = false;
	if(do_stack && !settings->rpn_mode) do_stack = false;
	if(do_stack && do_mathoperation && f && stack_index == 0) do_stack = false;
	if(!do_stack) stack_index = 0;

	//if(!mbak_convert.isUndefined() && stack_index == 0) mbak_convert.setUndefined();

	if(execute_str.empty()) {
		to_fraction = false; to_prefix = 0; to_base = 0; to_bits = 0; to_nbase.clear(); to_caf = -1;
	}
	bool current_expr = false;
	if(str.empty() && !do_mathoperation) {
		if(do_stack) {
			/*GtkTreeIter iter;
			gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, stack_index);
			gchar *gstr;
			gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
			str = gstr;
			g_free(gstr);*/
		} else {
			current_expr = true;
			str = expressionEdit->toPlainText().toStdString();
			if(!force && (expressionEdit->expressionHasChanged() || str.find_first_not_of(SPACES) == std::string::npos)) {
				b_busy--;
				return;
			}
			expressionEdit->setExpressionHasChanged(false);
			if(!do_mathoperation && !str.empty()) expressionEdit->addToHistory();
			askDot(str);
		}
	}

	std::string to_str, str_conv;

	if(execute_str.empty()) {
		bool double_tag = false;
		to_str = CALCULATOR->parseComments(str, settings->evalops.parse_options, &double_tag);
		if(!to_str.empty()) {
			if(str.empty()) {
				if(!double_tag) {
					expressionEdit->clear();
					CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
					historyView->addMessages();
					b_busy--;
					return;
				}
				execute_str = CALCULATOR->f_message->referenceName();
				execute_str += "(";
				execute_str += to_str;
				execute_str += ")";
			} else {
				CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
			}
		}
		// qalc command
		bool b_command = false;
		if(str[0] == '/' && str.length() > 1) {
			size_t i = str.find_first_not_of(SPACES, 1);
			if(i != std::string::npos && str[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, str[i])) {
				b_command = true;
			}
		}
		if(b_command) {
			str.erase(0, 1);
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			std::string scom;
			if(ispace == std::string::npos) {
				scom = "";
			} else {
				scom = str.substr(1, ispace);
			}
			if(equalsIgnoreCase(scom, "convert") || equalsIgnoreCase(scom, "to")) {
				str = std::string("to") + str.substr(ispace, slen - ispace);
				b_command = false;
			} else if((str.length() > 2 && str[0] == '-' && str[1] == '>') || (str.length() > 3 && str[0] == '\xe2' && ((str[1] == '\x86' && str[2] == '\x92') || (str[1] == '\x9e' && (unsigned char) str[2] >= 148 && (unsigned char) str[3] <= 191)))) {
				b_command = false;
			} else if(str == "M+" || str == "M-" || str == "M−" || str == "MS" || str == "MC") {
				b_command = false;
			}
		}
		if(b_command) {
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			std::string scom;
			if(ispace == std::string::npos) {
				scom = "";
			} else {
				scom = str.substr(0, ispace);
			}
			b_busy--;
			if(equalsIgnoreCase(scom, "set")) {
				if(current_expr) setPreviousExpression();
				str = str.substr(ispace + 1, slen - (ispace + 1));
				setOption(str);
			} else if(equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(equalsIgnoreCase(str, "mode")) {
					settings->savePreferences();
				} else if(equalsIgnoreCase(str, "definitions")) {
					if(!CALCULATOR->saveDefinitions()) {
						QMessageBox::critical(this, tr("Error"), tr("Couldn't write definitions"), QMessageBox::Ok);
					}
				} else {
					std::string name = str, cat, title;
					if(str[0] == '\"') {
						size_t i = str.find('\"', 1);
						if(i != std::string::npos) {
							name = str.substr(1, i - 1);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
					} else {
						size_t i = str.find_first_of(SPACES, 1);
						if(i != std::string::npos) {
							name = str.substr(0, i);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
						bool catset = false;
						if(str.empty()) {
							cat = CALCULATOR->temporaryCategory();
						} else {
							if(str[0] == '\"') {
								size_t i = str.find('\"', 1);
								if(i != std::string::npos) {
									cat = str.substr(1, i - 1);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							} else {
								size_t i = str.find_first_of(SPACES, 1);
								if(i != std::string::npos) {
									cat = str.substr(0, i);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							}
							catset = true;
						}
						bool b = true;
						if(!CALCULATOR->variableNameIsValid(name)) {
							CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
							b = false;
						}
						Variable *v = NULL;
						if(b) v = CALCULATOR->getActiveVariable(name);
						if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
							CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
							b = false;
						}
						if(b) {
							if(v && v->isLocal() && v->isKnown()) {
								if(catset) v->setCategory(cat);
								if(!title.empty()) v->setTitle(title);
								((KnownVariable*) v)->set(*mstruct);
								if(v->countNames() == 0) {
									ExpressionName ename(name);
									ename.reference = true;
									v->setName(ename, 1);
								} else {
									v->setName(name, 1);
								}
							} else {
								CALCULATOR->addVariable(new KnownVariable(cat, name, *mstruct, title));
							}
							//update_vmenu();
						}
					}
				}
			} else if(equalsIgnoreCase(scom, "variable")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				std::string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != std::string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != std::string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->variableNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				Variable *v = NULL;
				if(b) v = CALCULATOR->getActiveVariable(name);
				if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
					CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(v && v->isLocal() && v->isKnown()) {
						((KnownVariable*) v)->set(expr);
						if(v->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							v->setName(ename, 1);
						} else {
							v->setName(name, 1);
						}
					} else {
						CALCULATOR->addVariable(new KnownVariable("", name, expr));
					}
					//update_vmenu();
				}
			} else if(equalsIgnoreCase(scom, "function")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				std::string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != std::string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != std::string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->functionNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				MathFunction *f = CALCULATOR->getActiveFunction(name);
				if(b && ((!f && CALCULATOR->functionNameTaken(name)) || (f && (!f->isLocal() || f->subtype() != SUBTYPE_USER_FUNCTION)))) {
					CALCULATOR->error(true, "A function with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(expr.find("\\") == std::string::npos) {
						gsub("x", "\\x", expr);
						gsub("y", "\\y", expr);
						gsub("z", "\\z", expr);
					}
					if(f && f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) {
						((UserFunction*) f)->setFormula(expr);
						if(f->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							f->setName(ename, 1);
						} else {
							f->setName(name, 1);
						}
					} else {
						CALCULATOR->addFunction(new UserFunction("", name, expr));
					}
					//update_fmenu();
				}
			} else if(equalsIgnoreCase(scom, "delete")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				Variable *v = CALCULATOR->getActiveVariable(str);
				if(v && v->isLocal()) {
					v->destroy();
					//update_vmenu();
				} else {
					MathFunction *f = CALCULATOR->getActiveFunction(str);
					if(f && f->isLocal()) {
						f->destroy();
						//update_fmenu();
					} else {
						CALCULATOR->error(true, "No user-defined variable or function with the specified name (%s) exist.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(scom, "base")) {
				if(current_expr) setPreviousExpression();
				setOption(str);
			} else if(equalsIgnoreCase(scom, "assume")) {
				if(current_expr) setPreviousExpression();
				std::string str2 = "assumptions ";
				setOption(str2 + str.substr(ispace + 1, slen - (ispace + 1)));
			} else if(equalsIgnoreCase(scom, "rpn")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				/*if(equalsIgnoreCase(str, "syntax")) {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), FALSE);
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
				} else if(equalsIgnoreCase(str, "stack")) {
					if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
					}
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), TRUE);
				} else {
					int v = s2b(str);
					if(v < 0) {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					} else if(v) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), TRUE);
					} else {
						if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
							gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
						}
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), FALSE);
					}
				}*/
			} else if(equalsIgnoreCase(str, "exrates")) {
				if(current_expr) setPreviousExpression();
				fetchExchangeRates();
			} else if(equalsIgnoreCase(str, "stack")) {
				//gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), TRUE);
			} else if(equalsIgnoreCase(str, "swap")) {
				/*if(CALCULATOR->RPNStackSize() > 1) {
					gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_registerswap_clicked(NULL, NULL);
				}*/
			} else if(equalsIgnoreCase(scom, "swap")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					int index1 = 0, index2 = 0;
					str = str.substr(ispace + 1, slen - (ispace + 1));
					std::string str2 = "";
					remove_blank_ends(str);
					ispace = str.find_first_of(SPACES);
					if(ispace != std::string::npos) {
						str2 = str.substr(ispace + 1, str.length() - (ispace + 1));
						str = str.substr(0, ispace);
						remove_blank_ends(str2);
						remove_blank_ends(str);
					}
					index1 = s2i(str);
					if(str2.empty()) index2 = 1;
					else index2 = s2i(str2);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index2 < 0) index2 = (int) CALCULATOR->RPNStackSize() + 1 + index2;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize() || (!str2.empty() && (index2 <= 0 || index2 > (int) CALCULATOR->RPNStackSize()))) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else if(index2 != 1 && index1 != 1) {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					} else if(index1 != index2) {
						if(index1 == 1) index1 = index2;
						/*GtkTreeIter iter;
						if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index1 - 1)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
							on_button_registerswap_clicked(NULL, NULL);
							gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						}*/
					}
				}
			} else if(equalsIgnoreCase(scom, "move")) {
				CALCULATOR->error(true, "Unsupported command: %s.", scom.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					/*gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_registerdown_clicked(NULL, NULL);*/
				}
			} else if(equalsIgnoreCase(scom, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					/*if(equalsIgnoreCase(str, "up")) {
						gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						on_button_registerup_clicked(NULL, NULL);
					} else if(equalsIgnoreCase(str, "down")) {
						gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						on_button_registerdown_clicked(NULL, NULL);
					} else {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					}*/
				}
			} else if(equalsIgnoreCase(str, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					/*gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_copyregister_clicked(NULL, NULL);*/
				}
			} else if(equalsIgnoreCase(scom, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						/*GtkTreeIter iter;
						if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index1 - 1)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
							on_button_copyregister_clicked(NULL, NULL);
							gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						}*/
					}
				}
			} else if(equalsIgnoreCase(str, "clear stack")) {
				//if(CALCULATOR->RPNStackSize() > 0) on_button_clearstack_clicked(NULL, NULL);
			} else if(equalsIgnoreCase(str, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					/*gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_deleteregister_clicked(NULL, NULL);*/
				}
			} else if(equalsIgnoreCase(scom, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						/*GtkTreeIter iter;
						if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index1 - 1)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
							on_button_deleteregister_clicked(NULL, NULL);
						}*/
					}
				}
			} else if(equalsIgnoreCase(str, "factor")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_FACTORIZE);
			} else if(equalsIgnoreCase(str, "partial fraction")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
			} else if(equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_EXPAND);
			} else if(equalsIgnoreCase(str, "exact")) {
				if(current_expr) setPreviousExpression();
				//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), TRUE);
			} else if(equalsIgnoreCase(str, "approximate") || str == "approx") {
				if(current_expr) setPreviousExpression();
				//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "button_exact")), FALSE);
			} else if(equalsIgnoreCase(str, "mode")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "help") || str == "?") {
				//show_help("index.html", gtk_builder_get_object(main_builder, "main_window"));
			} else if(equalsIgnoreCase(str, "list")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) {
				str = str.substr(ispace + 1);
				remove_blank_ends(str);
				/*char list_type = 0;
				GtkTreeIter iter;
				if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find")) {
					size_t i = str.find_first_of(SPACES);
					std::string str1, str2;
					if(i == std::string::npos) {
						str1 = str;
					} else {
						str1 = str.substr(0, i);
						str2 = str.substr(i + 1);
						remove_blank_ends(str2);
					}
					if(equalsIgnoreCase(str1, "currencies")) list_type = 'c';
					else if(equalsIgnoreCase(str1, "functions")) list_type = 'f';
					else if(equalsIgnoreCase(str1, "variables")) list_type = 'v';
					else if(equalsIgnoreCase(str1, "units")) list_type = 'u';
					else if(equalsIgnoreCase(str1, "prefixes")) list_type = 'p';
					if(list_type == 'c') {
						manage_units();
						std::string s_cat = CALCULATOR->u_euro->category();
						GtkTreeIter iter1;
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter1) && gtk_tree_model_iter_children(GTK_TREE_MODEL(tUnitCategories_store), &iter, &iter1)) {
							do {
								gchar *gstr;
								gtk_tree_model_get(GTK_TREE_MODEL(tUnitCategories_store), &iter, 0, &gstr, -1);
								if(s_cat == gstr) {
									gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
									g_free(gstr);
									break;
								}
								g_free(gstr);
							} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnitCategories_store), &iter));
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str2.c_str());
					} else if(list_type == 'f') {
						manage_functions();
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionCategories_store), &iter)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), str2.c_str());
					} else if(list_type == 'v') {
						manage_variables();
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str2.c_str());
					} else if(list_type == 'u') {
						manage_units();
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str2.c_str());
					} else if(list_type == 'p') {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					}
				}
				if(list_type == 0) {
					ExpressionItem *item = CALCULATOR->getActiveExpressionItem(str);
					if(item) {
						if(item->type() == TYPE_UNIT) {
							manage_units();
							if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
								gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
							}
							gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str.c_str());
						} else if(item->type() == TYPE_FUNCTION) {
							manage_functions();
							if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionCategories_store), &iter)) {
								gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
							}
							gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), str.c_str());
						} else if(item->type() == TYPE_VARIABLE) {
							manage_variables();
							if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariableCategories_store), &iter)) {
								gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
							}
							gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variables_builder, "variables_entry_search")), str.c_str());
						}
					} else {
						CALCULATOR->error(true, "No function, variable, or unit with the specified name (%s) was found.", str.c_str(), NULL);
					}
				}*/
			} else if(equalsIgnoreCase(str, "quit") || equalsIgnoreCase(str, "exit")) {
				QApplication::quit();
			} else {
				CALCULATOR->error(true, "Unknown command: %s.", str.c_str(), NULL);
			}
			/*GtkTextIter istart, iend;
			gtk_text_buffer_get_start_iter(expressionbuffer, &istart);
			gtk_text_buffer_get_end_iter(expressionbuffer, &iend);
			gtk_text_buffer_select_range(expressionbuffer, &istart, &iend);
			if(current_inhistory_index < 0) current_inhistory_index = 0;*/
			displayMessages(this);
			return;
		}
	}

	if(execute_str.empty()) {
		if(str == "MC") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMCClicked();
			return;
		} else if(str == "MS") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMSClicked();
			return;
		} else if(str == "M+") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMPlusClicked();
			return;
		} else if(str == "M-" || str == "M−") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMMinusClicked();
			return;
		}
	}

	ComplexNumberForm cnf_bak = settings->evalops.complex_number_form;
	bool b_units_saved = settings->evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = settings->evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = settings->evalops.mixed_units_conversion;

	bool had_to_expression = false;
	std::string from_str = str;
	bool last_is_space = !from_str.empty() && is_in(SPACES, from_str[from_str.length() - 1]);
	if(execute_str.empty() && CALCULATOR->separateToExpression(from_str, to_str, settings->evalops, true, !do_stack)) {
		remove_duplicate_blanks(to_str);
		had_to_expression = true;
		std::string str_left;
		std::string to_str1, to_str2;
		bool do_to = false;
		while(true) {
			if(!from_str.empty()) {
				if(last_is_space) to_str += " ";
				CALCULATOR->separateToExpression(to_str, str_left, settings->evalops, true, false);
				remove_blank_ends(to_str);
			}
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != std::string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, tr("hexadecimal").toStdString())) {
				to_base = BASE_HEXADECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, tr("octal").toStdString())) {
				to_base = BASE_OCTAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "dec") || equalsIgnoreCase(to_str, "decimal") || equalsIgnoreCase(to_str, tr("decimal").toStdString())) {
				to_base = BASE_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "duo") || equalsIgnoreCase(to_str, "duodecimal") || equalsIgnoreCase(to_str, tr("duodecimal").toStdString())) {
				to_base = BASE_DUODECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, tr("binary").toStdString())) {
				to_base = BASE_BINARY;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, tr("roman").toStdString())) {
				to_base = BASE_ROMAN_NUMERALS;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, tr("bijective").toStdString())) {
				to_base = BASE_BIJECTIVE_26;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, tr("sexagesimal").toStdString())) {
				to_base = BASE_SEXAGESIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "2")) {
				to_base = BASE_SEXAGESIMAL_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "3")) {
				to_base = BASE_SEXAGESIMAL_3;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "latitude") || equalsIgnoreCase(to_str, tr("latitude").toStdString())) {
				to_base = BASE_LATITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", tr("latitude"), "2")) {
				to_base = BASE_LATITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "longitude") || equalsIgnoreCase(to_str, tr("longitude").toStdString())) {
				to_base = BASE_LONGITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", tr("longitude"), "2")) {
				to_base = BASE_LONGITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
				to_base = BASE_FP32;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
				to_base = BASE_FP64;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
				to_base = BASE_FP16;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp80")) {
				to_base = BASE_FP80;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
				to_base = BASE_FP128;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, tr("time").toStdString())) {
				to_base = BASE_TIME;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "Unicode")) {
				to_base = BASE_UNICODE;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				settings->printops.time_zone = TIME_ZONE_UTC;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.custom_time_zone = 0;
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_BINARY;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_HEXADECIMAL;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
				to_str = to_str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					CALCULATOR->error(true, tr("Time zone parsing failed.").toUtf8().data(),  NULL);
				}
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = itz;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.custom_time_zone = 0;
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str == "CET") {
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = 60;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.custom_time_zone = 0;
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, tr("bases").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					if(current_expr) setPreviousExpression();
					//convert_number_bases(result_text.c_str());
					return;
				}
				do_bases = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, tr("calendars").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					if(current_expr) setPreviousExpression();
					//on_popup_menu_item_calendarconversion_activate(NULL, NULL);
					return;
				}
				do_calendars = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "rectangular") || equalsIgnoreCase(to_str, "cartesian") || equalsIgnoreCase(to_str, tr("rectangular").toStdString()) || equalsIgnoreCase(to_str, tr("cartesian").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "exponential") || equalsIgnoreCase(to_str, tr("exponential").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "polar") || equalsIgnoreCase(to_str, tr("polar").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				to_caf = 0;
				do_to = true;
			} else if(to_str == "cis") {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "phasor") || equalsIgnoreCase(to_str, tr("phasor").toStdString()) || equalsIgnoreCase(to_str, "angle") || equalsIgnoreCase(to_str, tr("angle").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				to_caf = 1;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, tr("optimal").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_OPTIMAL);
					if(current_expr) setPreviousExpression();
					return;
				}
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, tr("base").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_BASE);
					if(current_expr) setPreviousExpression();
					return;
				}
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_BASE;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, tr("mixed").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_NONE;
				settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				if(from_str.empty()) {
					b_busy--;
					if(!previous_expression.empty()) calculateExpression(force, do_mathoperation, op, f, do_stack, stack_index, previous_expression);
					if(current_expr) setPreviousExpression();
					settings->evalops.auto_post_conversion = save_auto_post_conversion;
					settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
					settings->evalops.parse_options.units_enabled = b_units_saved;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fraction") || equalsIgnoreCase(to_str, tr("fraction").toStdString())) {
				do_to = true;
				to_fraction = true;
			} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, tr("factors").toStdString()) || equalsIgnoreCase(to_str, "factor")) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_FACTORIZE);
					if(current_expr) setPreviousExpression();
					return;
				}
				do_factors = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, tr("partial fraction").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
					if(current_expr) setPreviousExpression();
					return;
				}
				do_pfe = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, tr("base").toStdString())) {
				base_from_string(to_str2, to_base, to_nbase);
				do_to = true;
			} else if(from_str.empty()) {
				b_busy--;
				executeCommand(COMMAND_CONVERT_STRING, true, CALCULATOR->unlocalizeExpression(to_str, settings->evalops.parse_options));
				if(current_expr) setPreviousExpression();
				return;
			} else {
				if(to_str[0] == '?') {
					to_prefix = 1;
				} else if(to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
					to_prefix = to_str[0];

				}
				do_to = true;
				if(!str_conv.empty()) str_conv += " to ";
				str_conv += to_str;
			}
			if(str_left.empty()) break;
			to_str = str_left;
		}
		if(do_to) {
			if(from_str.empty()) {
				b_busy--;
				setResult(NULL, true, false, false);
				if(current_expr) setPreviousExpression();
				return;
			} else {
				execute_str = from_str;
				if(!str_conv.empty()) {
					execute_str += " to ";
					execute_str += str_conv;
				}
			}
		}
	}
	if(execute_str.empty()) {
		size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
		if(i != std::string::npos) {
			to_str = str.substr(0, i);
			if(to_str == "factor" || equalsIgnoreCase(to_str, "factorize") || equalsIgnoreCase(to_str, tr("factorize").toStdString())) {
				execute_str = str.substr(i + 1);
				do_factors = true;
			} else if(equalsIgnoreCase(to_str, "expand") || equalsIgnoreCase(to_str, tr("expand").toStdString())) {
				execute_str = str.substr(i + 1);
				do_expand = true;
			}
		}
	}

	size_t stack_size = 0;

	/*if(do_ceu && str_conv.empty() && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion"))) && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		std::string ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), pa);
		remove_blank_ends(ceu_str);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {
			if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
				ceu_str = "?" + ceu_str;
			}
		}
		if(ceu_str.empty()) {
			parsed_tostruct->setUndefined();
		} else {
			if(ceu_str[0] == '?') {
				to_prefix = 1;
			} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
				to_prefix = ceu_str[0];
			}
			parsed_tostruct->set(ceu_str);
		}
	} else {*/
		parsed_tostruct->setUndefined();
	//}
	CALCULATOR->resetExchangeRatesUsed();
	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation && f) {
			CALCULATOR->getRPNRegister(stack_index + 1)->transform(f);
			parsed_mstruct->set(*CALCULATOR->getRPNRegister(stack_index + 1));
			CALCULATOR->calculateRPNRegister(stack_index + 1, 0, settings->evalops);
		} else {
			CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options), 0, settings->evalops, parsed_mstruct, parsed_tostruct);
		}
	} else if(settings->rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			if(mstruct) lastx = *mstruct;
			//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx")), TRUE);
			if(f) CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, settings->evalops, parsed_mstruct);
		} else {
			std::string str2 = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options);
			CALCULATOR->parseSigns(str2);
			remove_blank_ends(str2);
			MathStructure lastx_bak(lastx);
			if(mstruct) lastx = *mstruct;
			if(str2.length() == 1) {
				do_mathoperation = true;
				switch(str2[0]) {
					case '^': {CALCULATOR->calculateRPN(OPERATION_RAISE, 0, settings->evalops, parsed_mstruct); break;}
					case '+': {CALCULATOR->calculateRPN(OPERATION_ADD, 0, settings->evalops, parsed_mstruct); break;}
					case '-': {CALCULATOR->calculateRPN(OPERATION_SUBTRACT, 0, settings->evalops, parsed_mstruct); break;}
					case '*': {CALCULATOR->calculateRPN(OPERATION_MULTIPLY, 0, settings->evalops, parsed_mstruct); break;}
					case '/': {CALCULATOR->calculateRPN(OPERATION_DIVIDE, 0, settings->evalops, parsed_mstruct); break;}
					case '&': {CALCULATOR->calculateRPN(OPERATION_BITWISE_AND, 0, settings->evalops, parsed_mstruct); break;}
					case '|': {CALCULATOR->calculateRPN(OPERATION_BITWISE_OR, 0, settings->evalops, parsed_mstruct); break;}
					case '~': {CALCULATOR->calculateRPNBitwiseNot(0, settings->evalops, parsed_mstruct); break;}
					case '!': {CALCULATOR->calculateRPN(CALCULATOR->f_factorial, 0, settings->evalops, parsed_mstruct); break;}
					case '>': {CALCULATOR->calculateRPN(OPERATION_GREATER, 0, settings->evalops, parsed_mstruct); break;}
					case '<': {CALCULATOR->calculateRPN(OPERATION_LESS, 0, settings->evalops, parsed_mstruct); break;}
					case '=': {CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, settings->evalops, parsed_mstruct); break;}
					case '\\': {
						MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
						if(fdiv) {
							CALCULATOR->calculateRPN(fdiv, 0, settings->evalops, parsed_mstruct);
							break;
						}
					}
					default: {do_mathoperation = false;}
				}
			} else if(str2.length() == 2) {
				if(str2 == "**") {
					CALCULATOR->calculateRPN(OPERATION_RAISE, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!!") {
					CALCULATOR->calculateRPN(CALCULATOR->f_factorial2, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!=" || str == "=!" || str == "<>") {
					CALCULATOR->calculateRPN(OPERATION_NOT_EQUALS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "<=" || str == "=<") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_LESS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == ">=" || str == "=>") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_GREATER, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "==") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "//") {
					MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
					if(fdiv) {
						CALCULATOR->calculateRPN(fdiv, 0, settings->evalops, parsed_mstruct);
						do_mathoperation = true;
					}
				}
			} else if(str2.length() == 3) {
				if(str2 == "⊻") {
					CALCULATOR->calculateRPN(OPERATION_BITWISE_XOR, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				}
			}
			if(!do_mathoperation) {
				bool had_nonnum = false, test_function = true;
				int in_par = 0;
				for(size_t i = 0; i < str2.length(); i++) {
					if(is_in(NUMBERS, str2[i])) {
						if(!had_nonnum || in_par) {
							test_function = false;
							break;
						}
					} else if(str2[i] == '(') {
						if(in_par || !had_nonnum) {
							test_function = false;
							break;
						}
						in_par = i;
					} else if(str2[i] == ')') {
						if(i != str2.length() - 1) {
							test_function = false;
							break;
						}
					} else if(str2[i] == ' ') {
						if(!in_par) {
							test_function = false;
							break;
						}
					} else if(is_in(NOT_IN_NAMES, str2[i])) {
						test_function = false;
						break;
					} else {
						if(in_par) {
							test_function = false;
							break;
						}
						had_nonnum = true;
					}
				}
				f = NULL;
				if(test_function) {
					if(in_par) f = CALCULATOR->getActiveFunction(str2.substr(0, in_par));
					else f = CALCULATOR->getActiveFunction(str2);
				}
				if(f && f->minargs() > 0) {
					do_mathoperation = true;
					original_expression = "";
					CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
				} else {
					original_expression = str2;
					CALCULATOR->RPNStackEnter(str2, 0, settings->evalops, parsed_mstruct, parsed_tostruct);
				}
				/*if(do_mathoperation) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx")), TRUE);
				else lastx = lastx_bak;*/
			}
		}
	} else {
		original_expression = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options);
		CALCULATOR->calculate(mstruct, original_expression, 0, settings->evalops, parsed_mstruct, parsed_tostruct);
	}

	bool title_set = false, was_busy = false;

	QProgressDialog *dialog = NULL;

	do_progress:
	int i = 0;
	while(CALCULATOR->busy() && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(CALCULATOR->busy() && !was_busy) {
		if(updateWindowTitle(tr("Calculating…"))) title_set = true;
		dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(CALCULATOR->busy()) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(!do_mathoperation && !str_conv.empty() && parsed_tostruct->containsType(STRUCT_UNIT, true) && !mstruct->containsType(STRUCT_UNIT) && !parsed_mstruct->containsType(STRUCT_UNIT, false, true, true) && !CALCULATOR->hasToExpression(str_conv, false, settings->evalops)) {
		MathStructure to_struct(*parsed_tostruct);
		to_struct.unformat();
		to_struct = CALCULATOR->convertToOptimalUnit(to_struct, settings->evalops, true);
		fix_to_struct(to_struct);
		if(!to_struct.isZero()) {
			mstruct->multiply(to_struct);
			PrintOptions po = settings->printops;
			po.negative_exponents = false;
			to_struct.format(po);
			if(to_struct.isMultiplication() && to_struct.size() >= 2) {
				if(to_struct[0].isOne()) to_struct.delChild(1, true);
				else if(to_struct[1].isOne()) to_struct.delChild(2, true);
			}
			parsed_mstruct->multiply(to_struct);
			CALCULATOR->calculate(mstruct, 0, settings->evalops, CALCULATOR->unlocalizeExpression(str_conv, settings->evalops.parse_options));
			str_conv = "";
			goto do_progress;
		}
	}

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	b_busy--;

	if(settings->rpn_mode && stack_index == 0) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
	}

	if(do_stack && stack_index > 0) {
	} else if(settings->rpn_mode && do_mathoperation) {
		result_text = tr("RPN Operation").toStdString();
	} else {
		result_text = str;
	}
	settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
	if(settings->rpn_mode && stack_index == 0) {
		expressionEdit->clear();
		while(CALCULATOR->RPNStackSize() < stack_size) {
			//RPNRegisterRemoved(1);
			stack_size--;
		}
		if(CALCULATOR->RPNStackSize() > stack_size) {
			//RPNRegisterAdded("");
		}
	}

	if(settings->rpn_mode && do_mathoperation && parsed_tostruct && !parsed_tostruct->isUndefined() && parsed_tostruct->isSymbolic()) {
		mstruct->set(CALCULATOR->convert(*mstruct, parsed_tostruct->symbol(), settings->evalops));
	}

	// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
	if(!settings->rpn_mode && (!parsed_tostruct || parsed_tostruct->isUndefined()) && execute_str.empty() && !had_to_expression && (settings->evalops.approximation == APPROXIMATION_EXACT || settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) && parsed_mstruct && mstruct && ((parsed_mstruct->isMultiplication() && parsed_mstruct->size() == 2 && (*parsed_mstruct)[0].isNumber() && (*parsed_mstruct)[1].isUnit_exp() && parsed_mstruct->equals(*mstruct)) || (parsed_mstruct->isNegate() && (*parsed_mstruct)[0].isMultiplication() && (*parsed_mstruct)[0].size() == 2 && (*parsed_mstruct)[0][0].isNumber() && (*parsed_mstruct)[0][1].isUnit_exp() && mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1] == (*parsed_mstruct)[0][1] && (*mstruct)[0].isNumber() && (*parsed_mstruct)[0][0].number() == -(*mstruct)[0].number()) || (parsed_mstruct->isUnit_exp() && parsed_mstruct->equals(*mstruct)))) {
		Unit *u = NULL;
		MathStructure *munit = NULL;
		if(mstruct->isMultiplication()) munit = &(*mstruct)[1];
		else munit = mstruct;
		if(munit->isUnit()) u = munit->unit();
		else u = (*munit)[0].unit();
		if(u && u->isCurrency()) {
			if(settings->evalops.local_currency_conversion && CALCULATOR->getLocalCurrency() && u != CALCULATOR->getLocalCurrency()) {
				ApproximationMode abak = settings->evalops.approximation;
				if(settings->evalops.approximation == APPROXIMATION_EXACT) settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = abak;
			}
		} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
			MathStructure mbak(*mstruct);
			if(settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) {
				if(munit->isUnit() && u->referenceName() == "oF") {
					u = CALCULATOR->getActiveUnit("oC");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, settings->evalops, true, false));
				} else {
					mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				}
			}
			if(settings->evalops.approximation == APPROXIMATION_EXACT && ((settings->evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL && settings->evalops.auto_post_conversion != POST_CONVERSION_NONE) || mstruct->equals(mbak))) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				if(settings->evalops.auto_post_conversion == POST_CONVERSION_BASE) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, settings->evalops));
				else mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = APPROXIMATION_EXACT;
			}
		}
	}

	if(!do_mathoperation && (askTC(*parsed_mstruct) || (check_exrates && checkExchangeRates()))) {
		calculateExpression(force, do_mathoperation, op, f, settings->rpn_mode, stack_index, saved_execute_str, str, false);
		settings->evalops.complex_number_form = cnf_bak;
		settings->evalops.auto_post_conversion = save_auto_post_conversion;
		settings->evalops.parse_options.units_enabled = b_units_saved;
		settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
		settings->printops.custom_time_zone = 0;
		settings->printops.time_zone = TIME_ZONE_LOCAL;
		return;
	}

	//update "ans" variables
	if(stack_index == 0) {
		MathStructure m4(settings->vans[3]->get());
		m4.replace(settings->vans[4], settings->vans[4]->get());
		settings->vans[4]->set(m4);
		MathStructure m3(settings->vans[2]->get());
		m3.replace(settings->vans[3], settings->vans[4]);
		settings->vans[3]->set(m3);
		MathStructure m2(settings->vans[1]->get());
		m2.replace(settings->vans[2], settings->vans[3]);
		settings->vans[2]->set(m2);
		MathStructure m1(settings->vans[0]->get());
		m1.replace(settings->vans[1], settings->vans[2]);
		settings->vans[1]->set(m1);
		mstruct->replace(settings->vans[0], settings->vans[1]);
		settings->vans[0]->set(*mstruct);
	}

	if(do_factors || do_pfe || do_expand) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			mstruct = save_mstruct;
		} else {
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS  : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
		}
	}

	if(!do_stack) previous_expression = execute_str.empty() ? str : execute_str;
	setResult(NULL, true, stack_index == 0, true, "", stack_index);
	
	/*if(do_bases) convert_number_bases(execute_str.c_str());
	if(do_calendars) on_popup_menu_item_calendarconversion_activate(NULL, NULL);*/
	
	settings->evalops.complex_number_form = cnf_bak;
	settings->evalops.auto_post_conversion = save_auto_post_conversion;
	settings->evalops.parse_options.units_enabled = b_units_saved;
	settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
	settings->printops.custom_time_zone = 0;
	settings->printops.time_zone = TIME_ZONE_LOCAL;

	if(stack_index == 0) {
		/*if(!block_conversion_category_switch) {
			Unit *u = CALCULATOR->findMatchingUnit(*mstruct);
			if(u && !u->category().empty()) {
				std::string s_cat = u->category();
				if(s_cat.empty()) s_cat = tr("Uncategorized").toStdString();
				if(s_cat != selected_unit_category) {
					GtkTreeIter iter = convert_category_map[s_cat];
					GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
					gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tUnitSelectorCategories), path);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tUnitSelectorCategories), path, NULL, TRUE, 0.5, 0);
					gtk_tree_path_free(path);
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
				}
			}
			if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")))) {
				gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)));
			}
		}*/
		expressionEdit->blockCompletion(true);
		expressionEdit->blockParseStatus(true);
		if(!execute_str.empty()) {
			from_str = execute_str;
			CALCULATOR->separateToExpression(from_str, str, settings->evalops, true, true);
		}
		if(!exact_text.empty() && unicode_length(exact_text) < unicode_length(from_str)) {
			if(exact_text == "0") expressionEdit->clear();
			else expressionEdit->setPlainText(QString::fromStdString(exact_text));
		} else {
			expressionEdit->setPlainText(QString::fromStdString(from_str));
		}
		if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
		expressionEdit->selectAll();
		expressionEdit->blockCompletion(false);
		expressionEdit->blockParseStatus(false);
		expressionEdit->setExpressionHasChanged(false);
	}
}

/*void QalculateWindow::calculateExpression(bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, bool check_exrates) {

	std::string str, str_conv;
	bool do_bases = false, do_factors = false, do_expand = false, do_pfe = false, do_calendars = false, do_binary_prefixes = false;

	int save_base = settings->printops.base;
	bool save_pre = settings->printops.use_unit_prefixes;
	bool save_cur = settings->printops.use_prefixes_for_currencies;
	bool save_den = settings->printops.use_denominator_prefix;
	bool save_allu = settings->printops.use_prefixes_for_all_units;
	bool save_all = settings->printops.use_all_prefixes;
	int save_bin = CALCULATOR->usesBinaryPrefixes();
	ComplexNumberForm save_complex_number_form = settings->evalops.complex_number_form;
	bool caf_bak = settings->complex_angle_form;
	bool b_units_saved = settings->evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = settings->evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = settings->evalops.mixed_units_conversion;
	NumberFractionFormat save_format = settings->printops.number_fraction_format;
	bool save_rfl = settings->printops.restrict_fraction_length;
	Number save_cbase;
	bool custom_base_set = false;
	bool had_to_expression = false;

	if(do_stack) {
	} else {
		str = expressionEdit->expression();
		std::string to_str = CALCULATOR->parseComments(str, settings->evalops.parse_options);
		if(!to_str.empty() && str.empty()) return;

		if(str == "MC") {
			settings->v_memory->set(m_zero);
			return;
		} else if(str == "MS") {
			if(mstruct) settings->v_memory->set(*mstruct);
			return;
		} else if(str == "M+") {
			if(mstruct) {
				MathStructure m = settings->v_memory->get();
				m.calculateAdd(*mstruct, settings->evalops);
				settings->v_memory->set(m);
			}
			return;
		} else if(str == "M-" || str == "M−") {
			if(mstruct) {
				MathStructure m = settings->v_memory->get();
				m.calculateSubtract(*mstruct, settings->evalops);
				settings->v_memory->set(m);
			}
			return;
		}

		std::string from_str = str;
		askDot(str);
		if(CALCULATOR->separateToExpression(from_str, to_str, settings->evalops, true)) {
			had_to_expression = true;
			remove_duplicate_blanks(to_str);
			std::string str_left;
			std::string to_str1, to_str2;
			while(true) {
				CALCULATOR->separateToExpression(to_str, str_left, settings->evalops, true);
				remove_blank_ends(to_str);
				size_t ispace = to_str.find_first_of(SPACES);
				if(ispace != std::string::npos) {
					to_str1 = to_str.substr(0, ispace);
					remove_blank_ends(to_str1);
					to_str2 = to_str.substr(ispace + 1);
					remove_blank_ends(to_str2);
				}
				if(equalsIgnoreCase(to_str, "hex") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "hexadecimal", tr("hexadecimal"))) {
					settings->printops.base = BASE_HEXADECIMAL;
				} else if(equalsIgnoreCase(to_str, "bin") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "binary", tr("binary"))) {
					settings->printops.base = BASE_BINARY;
				} else if(equalsIgnoreCase(to_str, "dec") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "decimal", tr("decimal"))) {
					settings->printops.base = BASE_DECIMAL;
				} else if(equalsIgnoreCase(to_str, "oct") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "octal", tr("octal"))) {
					settings->printops.base = BASE_OCTAL;
				} else if(equalsIgnoreCase(to_str, "duo") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "duodecimal", tr("duodecimal"))) {
					settings->printops.base = BASE_DUODECIMAL;
				} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, tr("roman").toStdString())) {
					settings->printops.base = BASE_ROMAN_NUMERALS;
				} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, tr("bijective").toStdString())) {
					settings->printops.base = BASE_BIJECTIVE_26;
				} else if(equalsIgnoreCase(to_str, "sexa") || EQUALS_IGNORECASE_AND_LOCAL(to_str, "sexagesimal", tr("sexagesimal"))) {
					settings->printops.base = BASE_SEXAGESIMAL;
				} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "2")) {
					settings->printops.base = BASE_SEXAGESIMAL_2;
				} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "3")) {
					settings->printops.base = BASE_SEXAGESIMAL_3;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "longitude", tr("longitude"))) {
					settings->printops.base = BASE_LONGITUDE;
				} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", tr("longitude"), "2")) {
					settings->printops.base = BASE_LONGITUDE_2;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "latitude", tr("latitude"))) {
					settings->printops.base = BASE_LATITUDE;
				} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", tr("latitude"), "2")) {
					settings->printops.base = BASE_LATITUDE_2;
				} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
					settings->printops.base = BASE_FP32;
				} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
					settings->printops.base = BASE_FP64;
				} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
					settings->printops.base = BASE_FP16;
				} else if(equalsIgnoreCase(to_str, "fp80")) {
					settings->printops.base = BASE_FP80;
				} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
					settings->printops.base = BASE_FP128;
				} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, tr("time").toStdString())) {
					settings->printops.base = BASE_TIME;
				} else if(equalsIgnoreCase(str, "unicode")) {
					settings->printops.base = BASE_UNICODE;
				} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
					settings->printops.time_zone = TIME_ZONE_UTC;
				} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
					settings->printops.base = BASE_BINARY;
					settings->printops.binary_bits = s2i(to_str.substr(3));
				} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
					settings->printops.base = BASE_HEXADECIMAL;
					settings->printops.binary_bits = s2i(to_str.substr(3));
				} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
					to_str = to_str.substr(3);
					remove_blanks(to_str);
					bool b_minus = false;
					if(to_str[0] == '+') {
						to_str.erase(0, 1);
					} else if(to_str[0] == '-') {
						b_minus = true;
						to_str.erase(0, 1);
					} else if(to_str.find(SIGN_MINUS) == 0) {
						b_minus = true;
						to_str.erase(0, strlen(SIGN_MINUS));
					}
					unsigned int tzh = 0, tzm = 0;
					int itz = 0;
					if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
						itz = tzh * 60 + tzm;
						if(b_minus) itz = -itz;
					} else {
						CALCULATOR->error(true, tr("Time zone parsing failed.").toUtf8().constData(), NULL);
					}
					settings->printops.time_zone = TIME_ZONE_CUSTOM;
					settings->printops.custom_time_zone = itz;
				} else if(to_str == "CET") {
					settings->printops.time_zone = TIME_ZONE_CUSTOM;
					settings->printops.custom_time_zone = 60;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "fraction", tr("fraction")) || to_str == "frac") {
					settings->printops.restrict_fraction_length = false;
					settings->printops.number_fraction_format = FRACTION_COMBINED;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "factors", tr("factors")) || to_str == "factor") {
					do_factors = true;
				}  else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, tr("partial fraction").toStdString()) || to_str == "partial") {
					do_pfe = true;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "bases", tr("bases"))) {
					do_bases = true;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "calendars", tr("calendars"))) {
					do_calendars = true;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "rectangular", tr("rectangular")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "cartesian", tr("cartesian")) || to_str == "rect") {
					settings->complex_angle_form = false;
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "exponential", tr("exponential")) || to_str == "exp") {
					settings->complex_angle_form = false;
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "polar", tr("polar"))) {
					settings->complex_angle_form = false;
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "angle", tr("angle")) || EQUALS_IGNORECASE_AND_LOCAL(to_str, "phasor", tr("phasor"))) {
					settings->complex_angle_form = true;
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				} else if(to_str == "cis") {
					settings->complex_angle_form = false;
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "optimal", tr("optimal"))) {
					settings->evalops.parse_options.units_enabled = true;
					settings->evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
					str_conv = "";
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "base", tr("base", "units"))) {
					settings->evalops.parse_options.units_enabled = true;
					settings->evalops.auto_post_conversion = POST_CONVERSION_BASE;
					str_conv = "";
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str1, "base", tr("base"))) {
					if(to_str2 == "b26" || to_str2 == "B26") settings->printops.base = BASE_BIJECTIVE_26;
					else if(equalsIgnoreCase(to_str2, "golden") || equalsIgnoreCase(to_str2, "golden ratio") || to_str2 == "φ") settings->printops.base = BASE_GOLDEN_RATIO;
					else if(equalsIgnoreCase(to_str2, "unicode")) settings->printops.base = BASE_UNICODE;
					else if(equalsIgnoreCase(to_str2, "supergolden") || equalsIgnoreCase(to_str2, "supergolden ratio") || to_str2 == "ψ") settings->printops.base = BASE_SUPER_GOLDEN_RATIO;
					else if(equalsIgnoreCase(to_str2, "pi") || to_str2 == "π") settings->printops.base = BASE_PI;
					else if(to_str2 == "e") settings->printops.base = BASE_E;
					else if(to_str2 == "sqrt(2)" || to_str2 == "sqrt 2" || to_str2 == "sqrt2" || to_str2 == "√2") settings->printops.base = BASE_SQRT2;
					else {
						EvaluationOptions eo = settings->evalops;
						eo.parse_options.base = 10;
						MathStructure m;
						eo.approximation = APPROXIMATION_TRY_EXACT;
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(to_str2, eo.parse_options), 500, eo);
						if(CALCULATOR->endTemporaryStopMessages()) {
							settings->printops.base = BASE_CUSTOM;
							custom_base_set = true;
							save_cbase = CALCULATOR->customOutputBase();
							CALCULATOR->setCustomOutputBase(nr_zero);
						} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
							settings->printops.base = m.number().intValue();
						} else {
							settings->printops.base = BASE_CUSTOM;
							custom_base_set = true;
							save_cbase = CALCULATOR->customOutputBase();
							CALCULATOR->setCustomOutputBase(m.number());
						}
					}
				} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "mixed", tr("mixed", "units"))) {
					settings->evalops.auto_post_conversion = POST_CONVERSION_NONE;
					settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				} else {
					settings->evalops.parse_options.units_enabled = true;
					if((to_str[0] == '?' && (!settings->printops.use_unit_prefixes || !settings->printops.use_prefixes_for_currencies || !settings->printops.use_prefixes_for_all_units)) || (to_str.length() > 1 && to_str[1] == '?' && to_str[0] == 'a' && (!settings->printops.use_unit_prefixes || !settings->printops.use_prefixes_for_currencies || !settings->printops.use_all_prefixes || !settings->printops.use_prefixes_for_all_units)) || (to_str.length() > 1 && to_str[1] == '?' && to_str[0] == 'd' && (!settings->printops.use_unit_prefixes || !settings->printops.use_prefixes_for_currencies || !settings->printops.use_prefixes_for_all_units || CALCULATOR->usesBinaryPrefixes() > 0))) {
						settings->printops.use_unit_prefixes = true;
						settings->printops.use_prefixes_for_currencies = true;
						settings->printops.use_prefixes_for_all_units = true;
						if(to_str[0] == 'a') settings->printops.use_all_prefixes = true;
						else if(to_str[0] == 'd') CALCULATOR->useBinaryPrefixes(0);
					} else if(to_str.length() > 1 && to_str[1] == '?' && to_str[0] == 'b') {
						do_binary_prefixes = true;
					}
					if(!str_conv.empty()) str_conv += " to ";
					str_conv += to_str;
				}
				if(str_left.empty()) break;
				to_str = str_left;
			}
			str = from_str;
			if(!str_conv.empty()) {
				str += " to ";
				str += str_conv;
			}
		}
		size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
		if(i != std::string::npos) {
			to_str = str.substr(0, i);
			if(to_str == "factor" || EQUALS_IGNORECASE_AND_LOCAL(to_str, "factorize", tr("factorize"))) {
				str = str.substr(i + 1);
				do_factors = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL(to_str, "expand", tr("expand"))) {
				str = str.substr(i + 1);
				do_expand = true;
			}
		}
	}

	b_busy++;

	size_t stack_size = 0;

	CALCULATOR->resetExchangeRatesUsed();

	MathStructure to_struct;

	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options), 0, settings->evalops, parsed_mstruct, NULL);
	} else if(settings->rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			if(f) CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, settings->evalops, parsed_mstruct);
		} else {
			std::string str2 = CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options);
			CALCULATOR->parseSigns(str2);
			remove_blank_ends(str2);
			if(str2.length() == 1) {
				do_mathoperation = true;
				switch(str2[0]) {
					case '^': {CALCULATOR->calculateRPN(OPERATION_RAISE, 0, settings->evalops, parsed_mstruct); break;}
					case '+': {CALCULATOR->calculateRPN(OPERATION_ADD, 0, settings->evalops, parsed_mstruct); break;}
					case '-': {CALCULATOR->calculateRPN(OPERATION_SUBTRACT, 0, settings->evalops, parsed_mstruct); break;}
					case '*': {CALCULATOR->calculateRPN(OPERATION_MULTIPLY, 0, settings->evalops, parsed_mstruct); break;}
					case '/': {CALCULATOR->calculateRPN(OPERATION_DIVIDE, 0, settings->evalops, parsed_mstruct); break;}
					case '&': {CALCULATOR->calculateRPN(OPERATION_BITWISE_AND, 0, settings->evalops, parsed_mstruct); break;}
					case '|': {CALCULATOR->calculateRPN(OPERATION_BITWISE_OR, 0, settings->evalops, parsed_mstruct); break;}
					case '~': {CALCULATOR->calculateRPNBitwiseNot(0, settings->evalops, parsed_mstruct); break;}
					case '!': {CALCULATOR->calculateRPN(CALCULATOR->getFunctionById(FUNCTION_ID_FACTORIAL), 0, settings->evalops, parsed_mstruct); break;}
					case '>': {CALCULATOR->calculateRPN(OPERATION_GREATER, 0, settings->evalops, parsed_mstruct); break;}
					case '<': {CALCULATOR->calculateRPN(OPERATION_LESS, 0, settings->evalops, parsed_mstruct); break;}
					case '=': {CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, settings->evalops, parsed_mstruct); break;}
					case '\\': {
						MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
						if(fdiv) {
							CALCULATOR->calculateRPN(fdiv, 0, settings->evalops, parsed_mstruct);
							break;
						}
					}
					default: {do_mathoperation = false;}
				}
			} else if(str2.length() == 2) {
				if(str2 == "**") {
					CALCULATOR->calculateRPN(OPERATION_RAISE, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!!") {
					CALCULATOR->calculateRPN(CALCULATOR->getFunctionById(FUNCTION_ID_DOUBLE_FACTORIAL), 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!=" || str == "=!" || str == "<>") {
					CALCULATOR->calculateRPN(OPERATION_NOT_EQUALS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "<=" || str == "=<") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_LESS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == ">=" || str == "=>") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_GREATER, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "==") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "//") {
					MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
					if(fdiv) {
						CALCULATOR->calculateRPN(fdiv, 0, settings->evalops, parsed_mstruct);
						do_mathoperation = true;
					}
				}
			} else if(str2.length() == 3) {
				if(str2 == "⊻") {
					CALCULATOR->calculateRPN(OPERATION_BITWISE_XOR, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				}
			}
			if(!do_mathoperation) {
				bool had_nonnum = false, test_function = true;
				int in_par = 0;
				for(size_t i = 0; i < str2.length(); i++) {
					if(is_in(NUMBERS, str2[i])) {
						if(!had_nonnum || in_par) {
							test_function = false;
							break;
						}
					} else if(str2[i] == '(') {
						if(in_par || !had_nonnum) {
							test_function = false;
							break;
						}
						in_par = i;
					} else if(str2[i] == ')') {
						if(i != str2.length() - 1) {
							test_function = false;
							break;
						}
					} else if(str2[i] == ' ') {
						if(!in_par) {
							test_function = false;
							break;
						}
					} else if(is_in(NOT_IN_NAMES, str2[i])) {
						test_function = false;
						break;
					} else {
						if(in_par) {
							test_function = false;
							break;
						}
						had_nonnum = true;
					}
				}
				f = NULL;
				if(test_function) {
					if(in_par) f = CALCULATOR->getActiveFunction(str2.substr(0, in_par));
					else f = CALCULATOR->getActiveFunction(str2);
				}
				if(f && f->minargs() > 0) {
					do_mathoperation = true;
					original_expression = "";
					CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
				} else {
					original_expression = str2;
					CALCULATOR->RPNStackEnter(str2, 0, settings->evalops, parsed_mstruct, NULL);
				}
			}
		}
	} else {
		original_expression = CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options);
		CALCULATOR->calculate(mstruct, original_expression, 0, settings->evalops, parsed_mstruct, &to_struct);
	}

	calculation_wait:

	int i = 0;
	while(CALCULATOR->busy() && i < 75) {
		sleep_ms(10);
		i++;
	}
	i = 0;
	QProgressDialog *dialog = NULL;
	if(CALCULATOR->busy()) {
		dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
	}
	while(CALCULATOR->busy()) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(dialog) {
		dialog->hide();
		dialog->deleteLater();
	}
	bool units_changed = false;

	if(!do_mathoperation && !str_conv.empty() && to_struct.containsType(STRUCT_UNIT, true) && !mstruct->containsType(STRUCT_UNIT) && !parsed_mstruct->containsType(STRUCT_UNIT, false, true, true) && !CALCULATOR->hasToExpression(str_conv, false, settings->evalops)) {
		to_struct.unformat();
		to_struct = CALCULATOR->convertToOptimalUnit(to_struct, settings->evalops, true);
		fix_to_struct(to_struct);
		if(!to_struct.isZero()) {
			mstruct->multiply(to_struct);
			PrintOptions po = settings->printops;
			po.negative_exponents = false;
			to_struct.format(po);
			if(to_struct.isMultiplication() && to_struct.size() >= 2) {
				if(to_struct[0].isOne()) to_struct.delChild(1, true);
				else if(to_struct[1].isOne()) to_struct.delChild(2, true);
			}
			parsed_mstruct->multiply(to_struct);
			to_struct.clear();
			CALCULATOR->calculate(mstruct, 0, settings->evalops, CALCULATOR->unlocalizeExpression(str_conv, settings->evalops.parse_options));
			units_changed = true;
			goto calculation_wait;
		}
	}

	// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
	if(!settings->rpn_mode && !had_to_expression && ((settings->evalops.approximation == APPROXIMATION_EXACT && settings->evalops.auto_post_conversion != POST_CONVERSION_NONE) || settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL) && parsed_mstruct && mstruct && ((parsed_mstruct->isMultiplication() && parsed_mstruct->size() == 2 && (*parsed_mstruct)[0].isNumber() && (*parsed_mstruct)[1].isUnit_exp() && parsed_mstruct->equals(*mstruct)) || (parsed_mstruct->isNegate() && (*parsed_mstruct)[0].isMultiplication() && (*parsed_mstruct)[0].size() == 2 && (*parsed_mstruct)[0][0].isNumber() && (*parsed_mstruct)[0][1].isUnit_exp() && mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1] == (*parsed_mstruct)[0][1] && (*mstruct)[0].isNumber() && (*parsed_mstruct)[0][0].number() == -(*mstruct)[0].number()) || (parsed_mstruct->isUnit_exp() && parsed_mstruct->equals(*mstruct)))) {
		Unit *u = NULL;
		MathStructure *munit = NULL;
		if(mstruct->isMultiplication()) munit = &(*mstruct)[1];
		else munit = mstruct;
		if(munit->isUnit()) u = munit->unit();
		else u = (*munit)[0].unit();
		if(u && u->isCurrency()) {
			if(settings->evalops.local_currency_conversion && CALCULATOR->getLocalCurrency() && u != CALCULATOR->getLocalCurrency()) {
				ApproximationMode abak = settings->evalops.approximation;
				if(settings->evalops.approximation == APPROXIMATION_EXACT) settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = abak;
			}
		} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
			MathStructure mbak(*mstruct);
			if(settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL) {
				if(munit->isUnit() && u->referenceName() == "oF") {
					u = CALCULATOR->getActiveUnit("oC");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, settings->evalops, true, false));
				} else {
					mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				}
			}
			if(settings->evalops.approximation == APPROXIMATION_EXACT && (settings->evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL || mstruct->equals(mbak))) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				if(settings->evalops.auto_post_conversion == POST_CONVERSION_BASE) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, settings->evalops));
				else mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = APPROXIMATION_EXACT;
			}
		}
	}

	if(settings->rpn_mode && (!do_stack || stack_index == 0)) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
		settings->current_result = mstruct;
	}

	if(!do_mathoperation && (askTC(*parsed_mstruct) || (check_exrates && checkExchangeRates()))) {
		b_busy--;
		calculateExpression(do_mathoperation, op, f, settings->rpn_mode, do_stack ? stack_index : 0, false);
		settings->evalops.auto_post_conversion = save_auto_post_conversion;
		settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
		settings->evalops.parse_options.units_enabled = b_units_saved;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_cbase);
		settings->evalops.complex_number_form = save_complex_number_form;
		settings->complex_angle_form = caf_bak;
		settings->printops.custom_time_zone = 0;
		settings->printops.time_zone = TIME_ZONE_LOCAL;
		settings->printops.binary_bits = 0;
		settings->printops.base = save_base;
		settings->printops.use_unit_prefixes = save_pre;
		settings->printops.use_prefixes_for_currencies = save_cur;
		settings->printops.use_prefixes_for_all_units = save_allu;
		settings->printops.use_all_prefixes = save_all;
		settings->printops.use_denominator_prefix = save_den;
		settings->printops.restrict_fraction_length = save_rfl;
		settings->printops.number_fraction_format = save_format;
		CALCULATOR->useBinaryPrefixes(save_bin);
		return;
	}

	mstruct_exact.setUndefined();
	
	if((!do_calendars || !mstruct->isDateTime()) && (settings->dual_approximation > 0 || settings->printops.base == BASE_DECIMAL) && !do_bases && !units_changed) {
		long int i_timeleft = 0;
		i_timeleft = mstruct->containsType(STRUCT_COMPARISON) ? 2000 : 1000;
		if(i_timeleft > 0) {
			calculate_dual_exact(mstruct_exact, mstruct, original_expression, parsed_mstruct, settings->evalops, settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_approximation > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), i_timeleft, 5);
		}
	}

	b_busy--;

	if(do_factors || do_expand || do_pfe) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			mstruct = save_mstruct;
		} else {
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
		}
	}

	//update "ans" variables
	if(!do_stack || stack_index == 0) {
		MathStructure m4(settings->vans[3]->get());
		m4.replace(settings->vans[4], settings->vans[4]->get());
		settings->vans[4]->set(m4);
		MathStructure m3(settings->vans[2]->get());
		m3.replace(settings->vans[3], settings->vans[4]);
		settings->vans[3]->set(m3);
		MathStructure m2(settings->vans[1]->get());
		m2.replace(settings->vans[2], settings->vans[3]);
		settings->vans[2]->set(m2);
		MathStructure m1(settings->vans[0]->get());
		m1.replace(settings->vans[1], settings->vans[2]);
		settings->vans[1]->set(m1);
		mstruct->replace(settings->vans[0], settings->vans[1]);
		settings->vans[0]->set(*mstruct);
	}

	if(do_stack && stack_index > 0) {
	} else if(settings->rpn_mode && do_mathoperation) {
		result_text = tr("RPN Operation").toStdString();
	} else {
		result_text = str;
	}
	settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
	if(settings->rpn_mode && (!do_stack || stack_index == 0)) {
		if(CALCULATOR->RPNStackSize() < stack_size) {
			//RPNRegisterRemoved(1);
		} else if(CALCULATOR->RPNStackSize() > stack_size) {
			//RPNRegisterAdded("");
		}
	}

	if(do_binary_prefixes) {
		int i = 0;
		if(!do_stack || stack_index == 0) i = has_information_unit(*mstruct);
		CALCULATOR->useBinaryPrefixes(i > 0 ? 1 : 2);
		settings->printops.use_unit_prefixes = true;
		if(i == 1) {
			settings->printops.use_denominator_prefix = false;
		} else if(i > 1) {
			settings->printops.use_denominator_prefix = true;
		} else {
			settings->printops.use_prefixes_for_currencies = true;
			settings->printops.use_prefixes_for_all_units = true;
		}
	}
	setResult(NULL, (!do_stack || stack_index == 0), do_stack ? stack_index : 0);
	
	settings->evalops.auto_post_conversion = save_auto_post_conversion;
	settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
	settings->evalops.parse_options.units_enabled = b_units_saved;
	if(custom_base_set) CALCULATOR->setCustomOutputBase(save_cbase);
	settings->evalops.complex_number_form = save_complex_number_form;
	settings->complex_angle_form = caf_bak;
	settings->printops.custom_time_zone = 0;
	settings->printops.time_zone = TIME_ZONE_LOCAL;
	settings->printops.binary_bits = 0;
	settings->printops.base = save_base;
	settings->printops.use_unit_prefixes = save_pre;
	settings->printops.use_prefixes_for_currencies = save_cur;
	settings->printops.use_prefixes_for_all_units = save_allu;
	settings->printops.use_all_prefixes = save_all;
	settings->printops.use_denominator_prefix = save_den;
	settings->printops.restrict_fraction_length = save_rfl;
	settings->printops.number_fraction_format = save_format;
	CALCULATOR->useBinaryPrefixes(save_bin);

}*/

void CommandThread::run() {

	enableAsynchronousCancel();

	while(true) {
		int command_type = 0;
		if(!read(&command_type)) break;
		void *x = NULL;
		if(!read(&x) || !x) break;
		void *x2 = NULL;
		if(!read(&x2)) break;
		CALCULATOR->startControl();
		EvaluationOptions eo2 = settings->evalops;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->structure(STRUCTURING_FACTORIZE, eo2, true);
				}
				if(x2 && !((MathStructure*) x2)->integerFactorize()) {
					eo2.approximation = APPROXIMATION_EXACT;
					((MathStructure*) x2)->structure(STRUCTURING_FACTORIZE, eo2, true);
				}
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				((MathStructure*) x)->expandPartialFractions(eo2);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->expandPartialFractions(eo2);
				break;
			}
			case COMMAND_EXPAND: {
				((MathStructure*) x)->expand(eo2);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->expand(eo2);
				break;
			}
			case COMMAND_TRANSFORM: {
				std::string ceu_str;
				/*if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion"))) && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
					ParseOptions pa = eo2.parse_options; pa.base = 10;
					ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), pa);
					remove_blank_ends(ceu_str);
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {
						if(ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
							ceu_str = "?" + ceu_str;
						}
					}
					if(!ceu_str.empty() && ceu_str[0] == '?') {
						to_prefix = 1;
					} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
						to_prefix = ceu_str[0];
					}
				}*/
				((MathStructure*) x)->set(CALCULATOR->calculate(*((MathStructure*) x), eo2, ceu_str));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->calculate(*((MathStructure*) x2), eo2, ceu_str));
				break;
			}
			case COMMAND_CONVERT_STRING: {
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_units_string, eo2));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convert(*((MathStructure*) x2), command_convert_units_string, eo2));
				break;
			}
			case COMMAND_CONVERT_UNIT: {
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_unit, eo2, false));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convert(*((MathStructure*) x2), command_convert_unit, eo2, false));
				break;
			}
			case COMMAND_CONVERT_OPTIMAL: {
				((MathStructure*) x)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x), eo2, true));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x2), eo2, true));
				break;
			}
			case COMMAND_CONVERT_BASE: {
				((MathStructure*) x)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x), eo2));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x2), eo2));
				break;
			}
			case COMMAND_CALCULATE: {
				eo2.calculate_functions = false;
				eo2.sync_units = false;
				((MathStructure*) x)->calculatesub(eo2, eo2, true);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->calculatesub(eo2, eo2, true);
				break;
			}
			case COMMAND_EVAL: {
			
				((MathStructure*) x)->eval(eo2);
				if(x2) ((MathStructure*) x2)->eval(eo2);
				break;
			}
		}
		b_busy--;
		CALCULATOR->stopControl();

	}
}

void QalculateWindow::executeCommand(int command_type, bool show_result, std::string ceu_str, Unit *u, int run) {

	if(run == 1) {
	
		if(expressionEdit->expressionHasChanged() && !settings->rpn_mode && command_type != COMMAND_TRANSFORM) {
			calculateExpression();
		}

		if(b_busy) return;

		/*if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING) {
			if(mbak_convert.isUndefined()) mbak_convert.set(*mstruct);
			else mstruct->set(mbak_convert);
		} else {
			if(!mbak_convert.isUndefined()) mbak_convert.setUndefined();
		}*/

		b_busy++;
		command_aborted = false;

		if(command_type >= COMMAND_CONVERT_UNIT) {
			CALCULATOR->resetExchangeRatesUsed();
			command_convert_units_string = ceu_str;
			command_convert_unit = u;
		}
		if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING || command_type == COMMAND_CONVERT_BASE || command_type == COMMAND_CONVERT_OPTIMAL) {
			to_prefix = 0;
		}
	}

	bool title_set = false, was_busy = false;
	QProgressDialog *dialog = NULL;

	int i = 0;

	MathStructure *mfactor = new MathStructure(*mstruct);
	MathStructure *mfactor2 = NULL;
	if(!mstruct_exact.isUndefined()) mfactor2 = new MathStructure(mstruct_exact);

	rerun_command:

	if((!command_thread->running && !command_thread->start()) || !command_thread->write(command_type) || !command_thread->write((void *) mfactor) || !command_thread->write((void *) mfactor2)) {
		command_thread->cancel();
		mfactor->unref();
		if(mfactor2) mfactor2->unref();
		b_busy--;
		return;
	}

	while(b_busy && command_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(!was_busy && b_busy && command_thread->running) {
		QString progress_str;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				progress_str = tr("Factorizing…");
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				progress_str = tr("Expanding partial fractions…");
				break;
			}
			case COMMAND_EXPAND: {
				progress_str = tr("Expanding…");
				break;
			}
			case COMMAND_EVAL: {}
			case COMMAND_TRANSFORM: {
				progress_str = tr("Calculating…");
				break;
			}
			default: {
				progress_str = tr("Converting…");
				break;
			}
		}
		if(updateWindowTitle(progress_str)) title_set = true;
		dialog = new QProgressDialog(progress_str, tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abortCommand()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(b_busy && command_thread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(!command_thread->running) command_aborted = true;

	if(!command_aborted && run == 1 && command_type >= COMMAND_CONVERT_UNIT && checkExchangeRates()) {
		b_busy++;
		mfactor->set(*mstruct);
		run = 2;
		goto rerun_command;
	}

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	if(command_type == COMMAND_CONVERT_STRING && !ceu_str.empty()) {
		if(ceu_str[0] == '?') {
			to_prefix = 1;
		} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
			to_prefix = ceu_str[0];
		}
	}

	if(!command_aborted) {
		if(mfactor2) {mstruct_exact.set(*mfactor2); mfactor2->unref();}
		mstruct->set(*mfactor);
		mfactor->unref();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				settings->printops.allow_factorization = true;
				break;
			}
			case COMMAND_EXPAND: {
				settings->printops.allow_factorization = false;
				break;
			}
			default: {
				settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
			}
		}
		if(show_result) {
			setResult(NULL, true, false, true, command_type == COMMAND_TRANSFORM ? ceu_str : "");
		}
	}

}

void QalculateWindow::updateResultBases() {
}
void QalculateWindow::onExpressionChanged() {
}

void set_result_bases(const MathStructure &m) {
	result_bin = ""; result_oct = "", result_dec = "", result_hex = "";
	if(max_bases.isZero()) {max_bases = 2; max_bases ^= 64; min_bases = -max_bases;}
	if(!CALCULATOR->aborted() && ((m.isNumber() && m.number() < max_bases && m.number() > min_bases) || (m.isNegate() && m[0].isNumber() && m[0].number() < max_bases && m[0].number() > min_bases))) {
		Number nr;
		if(m.isNumber()) {
			nr = m.number();
		} else {
			nr = m[0].number();
			nr.negate();
		}
		nr.round(settings->printops.round_halfway_to_even);
		PrintOptions po = settings->printops;
		po.show_ending_zeroes = false;
		po.base_display = BASE_DISPLAY_NORMAL;
		po.min_exp = 0;
		if(settings->printops.base != 2) {
			po.base = 2;
			result_bin = nr.print(po);
		}
		if(settings->printops.base != 8) {
			po.base = 8;
			result_oct = nr.print(po);
			size_t i = result_oct.find_first_of(NUMBERS);
			if(i != std::string::npos && result_oct.length() > i + 1 && result_oct[i] == '0' && is_in(NUMBERS, result_oct[i + 1])) result_oct.erase(i, 1);
		}
		if(settings->printops.base != 10) {
			po.base = 10;
			result_dec = nr.print(po);
		}
		if(settings->printops.base != 16) {
			po.base = 16;
			result_hex = nr.print(po);
			gsub("0x", "", result_hex);
			size_t l = result_hex.length();
			size_t i_after_minus = 0;
			if(nr.isNegative()) {
				if(l > 1 && result_hex[0] == '-') i_after_minus = 1;
				else if(result_hex.find("−") == 0) i_after_minus = strlen("−");
			}
			for(int i = (int) l - 2; i > (int) i_after_minus; i -= 2) {
				result_hex.insert(i, 1, ' ');
			}
			if(result_hex.length() > i_after_minus + 1 && result_hex[i_after_minus + 1] == ' ') result_hex.insert(i_after_minus, 1, '0');
		}
	}
}

void ViewThread::run() {

	while(true) {

		void *x = NULL;
		if(!read(&x) || !x) break;
		MathStructure *mresult = (MathStructure*) x;
		x = NULL;
		bool b_stack = false;
		if(!read(&b_stack)) break;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		MathStructure *mparse = (MathStructure*) x;

		PrintOptions po;
		if(mparse) {
			if(!read(&po.is_approximate)) break;
			if(!read<bool>(&po.preserve_format)) break;
			po.show_ending_zeroes = settings->evalops.parse_options.read_precision != DONT_READ_PRECISION && CALCULATOR->usesIntervalArithmetic() && settings->evalops.parse_options.base > BASE_CUSTOM;
			po.lower_case_e = settings->printops.lower_case_e;
			po.lower_case_numbers = settings->printops.lower_case_numbers;
			po.base_display = settings->printops.base_display;
			po.twos_complement = settings->printops.twos_complement;
			po.hexadecimal_twos_complement = settings->printops.hexadecimal_twos_complement;
			po.base = settings->evalops.parse_options.base;
			po.allow_non_usable = true;
			Number nr_base;
			if(po.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
				nr_base = CALCULATOR->customOutputBase();
				CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
			} else if(po.base == BASE_CUSTOM || (po.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po.base != BASE_UNICODE)) {
				po.base = 10;
				po.min_exp = 6;
				po.use_max_decimals = true;
				po.max_decimals = 5;
				po.preserve_format = false;
			}
			po.abbreviate_names = false;
			po.digit_grouping = settings->printops.digit_grouping;
			po.use_unicode_signs = settings->printops.use_unicode_signs;
			po.multiplication_sign = settings->printops.multiplication_sign;
			po.division_sign = settings->printops.division_sign;
			po.short_multiplication = false;
			po.excessive_parenthesis = true;
			po.improve_division_multipliers = false;
			po.restrict_to_parent_precision = false;
			po.spell_out_logical_operators = settings->printops.spell_out_logical_operators;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			MathStructure mp(*mparse);
			mp.format(po);
			parsed_text = mp.print(po, true, settings->color, TAG_TYPE_HTML);
			if(po.base == BASE_CUSTOM) {
				CALCULATOR->setCustomOutputBase(nr_base);
			}
		}

		po = settings->printops;

		po.allow_non_usable = true;

		print_dual(*mresult, original_expression, mparse ? *mparse : *parsed_mstruct, mstruct_exact, result_text, alt_results, po, settings->evalops, settings->dual_fraction < 0 ? AUTOMATIC_FRACTION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_FRACTION_DUAL : AUTOMATIC_FRACTION_OFF), settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), settings->complex_angle_form, &exact_comparison, mparse != NULL, true, settings->color, TAG_TYPE_HTML);

		if(!b_stack) {
			set_result_bases(*mresult);
		}

		b_busy--;
		CALCULATOR->stopControl();

	}
}

bool contains_unknown_variable(const MathStructure &m) {
	if(m.isVariable()) return !m.variable()->isKnown();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_unknown_variable(m[i])) return true;
	}
	return false;
}

void QalculateWindow::setResult(Prefix *prefix, bool update_history, bool update_parse, bool force, std::string transformation, size_t stack_index, bool register_moved, bool supress_dialog) {

	if(block_result_update) return;

	if(expressionEdit->expressionHasChanged() && (!settings->rpn_mode || CALCULATOR->RPNStackSize() == 0)) {
		if(!force) return;
		calculateExpression();
		if(!prefix) return;
	}

	if(settings->rpn_mode && CALCULATOR->RPNStackSize() == 0) return;

	if(nr_of_new_expressions == 0 && !register_moved && !update_parse && update_history) {
		update_history = false;
	}

	if(b_busy) return;

	std::string prev_result_text = result_text;
	bool prev_approximate = *settings->printops.is_approximate;
	result_text = "?";

	if(update_parse) {
		parsed_text = tr("aborted").toStdString();
	}

	if(!settings->rpn_mode) stack_index = 0;
	if(stack_index != 0) {
		update_history = true;
		update_parse = false;
		
	}
	if(register_moved) {
		update_history = true;
		update_parse = false;
	}

	if(update_parse && parsed_mstruct && parsed_mstruct->isFunction() && (parsed_mstruct->function() == CALCULATOR->f_error || parsed_mstruct->function() == CALCULATOR->f_warning || parsed_mstruct->function() == CALCULATOR->f_message)) {
		expressionEdit->clear();
		historyView->addMessages();
		return;
	}

	b_busy++;

	if(!view_thread->running && !view_thread->start()) {b_busy--; return;}

	bool b_rpn_operation = false;

	if(update_history) {
		if(update_parse || register_moved) {
			if(register_moved) {
				result_text = tr("RPN Register Moved").toStdString();
			} else {
				remove_blank_ends(result_text);
				gsub("\n", " ", result_text);
				if(result_text == tr("RPN Operation").toStdString()) {
					b_rpn_operation = true;
				} else {
					if(settings->adaptive_interval_display) {
						QString expression_str = expressionEdit->toPlainText();
						if((parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_uncertainty)) || expression_str.contains("+/-") || expression_str.contains("+/" SIGN_MINUS) || expression_str.contains("±")) settings->printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
						else if(parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_interval)) settings->printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
						else settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
					}
				}
			}
			nr_of_new_expressions++;
		} else if(!transformation.empty()) {
		}
		result_text = "?";
	}

	if(update_parse) {
		parsed_text = "aborted";
	}

	bool parsed_approx = false;
	bool title_set = false, was_busy = false;

	Number save_nbase;
	bool custom_base_set = false;
	int save_base = settings->printops.base;
	bool caf_bak = settings->complex_angle_form;
	unsigned int save_bits = settings->printops.binary_bits;
	bool save_pre = settings->printops.use_unit_prefixes;
	bool save_cur = settings->printops.use_prefixes_for_currencies;
	bool save_allu = settings->printops.use_prefixes_for_all_units;
	bool save_all = settings->printops.use_all_prefixes;
	bool save_den = settings->printops.use_denominator_prefix;
	int save_bin = CALCULATOR->usesBinaryPrefixes();
	NumberFractionFormat save_format = settings->printops.number_fraction_format;
	bool save_restrict_fraction_length = settings->printops.restrict_fraction_length;
	bool do_to = false;

	if(stack_index == 0) {
		if(to_base != 0 || to_fraction || to_prefix != 0 || (to_caf >= 0 && to_caf != settings->complex_angle_form)) {
			if(to_base != 0 && (to_base != settings->printops.base || to_bits != settings->printops.binary_bits || (to_base == BASE_CUSTOM && to_nbase != CALCULATOR->customOutputBase()))) {
				settings->printops.base = to_base;
				settings->printops.binary_bits = to_bits;
				if(to_base == BASE_CUSTOM) {
					custom_base_set = true;
					save_nbase = CALCULATOR->customOutputBase();
					CALCULATOR->setCustomOutputBase(to_nbase);
				}
				do_to = true;
			}
			if(to_fraction && (settings->printops.restrict_fraction_length || settings->printops.number_fraction_format != FRACTION_COMBINED)) {
				settings->printops.restrict_fraction_length = false;
				settings->printops.number_fraction_format = FRACTION_COMBINED;
				do_to = true;
			}
			if(to_caf >= 0 && to_caf != settings->complex_angle_form) {
				settings->complex_angle_form = to_caf;
				do_to = true;
			}
			if(to_prefix != 0 && !prefix) {
				bool new_pre = settings->printops.use_unit_prefixes;
				bool new_cur = settings->printops.use_prefixes_for_currencies;
				bool new_allu = settings->printops.use_prefixes_for_all_units;
				bool new_all = settings->printops.use_all_prefixes;
				bool new_den = settings->printops.use_denominator_prefix;
				int new_bin = CALCULATOR->usesBinaryPrefixes();
				new_pre = true;
				if(to_prefix == 'b') {
					int i = has_information_unit(*mstruct);
					new_bin = (i > 0 ? 1 : 2);
					if(i == 1) {
						new_den = false;
					} else if(i > 1) {
						new_den = true;
					} else {
						new_cur = true;
						new_allu = true;
					}
				} else {
					new_cur = true;
					new_allu = true;
					if(to_prefix == 'a') new_all = true;
					else if(to_prefix == 'd') new_bin = 0;
				}
				if(settings->printops.use_unit_prefixes != new_pre || settings->printops.use_prefixes_for_currencies != new_cur || settings->printops.use_prefixes_for_all_units != new_allu || settings->printops.use_all_prefixes != new_all || settings->printops.use_denominator_prefix != new_den || CALCULATOR->usesBinaryPrefixes() != new_bin) {
					settings->printops.use_unit_prefixes = new_pre;
					settings->printops.use_all_prefixes = new_all;
					settings->printops.use_prefixes_for_currencies = new_cur;
					settings->printops.use_prefixes_for_all_units = new_allu;
					settings->printops.use_denominator_prefix = new_den;
					CALCULATOR->useBinaryPrefixes(new_bin);
					do_to = true;
				}
			}
		}
	}

	settings->printops.prefix = prefix;

	if(stack_index == 0) {
		if(!view_thread->write((void*) mstruct)) {b_busy--; view_thread->cancel(); return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void*) mreg)) {b_busy--; view_thread->cancel(); return;}
	}
	bool b_stack = stack_index != 0;
	if(!view_thread->write(b_stack)) {b_busy--; view_thread->cancel(); return;}
	if(update_parse) {
		if(!view_thread->write((void *) parsed_mstruct)) {b_busy--; view_thread->cancel(); return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write(parsed_approx_p)) {b_busy--; view_thread->cancel(); return;}
		if(!view_thread->write(b_rpn_operation)) {b_busy--; view_thread->cancel(); return;}
	} else {
		if(settings->printops.base != BASE_DECIMAL && settings->dual_approximation <= 0) mstruct_exact.setUndefined();
		if(!view_thread->write((void *) NULL)) {b_busy--; view_thread->cancel(); return;}
	}

	QProgressDialog *dialog = NULL;

	int i = 0;
	while(b_busy && view_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(b_busy && view_thread->running) {
		if(updateWindowTitle(tr("Processing…"))) title_set = true;
		dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(b_busy && view_thread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	b_busy++;

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set && stack_index != 0) updateWindowTitle();
	}

	if(stack_index == 0) {
		if(basesDock->isVisible()) updateResultBases();
		if(!updateWindowTitle(QString::fromStdString(result_text), true) && title_set) updateWindowTitle();
	}
	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}
	/*if(current_inhistory_index < 0) {
		update_parse = true;
		current_inhistory_index = 0;
	}*/
	if(stack_index != 0) {
		//RPNRegisterChanged(result_text, stack_index);
		if(supress_dialog) CALCULATOR->clearMessages();
		else displayMessages(this);
	} else if(update_history) {
		if(update_parse) {}
	} else {
		if(supress_dialog) CALCULATOR->clearMessages();
		else displayMessages(this);
	}

	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}

	if(stack_index != 0) {
		//RPNRegisterChanged(result_text, stack_index);
	} else {
		if(mstruct->isAborted() || settings->printops.base != settings->evalops.parse_options.base || (settings->printops.base > 32 || settings->printops.base < 2)) {
			exact_text = "";
		} else if(!mstruct->isApproximate() && !(*settings->printops.is_approximate)) {
			exact_text = unhtmlize(result_text, !contains_unknown_variable(*mstruct));
		} else if(!alt_results.empty()) {
			exact_text = unhtmlize(alt_results[0], !contains_unknown_variable(*mstruct));
		} else {
			exact_text = "";
		}
		alt_results.push_back(result_text);
		historyView->addResult(alt_results, update_parse ? parsed_text : "", (update_parse || !prev_approximate) && (exact_comparison || (!(*settings->printops.is_approximate) && !mstruct->isApproximate())));
	}

	if(do_to) {
		settings->complex_angle_form = caf_bak;
		settings->printops.base = save_base;
		settings->printops.binary_bits = save_bits;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_nbase);
		settings->printops.use_unit_prefixes = save_pre;
		settings->printops.use_all_prefixes = save_all;
		settings->printops.use_prefixes_for_currencies = save_cur;
		settings->printops.use_prefixes_for_all_units = save_allu;
		settings->printops.use_denominator_prefix = save_den;
		CALCULATOR->useBinaryPrefixes(save_bin);
		settings->printops.number_fraction_format = save_format;
		settings->printops.restrict_fraction_length = save_restrict_fraction_length;
	}
	settings->printops.prefix = NULL;

	b_busy--;
}

void QalculateWindow::changeEvent(QEvent *e) {
	if(e->type() == QEvent::StyleChange) {
		QColor c = historyView->palette().base().color();
		if(c.red() + c.green() + c.blue() < 255) settings->color = 2;
		else settings->color = 1;
	}
	QMainWindow::changeEvent(e);
}

void FetchExchangeRatesThread::run() {
	int timeout = 15;
	int n = -1;
	if(!read(&timeout)) return;
	if(!read(&n)) return;
	CALCULATOR->fetchExchangeRates(timeout, n);
}
bool QalculateWindow::checkExchangeRates() {
	int i = CALCULATOR->exchangeRatesUsed();
	if(i == 0) return false;
	if(settings->auto_update_exchange_rates == 0) return false;
	if(CALCULATOR->checkExchangeRatesDate(settings->auto_update_exchange_rates > 0 ? settings->auto_update_exchange_rates : 7, false, settings->auto_update_exchange_rates == 0, i)) return false;
	if(settings->auto_update_exchange_rates == 0) return false;
	bool b = false;
	if(settings->auto_update_exchange_rates < 0) {
		int days = (int) floor(difftime(time(NULL), CALCULATOR->getExchangeRatesTime(i)) / 86400);
		if(QMessageBox::question(this, tr("Update exchange rates?"), tr("It has been %n day(s) since the exchange rates last were updated.\n\nDo you wish to update the exchange rates now?", "", days), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
			b = true;
		}
	}
	if(b || settings->auto_update_exchange_rates > 0) {
		if(settings->auto_update_exchange_rates <= 0) i = -1;
		fetchExchangeRates(b ? 15 : 8, i);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}
void QalculateWindow::fetchExchangeRates() {
	CALCULATOR->clearMessages();
	fetchExchangeRates(15);
	CALCULATOR->loadExchangeRates();
	displayMessages(this);
	expressionCalculationUpdated();
}
void QalculateWindow::fetchExchangeRates(int timeout, int n) {
	b_busy++;
	FetchExchangeRatesThread fetch_thread;
	if(fetch_thread.start() && fetch_thread.write(timeout) && fetch_thread.write(n)) {
		if(fetch_thread.running) {
			QProgressDialog *dialog = new QProgressDialog(tr("Fetching exchange rates."), QString(), 0, 0, this);
			dialog->setWindowModality(Qt::WindowModal);
			dialog->setMinimumDuration(200);
			while(fetch_thread.running) {
				qApp->processEvents();
				sleep_ms(10);
			}
			dialog->deleteLater();
		}
	}
	b_busy--;
}
void QalculateWindow::abort() {
	CALCULATOR->abort();
}
void QalculateWindow::abortCommand() {
	CALCULATOR->abort();
	int msecs = 5000;
	while(b_busy && msecs > 0) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(b_busy) {
		command_thread->cancel();
		b_busy--;
		CALCULATOR->stopControl();
		command_aborted = true;
	}
}
bool contains_temperature_unit_qt(const MathStructure &m) {
	if(m.isUnit()) {
		return m.unit() == CALCULATOR->getUnitById(UNIT_ID_CELSIUS) || m.unit() == CALCULATOR->getUnitById(UNIT_ID_FAHRENHEIT);
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		return contains_temperature_unit_qt(((KnownVariable*) m.variable())->get());
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_temperature_unit_qt(m[i])) return true;
	}
	return false;
}
bool QalculateWindow::askTC(MathStructure &m) {
	if(settings->tc_set || !contains_temperature_unit_qt(m)) return false;
	MathStructure *mp = &m;
	if(m.isMultiplication() && m.size() == 2 && m[0].isMinusOne()) mp = &m[1];
	else if(m.isNegate()) mp = &m[0];
	if(mp->isUnit_exp()) return false;
	if(mp->isMultiplication() && mp->size() > 0 && mp->last().isUnit_exp()) {
		bool b = false;
		for(size_t i = 0; i < mp->size() - 1; i++) {
			if(contains_temperature_unit_qt((*mp)[i])) {b = true; break;}
		}
		if(!b) return false;
	}
	/*GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Temperature Calculation Mode"), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("The expression is ambiguous.\nPlease select temperature calculation mode\n(the mode can later be changed in preferences)."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_abs = gtk_radio_button_new_with_label(NULL, _("Absolute"));
	gtk_widget_set_valign(w_abs, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_abs, 0, 1, 1, 1);
	label = gtk_label_new("<i>1 °C + 1 °C ≈ 274 K + 274 K ≈ 548 K\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 K\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_rel = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_abs), _("Relative"));
	gtk_widget_set_valign(w_rel, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_rel, 0, 2, 1, 1);
	label = gtk_label_new("<i>1 °C + 1 °C = 2 °C\n1 °C + 5 °F = 1 °C + 5 °R ≈ 4 °C ≈ 277 K\n2 °C − 1 °C = 1 °C\n1 °C − 5 °F = 1 °C - 5 °R ≈ −2 °C\n1 °C + 1 K = 2 °C</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	GtkWidget *w_hybrid = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_abs), _("Hybrid"));
	gtk_widget_set_valign(w_hybrid, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_hybrid, 0, 3, 1, 1);
	label = gtk_label_new("<i>1 °C + 1 °C ≈ 2 °C\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 °C\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
	switch(CALCULATOR->getTemperatureCalculationMode()) {
		case TEMPERATURE_CALCULATION_ABSOLUTE: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_abs), TRUE); break;}
		case TEMPERATURE_CALCULATION_RELATIVE: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_rel), TRUE); break;}
		default: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_hybrid), TRUE); break;}
	}
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	TemperatureCalculationMode tc_mode = TEMPERATURE_CALCULATION_HYBRID;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_abs))) tc_mode = TEMPERATURE_CALCULATION_ABSOLUTE;
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_rel))) tc_mode = TEMPERATURE_CALCULATION_RELATIVE;
	gtk_widget_destroy(dialog);
	if(preferences_builder) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_abs_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_rel_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_hybrid_toggled, NULL);
		switch(tc_mode) {
			case TEMPERATURE_CALCULATION_ABSOLUTE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs")), TRUE);
				break;
			}
			case TEMPERATURE_CALCULATION_RELATIVE: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel")), TRUE);
				break;
			}
			default: {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid")), TRUE);
				break;
			}
		}
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_abs"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_abs_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_rel"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_rel_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_radiobutton_temp_hybrid"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_radiobutton_temp_hybrid_toggled, NULL);
	}
	tc_set = true;
	if(tc_mode != CALCULATOR->getTemperatureCalculationMode()) {
		CALCULATOR->setTemperatureCalculationMode(tc_mode);
		return true;
	}*/
	return false;
}

bool QalculateWindow::askDot(const std::string &str) {
	if(settings->dot_question_asked || CALCULATOR->getDecimalPoint() == DOT) return false;
	size_t i = 0;
	bool b = false;
	while(true) {
		i = str.find(DOT, i);
		if(i == std::string::npos) return false;
		i = str.find_first_not_of(SPACES, i + 1);
		if(i == std::string::npos) return false;
		if(is_in(NUMBERS, str[i])) {
			b = true;
			break;
		}
	}
	if(!b) return false;
	/*GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Interpretation of dots"), GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 6);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);
	gtk_widget_show(grid);
	GtkWidget *label = gtk_label_new(_("Please select interpretation of dots (\".\")\n(this can later be changed in preferences)."));
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 2, 1);
	GtkWidget *w_bothdeci = gtk_radio_button_new_with_label(NULL, _("Both dot and comma as decimal separators"));
	gtk_widget_set_valign(w_bothdeci, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_bothdeci, 0, 1, 1, 1);
	label = gtk_label_new("<i>(1.2 = 1,2)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	GtkWidget *w_ignoredot = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_bothdeci), _("Dot as thousands separator"));
	gtk_widget_set_valign(w_ignoredot, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_ignoredot, 0, 2, 1, 1);
	label = gtk_label_new("<i>(1.000.000 = 1000000)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	GtkWidget *w_dotdeci = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w_bothdeci), _("Only dot as decimal separator"));
	gtk_widget_set_valign(w_dotdeci, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), w_dotdeci, 0, 3, 1, 1);
	label = gtk_label_new("<i>(1.2 + root(16, 4) = 3.2)</i>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 3, 1, 1);
	if(evalops.parse_options.dot_as_separator) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_ignoredot), TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_bothdeci), TRUE);
	gtk_widget_show_all(grid);
	gtk_dialog_run(GTK_DIALOG(dialog));
	dot_question_asked = true;
	bool das = evalops.parse_options.dot_as_separator;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_dotdeci))) {
		evalops.parse_options.dot_as_separator = false;
		evalops.parse_options.comma_as_separator = false;
		b_decimal_comma = false;
		CALCULATOR->useDecimalPoint(false);
		das = !evalops.parse_options.dot_as_separator;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_ignoredot))) {
		evalops.parse_options.dot_as_separator = true;
	} else {
		evalops.parse_options.dot_as_separator = false;
	}
	if(preferences_builder) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_dot_as_separator_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_comma_as_separator_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_decimal_comma_toggled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma")), CALCULATOR->getDecimalPoint() == COMMA);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")), evalops.parse_options.dot_as_separator);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")), evalops.parse_options.comma_as_separator);
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator")), CALCULATOR->getDecimalPoint() != DOT);
		gtk_widget_set_visible(GTK_WIDGET(gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator")), CALCULATOR->getDecimalPoint() != COMMA);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_dot_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_dot_as_separator_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_comma_as_separator"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_comma_as_separator_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(preferences_builder, "preferences_checkbutton_decimal_comma"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_preferences_checkbutton_decimal_comma_toggled, NULL);
	}
	gtk_widget_destroy(dialog);
	return das != evalops.parse_options.dot_as_separator;*/
	return false;
}
void QalculateWindow::keyPressEvent(QKeyEvent *e) {
	if(!send_event) return;
	QMainWindow::keyPressEvent(e);
	if(!e->isAccepted() && (e->key() < Qt::Key_Tab || e->key() > Qt::Key_ScrollLock) && !(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::MetaModifier)) {
		e->accept();
		QKeyEvent *e2 = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->text());
		send_event = false;
		qApp->postEvent((QObject*) expressionEdit, (QEvent*) e2);
		expressionEdit->setFocus();
		qApp->processEvents();
		send_event = true;
	}
}

void QalculateWindow::onToActivated() {
	QTextCursor cur = expressionEdit->textCursor();
	if(!expressionEdit->expressionHasChanged() && cur.hasSelection() && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length()) {
		if(!expressionEdit->complete(mstruct, tb->widgetForAction(toAction)->mapToGlobal(tb->widgetForAction(toAction)->pos()))) {
			expressionEdit->insertPlainText("➞");
		}
	} else {
		expressionEdit->selectAll(false);
		expressionEdit->blockCompletion(true);
		expressionEdit->insertPlainText("➞");
		expressionEdit->complete(NULL, tb->widgetForAction(toAction)->mapToGlobal(tb->widgetForAction(toAction)->pos()));
		expressionEdit->blockCompletion(false);
	}
}
void QalculateWindow::onToConversionRequested(std::string str) {
	str.insert(0, "➞");
	calculateExpression(true, false, OPERATION_ADD, NULL, false, 0, "", str);
}
void QalculateWindow::onStoreActivated() {
}
void QalculateWindow::onKeypadActivated(bool b) {
	keypadDock->setVisible(b);
}
void QalculateWindow::onKeypadVisibilityChanged(bool b) {
	keypadAction->setChecked(b);
}
void QalculateWindow::onBasesActivated(bool b) {
	basesDock->setVisible(b);
}
void QalculateWindow::onBasesVisibilityChanged(bool b) {
	basesAction->setChecked(b);
	if(b) updateResultBases();
}
bool QalculateWindow::displayMessages(QWidget *parent) {
	if(!CALCULATOR->message()) return false;
	std::string str = "";
	int index = 0;
	MessageType mtype, mtype_highest = MESSAGE_INFORMATION;
	while(true) {
		mtype = CALCULATOR->message()->type();
		if(index > 0) {
			if(index == 1) str = "• " + str;
				str += "\n• ";
		}
		str += CALCULATOR->message()->message();
		if(mtype == MESSAGE_ERROR || (mtype_highest != MESSAGE_ERROR && mtype == MESSAGE_WARNING)) {
			mtype_highest = mtype;
		}
		index++;
		if(!CALCULATOR->nextMessage()) break;
	}
	if(!str.empty()) {
		if(mtype_highest == MESSAGE_ERROR) QMessageBox::critical(parent, tr("Error"), QString::fromStdString(str), QMessageBox::Ok);
		else if(mtype_highest == MESSAGE_WARNING) QMessageBox::warning(parent, tr("Warning"), QString::fromStdString(str), QMessageBox::Ok);
		else QMessageBox::information(parent, tr("Information"), QString::fromStdString(str), QMessageBox::Ok);
	}
	return false;
}
bool QalculateWindow::updateWindowTitle(const QString &str, bool is_result) {
	if(title_modified) return false;
	switch(settings->title_type) {
		case TITLE_RESULT: {
			if(str.isEmpty()) return false;
			if(!str.isEmpty()) setWindowTitle(str);
			break;
		}
		case TITLE_APP_RESULT: {
			if(!str.isEmpty()) setWindowTitle("Qalculate! (" + str + ")");
			break;
		}
		default: {
			if(is_result) return false;
			if(!str.isEmpty()) setWindowTitle("Qalculate! " + str);
			else setWindowTitle("Qalculate!");
		}
	}
	return true;
}

