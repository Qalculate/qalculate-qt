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
	COMMAND_EXPAND,
	COMMAND_EXPAND_PARTIAL_FRACTIONS,
	COMMAND_EVAL
};

std::vector<std::string> alt_results;
int b_busy;
bool exact_comparison, command_aborted;
std::string original_expression, result_text, parsed_text;
ViewThread *view_thread;
CommandThread *command_thread;
MathStructure *mstruct, *parsed_mstruct, mstruct_exact;

extern void fix_to_struct(MathStructure &m);
extern void print_dual(const MathStructure &mresult, const std::string &original_expression, const MathStructure &mparse, MathStructure &mexact, std::string &result_str, std::vector<std::string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle = false, bool *exact_cmp = NULL, bool b_parsed = true, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1);
extern void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const std::string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs = 0, int max_size = 10);
extern int has_information_unit(const MathStructure &m, bool top = true);

QalculateWindow::QalculateWindow() : QMainWindow() {

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	send_event = true;

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	ehSplitter = new QSplitter(Qt::Vertical, this);
	topLayout->addWidget(ehSplitter, 1);

	expressionEdit = new ExpressionEdit(this);
	expressionEdit->setFocus();
	ehSplitter->addWidget(expressionEdit);
	historyView = new HistoryView(this);

	ehSplitter->addWidget(historyView);
	ehSplitter->setStretchFactor(0, 0);
	ehSplitter->setStretchFactor(1, 1);

	keypad = new KeypadWidget(this);
	keypadDock = new QDockWidget(tr("Keypad"), this);
	keypadDock->setWidget(keypad);
	addDockWidget(Qt::BottomDockWidgetArea, keypadDock);

	QLocalServer::removeServer("qalculate-qt");
	server = new QLocalServer(this);
	server->listen("qalculate-qt");
	connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));
	
	mstruct = new MathStructure();
	settings->current_result = mstruct;
	mstruct_exact.setUndefined();
	parsed_mstruct = new MathStructure();
	view_thread = new ViewThread;
	command_thread = new CommandThread;

	settings->printops.can_display_unicode_string_arg = (void*) historyView;

	b_busy = 0;

	connect(expressionEdit, SIGNAL(returnPressed()), this, SLOT(calculate()));
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
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(str);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->unblockCompletion();
}
void QalculateWindow::onOperatorClicked(const QString &str) {
	bool do_exec = false;
	QTextCursor cur = expressionEdit->textCursor();
	bool sel = cur.hasSelection();
	if(str == "~" && sel) {
		expressionEdit->wrapSelection(str);
	} else {
		if(sel) {
			do_exec = (str == "!") && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
			expressionEdit->wrapSelection();
		}
		if(str == "E") {
			if(sel) expressionEdit->insertPlainText(SIGN_MULTIPLICATION "10^");
			else expressionEdit->insertPlainText(settings->printops.lower_case_e ? "e" : str);
		} else {
			expressionEdit->insertPlainText(str);
		}
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
	expressionEdit->blockCompletion();
	QTextCursor cur = expressionEdit->textCursor();
	bool do_exec = false;
	if(cur.hasSelection()) {
		do_exec = cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
	} else if(last_is_number(expressionEdit->toPlainText().toStdString())) {
		expressionEdit->selectAll();
		do_exec = true;
	}
	expressionEdit->wrapSelection(f->referenceName() == "neg" ? SIGN_MINUS : QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->unblockCompletion();
	if(do_exec) calculate();
}
void QalculateWindow::onVariableClicked(Variable *v) {
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(QString::fromStdString(v->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->unblockCompletion();
}
void QalculateWindow::onUnitClicked(Unit *u) {
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(QString::fromStdString(u->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->unblockCompletion();
}
void QalculateWindow::onDelClicked() {
	expressionEdit->blockCompletion();
	QTextCursor cur = expressionEdit->textCursor();
	if(cur.atEnd()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->unblockCompletion();
}
void QalculateWindow::onBackspaceClicked() {
	expressionEdit->blockCompletion();
	QTextCursor cur = expressionEdit->textCursor();
	if(!cur.atStart()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->unblockCompletion();
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
	command = command.trimmed();
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
	expressionEdit->hideCompletion();
	calculateExpression();
	expressionEdit->addToHistory();
	//expressionEdit->clear();
	expressionEdit->selectAll();
	expressionEdit->setExpressionHasChanged(false);
}
void QalculateWindow::calculate(const QString &expression) {
	expressionEdit->setPlainText(expression);
	calculate();
}

void QalculateWindow::calculateExpression(bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, bool check_exrates) {

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
						CALCULATOR->error(true, tr("Time zone parsing failed.").toStdString().c_str(), NULL);
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

}

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
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->structure(STRUCTURING_FACTORIZE, settings->evalops, true);
				}
				if(x2 && !((MathStructure*) x2)->integerFactorize()) {
					((MathStructure*) x2)->structure(STRUCTURING_FACTORIZE, settings->evalops, true);
				}
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				((MathStructure*) x)->expandPartialFractions(settings->evalops);
				if(x2) ((MathStructure*) x2)->expandPartialFractions(settings->evalops);
				break;
			}
			case COMMAND_EXPAND: {
				((MathStructure*) x)->expand(settings->evalops);
				if(x2) ((MathStructure*) x2)->expand(settings->evalops);
				break;
			}
			case COMMAND_EVAL: {
				((MathStructure*) x)->eval(settings->evalops);
				if(x2) ((MathStructure*) x2)->eval(settings->evalops);
				break;
			}
		}
		b_busy = false;
		CALCULATOR->stopControl();

	}
}

void QalculateWindow::executeCommand(int command_type, bool show_result) {

	b_busy = true;
	command_aborted = false;

	if(!command_thread->running && !command_thread->start()) {b_busy = false; return;}

	if(!command_thread->write(command_type)) {command_thread->cancel(); b_busy = false; return;}
	MathStructure *mfactor = new MathStructure(*mstruct);
	MathStructure *mfactor2 = NULL;
	if(!mstruct_exact.isUndefined()) mfactor2 = new MathStructure(mstruct_exact);
	if(!command_thread->write((void *) mfactor) || !command_thread->write((void *) mfactor2)) {
		command_thread->cancel();
		mfactor->unref();
		if(mfactor2) mfactor2->unref();
		b_busy = false;
		return;
	}

	int i = 0;
	while(b_busy && command_thread->running && i < 75) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	QProgressDialog *dialog = NULL;
	if(b_busy && command_thread->running) {
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				dialog = new QProgressDialog(tr("Factorizing…"), tr("Cancel"), 0, 0, this);
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				dialog = new QProgressDialog(tr("Expanding partial fractions…"), tr("Cancel"), 0, 0, this);
				break;
			}
			case COMMAND_EXPAND: {
				dialog = new QProgressDialog(tr("Expanding…"), tr("Cancel"), 0, 0, this);
				break;
			}
			case COMMAND_EVAL: {
				dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
				break;
			}
		}
		if(dialog) {
			connect(dialog, SIGNAL(canceled()), this, SLOT(abortCommand()));
			dialog->setWindowModality(Qt::WindowModal);
			dialog->show();
		}
	}
	while(b_busy && command_thread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(dialog) {
		dialog->hide();
		dialog->deleteLater();
	}

	b_busy = false;

	if(!command_aborted) {
		if(mfactor2) mstruct_exact.set(*mfactor2);
		mstruct->unref();
		mstruct = mfactor;
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
		if(show_result) setResult(NULL, false);
	}

}


void ViewThread::run() {

	while(true) {

		void *x = NULL;
		if(!read(&x) || !x) break;
		MathStructure *mresult = (MathStructure*) x;
		x = NULL;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		MathStructure *mparse = (MathStructure*) x;
		PrintOptions po;
		if(mparse) {
			if(!read(&po.is_approximate)) break;
			if(!read<bool>(&po.preserve_format)) break;
			po.show_ending_zeroes = false;
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

		b_busy--;
		CALCULATOR->stopControl();

	}

}

void QalculateWindow::setResult(Prefix *prefix, bool update_parse, size_t stack_index, bool register_moved, bool noprint) {

	b_busy++;

	if(!view_thread->running && !view_thread->start()) {b_busy--; return;}

	std::string prev_result_text = result_text;
	bool prev_approximate = *settings->printops.is_approximate;
	result_text = "?";

	if(update_parse) {
		parsed_text = tr("aborted").toStdString();
	}

	if(!settings->rpn_mode) stack_index = 0;
	if(stack_index != 0) {
		update_parse = false;
	}
	if(register_moved) {
		update_parse = false;
	}

	if(register_moved) {
		result_text = tr("RPN Register Moved").toStdString();
	}

	settings->printops.prefix = prefix;

	bool parsed_approx = false;
	if(stack_index == 0) {
		if(!view_thread->write((void*) mstruct)) {b_busy--; view_thread->cancel(); return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void*) mreg)) {b_busy--; view_thread->cancel(); return;}
	}
	if(update_parse) {
		if(settings->adaptive_interval_display) {
			if((parsed_mstruct && parsed_mstruct->containsFunctionId(FUNCTION_ID_UNCERTAINTY)) || original_expression.find("+/-") != std::string::npos || original_expression.find("+/" SIGN_MINUS) != std::string::npos || original_expression.find("±") != std::string::npos) settings->printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			else if(parsed_mstruct && parsed_mstruct->containsFunctionId(FUNCTION_ID_INTERVAL)) settings->printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
			else settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		}
		if(!view_thread->write((void *) parsed_mstruct)) {b_busy--; view_thread->cancel(); return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write(parsed_approx_p)) {b_busy--; view_thread->cancel(); return;}
		if(!view_thread->write(prev_result_text == tr("RPN Operation").toStdString() ? false : true)) {b_busy--; view_thread->cancel(); return;}
	} else {
		if(settings->printops.base != BASE_DECIMAL && settings->dual_approximation <= 0) mstruct_exact.setUndefined();
		if(!view_thread->write((void *) NULL)) {b_busy--; view_thread->cancel(); return;}
	}

	int i = 0;
	while(b_busy && view_thread->running && i < 75) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	QProgressDialog *dialog = NULL;
	if(b_busy && view_thread->running) {
		dialog = new QProgressDialog(tr("Processing…"), tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
	}
	while(b_busy && view_thread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(dialog) {
		dialog->hide();
		dialog->deleteLater();
	}

	settings->printops.prefix = NULL;

	if(noprint) {
		return;
	}

	b_busy++;

	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}

	//display_errors();

	if(stack_index != 0) {
		//RPNRegisterChanged(result_text, stack_index);
	} else {
		alt_results.push_back(result_text); 
		historyView->addResult(alt_results, parsed_text, (update_parse || !prev_approximate) && (exact_comparison || (!(*settings->printops.is_approximate) && !mstruct->isApproximate())));
	}

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
		b_busy = false;
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
