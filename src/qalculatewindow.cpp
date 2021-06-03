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
#include <QStatusBar>
#include <QDebug>

#include "qalculatewindow.h"
#include "qalculateqtsettings.h"
#include "expressionedit.h"
#include "historyview.h"

extern QalculateQtSettings *settings;

class ViewThread : public Thread {
protected:
	virtual void run();
};

std::vector<std::string> alt_results;
bool b_busy, exact_comparison;
std::string original_expression, result_text, parsed_text;
ViewThread *view_thread;
MathStructure *mstruct, *parsed_mstruct, mstruct_exact;

extern void fix_to_struct(MathStructure &m);
extern void print_dual(const MathStructure &mresult, const std::string &original_expression, const MathStructure &mparse, MathStructure &mexact, std::string &result_str, std::vector<std::string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle = false, bool *exact_cmp = NULL, bool b_parsed = true, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1);
extern void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const std::string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs = 0, int max_size = 10);
extern int has_information_unit(const MathStructure &m, bool top = true);

QalculateWindow::QalculateWindow() : QMainWindow() {

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	ehSplitter = new QSplitter(Qt::Vertical, this);
	topLayout->addWidget(ehSplitter);

	expressionEdit = new ExpressionEdit(this);
	ehSplitter->addWidget(expressionEdit);
	historyView = new HistoryView(this);
	ehSplitter->addWidget(historyView);
	ehSplitter->setStretchFactor(0, 0);
	ehSplitter->setStretchFactor(1, 1);

	statusBar();

	QLocalServer::removeServer("qalculate-qt");
	server = new QLocalServer(this);
	server->listen("qalculate-qt");
	connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));

	connect(expressionEdit, SIGNAL(returnPressed()), this, SLOT(calculate()));

	mstruct = new MathStructure();
	mstruct_exact.setUndefined();
	parsed_mstruct = new MathStructure();
	view_thread = new ViewThread;

	b_busy = false;

}
QalculateWindow::~QalculateWindow() {}

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
	//if(!command.isEmpty()) openURL(QUrl::fromUserInput(command));
}
void QalculateWindow::setCommandLineParser(QCommandLineParser *p) {
	parser = p;
}

void QalculateWindow::calculate() {
	calculateExpression();
	expressionEdit->clear();
}

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z.toStdString()))
#define EQUALS_IGNORECASE_AND_LOCAL_NR(x,y,z,a)	(equalsIgnoreCase(x, y a) || (x.length() == z.length() + strlen(a) && equalsIgnoreCase(x.substr(0, x.length() - strlen(a)), z.toStdString()) && equalsIgnoreCase(x.substr(x.length() - strlen(a)), a)))

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
		std::string from_str = str;
		//if(test_ask_dot(from_str)) ask_dot();
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
						CALCULATOR->error(true, tr("Time zone parsing failed.").toUtf8(), NULL);
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

	if(settings->caret_as_xor) gsub("^", "⊻", str);

	b_busy = true;

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

	if(CALCULATOR->busy()) {
		statusBar()->showMessage("Calculating...");
		qApp->processEvents();
	}
	while(CALCULATOR->busy()) {
		sleep_ms(100);
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
	}

	/*if(!do_mathoperation && ((ask_questions && test_ask_tc(*parsed_mstruct) && ask_tc()) || (check_exrates && check_exchange_rates()))) {
		b_busy = false;
		calculateExpression(do_mathoperation, op, f, settings->rpn_mode, do_stack ? stack_index : 0, false);
		settings->evalops.auto_post_conversion = save_auto_post_conversion;
		settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
		settings->evalops.parse_options.units_enabled = b_units_saved;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_cbase);
		settings->evalops.complex_number_form = save_complex_number_form;
		complex_angle_form = caf_bak;
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
	}*/

	mstruct_exact.setUndefined();
	
	if((!do_calendars || !mstruct->isDateTime()) && (settings->dual_approximation > 0 || settings->printops.base == BASE_DECIMAL) && !do_bases && !units_changed) {
		long int i_timeleft = 0;
		i_timeleft = mstruct->containsType(STRUCT_COMPARISON) ? 2000 : 1000;
		if(i_timeleft > 0) {
			calculate_dual_exact(mstruct_exact, mstruct, original_expression, parsed_mstruct, settings->evalops, settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_approximation > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), i_timeleft, 5);
		}
	}

	b_busy = false;

	/*if(do_factors || do_expand || do_pfe) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			execute_command(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			mstruct = save_mstruct;
		} else {
			execute_command(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
		}
	}*/

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
			parsed_text = mp.print(po, true, true, TAG_TYPE_HTML);
			if(po.base == BASE_CUSTOM) {
				CALCULATOR->setCustomOutputBase(nr_base);
			}
		}

		po = settings->printops;

		po.allow_non_usable = true;

		print_dual(*mresult, original_expression, mparse ? *mparse : *parsed_mstruct, mstruct_exact, result_text, alt_results, po, settings->evalops, settings->dual_fraction < 0 ? AUTOMATIC_FRACTION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_FRACTION_DUAL : AUTOMATIC_FRACTION_OFF), settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), settings->complex_angle_form, &exact_comparison, mparse != NULL, true, true, TAG_TYPE_HTML);

		b_busy = false;
		CALCULATOR->stopControl();

	}

}

void QalculateWindow::setResult(Prefix *prefix, bool update_parse, size_t stack_index, bool register_moved, bool noprint) {

	b_busy = true;

	if(!view_thread->running && !view_thread->start()) {b_busy = false; return;}

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
		if(!view_thread->write((void*) mstruct)) {b_busy = false; view_thread->cancel(); return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void*) mreg)) {b_busy = false; view_thread->cancel(); return;}
	}
	if(update_parse) {
		if(settings->adaptive_interval_display) {
			if((parsed_mstruct && parsed_mstruct->containsFunctionId(FUNCTION_ID_UNCERTAINTY)) || original_expression.find("+/-") != std::string::npos || original_expression.find("+/" SIGN_MINUS) != std::string::npos || original_expression.find("±") != std::string::npos) settings->printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			else if(parsed_mstruct && parsed_mstruct->containsFunctionId(FUNCTION_ID_INTERVAL)) settings->printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
			else settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		}
		if(!view_thread->write((void *) parsed_mstruct)) {b_busy = false; view_thread->cancel(); return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write(parsed_approx_p)) {b_busy = false; view_thread->cancel(); return;}
		if(!view_thread->write(prev_result_text == tr("RPN Operation").toStdString() ? false : true)) {b_busy = false; view_thread->cancel(); return;}
	} else {
		if(settings->printops.base != BASE_DECIMAL && settings->dual_approximation <= 0) mstruct_exact.setUndefined();
		if(!view_thread->write((void *) NULL)) {b_busy = false; view_thread->cancel(); return;}
	}

	bool has_printed = false;

	int i = 0;
	while(b_busy && view_thread->running && i < 75) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(b_busy && view_thread->running) {
		statusBar()->showMessage(tr("Processing..."));
		qApp->processEvents();
	}
	while(b_busy && view_thread->running) {
		sleep_ms(100);
	}

	settings->printops.prefix = NULL;

	if(noprint) {
		return;
	}

	b_busy = true;

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

	b_busy = false;
}

