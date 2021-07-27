/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "qalculateqtsettings.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>
#include <QMessageBox>
#include <QKeyEvent>
#include <QColor>
#include <QProgressDialog>
#include <QDebug>

extern int b_busy;

bool can_display_unicode_string_function(const char*, void*) {
	return true;
}

std::string to_html_escaped(const std::string strpre) {
	std::string str = strpre;
	if(settings->printops.use_unicode_signs) {
		gsub(">=", SIGN_GREATER_OR_EQUAL, str);
		gsub("<=", SIGN_LESS_OR_EQUAL, str);
		gsub("!=", SIGN_NOT_EQUAL, str);
	}
	gsub("&", "&amp;", str);
	gsub("<", "&lt;", str);
	gsub(">", "&gt;", str);
	gsub("\n","<br>", str);
	return str;
}
bool string_is_less(std::string str1, std::string str2) {
	size_t i = 0;
	bool b_uni = false;
	while(i < str1.length() && i < str2.length()) {
		if(str1[i] == str2[i]) i++;
		else if(str1[i] < 0 || str2[i] < 0) {b_uni = true; break;}
		else return str1[i] < str2[i];
	}
	if(b_uni) return QString::fromStdString(str1).compare(QString::fromStdString(str2)) < 0;
	return str1 < str2;
}
bool item_in_calculator(ExpressionItem *item) {
	if(!CALCULATOR->stillHasVariable((Variable*) item) || !CALCULATOR->stillHasFunction((MathFunction*) item) || !CALCULATOR->stillHasUnit((Unit*) item)) return false;
	if(item->type() == STRUCT_VARIABLE) return CALCULATOR->hasVariable((Variable*) item);
	if(item->type() == STRUCT_UNIT) return CALCULATOR->hasUnit((Unit*) item);
	if(item->type() == STRUCT_FUNCTION) return CALCULATOR->hasFunction((MathFunction*) item);
	return false;
}

AnswerFunction::AnswerFunction() : MathFunction(QApplication::tr("answer").toStdString(), 1, 1, "", QApplication::tr("History Answer Value").toStdString()) {
	if(QApplication::tr("answer") != "answer") addName("answer");
	VectorArgument *arg = new VectorArgument(QApplication::tr("History Index(es)").toStdString());
	arg->addArgument(new IntegerArgument("", ARGUMENT_MIN_MAX_NONZERO, true, true, INTEGER_TYPE_SINT));
	setArgumentDefinition(1, arg);
}
int AnswerFunction::calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions&) {
	if(vargs[0].size() == 0) return 0;
	if(vargs[0].size() > 1) mstruct.clearVector();
	for(size_t i = 0; i < vargs[0].size(); i++) {
		int index = vargs[0][i].number().intValue();
		if(index < 0) index = (int) settings->history_answer.size() + 1 + index;
		if(index <= 0 || index > (int) settings->history_answer.size() || settings->history_answer[(size_t) index - 1] == NULL) {
			CALCULATOR->error(true, QApplication::tr("History index %s does not exist.").toUtf8().data(), vargs[0][i].print().c_str(), NULL);
			if(vargs[0].size() == 1) mstruct.setUndefined();
			else mstruct.addChild(m_undefined);
		} else {
			if(vargs[0].size() == 1) mstruct.set(*settings->history_answer[(size_t) index - 1]);
			else mstruct.addChild(*settings->history_answer[(size_t) index - 1]);
		}
	}
	return 1;
}

QalculateQtSettings::QalculateQtSettings() {
	ignore_locale = false;
	fetch_exchange_rates_at_startup = false;
	allow_multiple_instances = -1;
	std::string filename = buildPath(getLocalDir(), "qalculate-qt.cfg");
	FILE *file = fopen(filename.c_str(), "r");
	char line[10000];
	bool b1 = false, b2 = false;
	std::string stmp;
	if(file) {
		while(true) {
			if(fgets(line, 10000, file) == NULL) break;
			if(strcmp(line, "ignore_locale=1\n") == 0) {
				ignore_locale = true;
				b1 = true; if(b2) break;
				break;
			} else if(strcmp(line, "ignore_locale=0\n") == 0) {
				b1 = true; if(b2) break;
				break;
			} else if(strcmp(line, "allow_multiple_instances=1\n") == 0) {
				allow_multiple_instances = 1;
				b2 = true; if(b1) break;
			} else if(strcmp(line, "allow_multiple_instances=0\n") == 0) {
				allow_multiple_instances = 0;
				b2 = true; if(b1) break;
			} else if(strcmp(line, "allow_multiple_instances=-1\n") == 0) {
				b2 = true; if(b1) break;
			}
		}
		fclose(file);
	}
}
QalculateQtSettings::~QalculateQtSettings() {}

bool QalculateQtSettings::isAnswerVariable(Variable *v) {
	return v == vans[0] || v == vans[1] || v == vans[2] || v == vans[3] || v == vans[4];
}

QIcon load_icon(const QString &str, QWidget *w) {
	QColor c = w->palette().buttonText().color();
	if(c.red() + c.green() + c.blue() < 255) return QIcon(":/icons/actions/scalable/" + str + ".svg");
	else return QIcon(":/icons/dark/actions/scalable/" + str + ".svg");
}

void QalculateQtSettings::loadPreferences() {

	f_answer = CALCULATOR->addFunction(new AnswerFunction());

	CALCULATOR->setPrecision(10);
	CALCULATOR->useIntervalArithmetic(true);
	CALCULATOR->setTemperatureCalculationMode(TEMPERATURE_CALCULATION_HYBRID);
	CALCULATOR->useBinaryPrefixes(0);

	printops.multiplication_sign = MULTIPLICATION_SIGN_X;
	printops.division_sign = DIVISION_SIGN_DIVISION_SLASH;
	printops.is_approximate = new bool(false);
	printops.prefix = NULL;
	printops.use_min_decimals = false;
	printops.use_denominator_prefix = true;
	printops.min_decimals = 0;
	printops.use_max_decimals = false;
	printops.max_decimals = 2;
	printops.base = 10;
	printops.min_exp = EXP_PRECISION;
	printops.negative_exponents = false;
	printops.sort_options.minus_last = true;
	printops.indicate_infinite_series = false;
	printops.show_ending_zeroes = true;
	printops.round_halfway_to_even = false;
	printops.number_fraction_format = FRACTION_DECIMAL;
	printops.restrict_fraction_length = false;
	printops.abbreviate_names = true;
	printops.use_unicode_signs = true;
	printops.digit_grouping = DIGIT_GROUPING_STANDARD;
	printops.use_unit_prefixes = true;
	printops.use_prefixes_for_currencies = false;
	printops.use_prefixes_for_all_units = false;
	printops.spacious = true;
	printops.short_multiplication = true;
	printops.place_units_separately = true;
	printops.use_all_prefixes = false;
	printops.excessive_parenthesis = false;
	printops.allow_non_usable = false;
	printops.lower_case_numbers = false;
	printops.lower_case_e = false;
	printops.base_display = BASE_DISPLAY_SUFFIX;
	printops.twos_complement = true;
	printops.hexadecimal_twos_complement = false;
	printops.limit_implicit_multiplication = false;
	printops.can_display_unicode_string_function = &can_display_unicode_string_function;
	printops.can_display_unicode_string_arg = NULL;
	printops.allow_factorization = false;
	printops.spell_out_logical_operators = true;
	printops.exp_to_root = true;
	printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;

	evalops.approximation = APPROXIMATION_TRY_EXACT;
	evalops.sync_units = true;
	evalops.structuring = STRUCTURING_SIMPLIFY;
	evalops.parse_options.unknowns_enabled = false;
	evalops.parse_options.read_precision = DONT_READ_PRECISION;
	evalops.parse_options.base = BASE_DECIMAL;
	evalops.allow_complex = true;
	evalops.allow_infinite = true;
	evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL;
	evalops.assume_denominators_nonzero = true;
	evalops.warn_about_denominators_assumed_nonzero = true;
	evalops.parse_options.limit_implicit_multiplication = false;
	evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
	evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	evalops.parse_options.comma_as_separator = false;
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	evalops.local_currency_conversion = true;
	evalops.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;

	title_type = TITLE_APP;
	dot_question_asked = false;
	complex_angle_form = false;
	decimal_comma = -1;
	adaptive_interval_display = true;
	tc_set = false;
	dual_fraction = -1;
	dual_approximation = -1;
	auto_update_exchange_rates = 7;
	rpn_mode = false;
	rpn_keys = true;
	rpn_shown = false;
	caret_as_xor = false;
	do_imaginary_j = false;
	color = 1;
	colorize_result = true;
	chain_mode = false;
	enable_input_method = false;
	enable_completion = true;
	enable_completion2 = true;
	completion_min = 1;
	completion_min2 = 1;
	completion_delay = 500;
	always_on_top = false;
	display_expression_status = true;
	expression_status_delay = 1000;
	prefixes_default = true;
	use_custom_result_font = false;
	use_custom_expression_font = false;
	use_custom_keypad_font = false;
	use_custom_app_font = false;
	save_custom_result_font = false;
	save_custom_expression_font = false;
	save_custom_keypad_font = false;
	save_custom_app_font = false;
	custom_result_font = "";
	custom_expression_font = "";
	custom_keypad_font = "";
	custom_app_font = "";
	style = -1;
	palette = -1;
	replace_expression = KEEP_EXPRESSION;
	save_mode_on_exit = true;
	save_defs_on_exit = true;
	clear_history_on_exit = false;
	keep_function_dialog_open = false;
#ifdef _WIN32
	check_version = true;
#else
	check_version = false;
#endif
	last_version_check_date.setToCurrentDate();

	FILE *file = NULL;
	std::string filename = buildPath(getLocalDir(), "qalculate-qt.cfg");
	file = fopen(filename.c_str(), "r");

	default_plot_legend_placement = PLOT_LEGEND_TOP_RIGHT;
	default_plot_display_grid = true;
	default_plot_full_border = false;
	default_plot_min = "0";
	default_plot_max = "10";
	default_plot_step = "1";
	default_plot_sampling_rate = 1001;
	default_plot_linewidth = 2;
	default_plot_rows = false;
	default_plot_type = 0;
	default_plot_style = PLOT_STYLE_LINES;
	default_plot_smoothing = PLOT_SMOOTHING_NONE;
	default_plot_variable = "x";
	default_plot_color = true;
	default_plot_use_sampling_rate = true;
	max_plot_time = 5;

	int version_numbers[] = {3, 20, 0};

	if(file) {
		char line[1000000L];
		std::string stmp, svalue, svar;
		size_t i;
		int v;
		while(true) {
			if(fgets(line, 1000000L, file) == NULL) break;
			stmp = line;
			remove_blank_ends(stmp);
			if((i = stmp.find_first_of("=")) != std::string::npos) {
				svar = stmp.substr(0, i);
				remove_blank_ends(svar);
				svalue = stmp.substr(i + 1);
				remove_blank_ends(svalue);
				v = s2i(svalue);
				if(svar == "history_expression") {
					v_expression.push_back(svalue);
					v_result.push_back(std::vector<std::string>());
					v_exact.push_back(std::vector<int>());
				} else if(svar == "history_result") {
					if(!v_result.empty()) v_result[settings->v_result.size() - 1].push_back(svalue);
				} else if(svar == "history_exact") {
					if(!v_exact.empty()) v_exact[settings->v_exact.size() - 1].push_back(v);
				} else if(svar == "expression_history") {
					expression_history.push_back(svalue);
				} else if(svar == "version") {
					parse_qalculate_version(svalue, version_numbers);
				} else if(svar == "always_on_top") {
					always_on_top = v;
				} else if(svar == "keep_function_dialog_open") {
					keep_function_dialog_open = v;
				} else if(svar == "save_mode_on_exit") {
					save_mode_on_exit = v;
				} else if(svar == "save_definitions_on_exit") {
					save_defs_on_exit = v;
				} else if(svar == "clear_history_on_exit") {
					clear_history_on_exit = v;
				} else if(svar == "window_state") {
					window_state = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "replace_expression") {
					replace_expression = v;
				} else if(svar == "window_geometry") {
					window_geometry = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "splitter_state") {
					splitter_state = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "functions_geometry") {
					functions_geometry = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "functions_vsplitter_state") {
					functions_vsplitter_state = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "functions_hsplitter_state") {
					functions_hsplitter_state = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "style") {
					style = v;
				} else if(svar == "palette") {
					palette = v;
				} else if(svar == "color") {
					colorize_result = v;
				} else if(svar == "ignore_locale") {
					ignore_locale = v;
				} else if(svar == "window_title_mode") {
					if(v >= 0 && v <= 2) title_type = v;
				} else if(svar == "auto_update_exchange_rates") {
					auto_update_exchange_rates = v;
				} else if(svar == "display_expression_status") {
					display_expression_status = v;
				} else if(svar == "expression_status_delay") {
					if(v < 0) v = 0;
					expression_status_delay = v;
				} else if(svar == "enable_input_method") {
					enable_input_method = v;
				} else if(svar == "enable_completion") {
					enable_completion = v;
				} else if(svar == "enable_completion2") {
					enable_completion2 = v;
				} else if(svar == "completion_min") {
					if(v < 1) v = 1;
					completion_min = v;
				} else if(svar == "completion_min2") {
					if(v < 1) v = 1;
					completion_min2 = v;
				} else if(svar == "completion_delay") {
					if(v < 0) v = 0;
					completion_delay = v;
				} else if(svar == "use_custom_result_font") {
					use_custom_result_font = v;
				} else if(svar == "use_custom_expression_font") {
					use_custom_expression_font = v;
				} else if(svar == "use_custom_keypad_font") {
					use_custom_keypad_font = v;
				} else if(svar == "use_custom_application_font") {
					use_custom_app_font = v;
				} else if(svar == "custom_result_font") {
					custom_result_font = svalue;
					save_custom_result_font = true;
				} else if(svar == "custom_expression_font") {
					custom_expression_font = svalue;
					save_custom_expression_font = true;
				} else if(svar == "custom_keypad_font") {
					custom_keypad_font = svalue;
					save_custom_keypad_font = true;
				} else if(svar == "custom_application_font") {
					custom_app_font = svalue;
					save_custom_app_font = true;
				} else if(svar == "plot_legend_placement") {
					if(v >= PLOT_LEGEND_NONE && v <= PLOT_LEGEND_OUTSIDE) default_plot_legend_placement = (PlotLegendPlacement) v;
				} else if(svar == "plot_style") {
					if(v >= PLOT_STYLE_LINES && v <= PLOT_STYLE_DOTS) default_plot_style = (PlotStyle) v;
				} else if(svar == "plot_smoothing") {
					if(v >= PLOT_SMOOTHING_NONE && v <= PLOT_SMOOTHING_SBEZIER) default_plot_smoothing = (PlotSmoothing) v;
				} else if(svar == "plot_display_grid") {
					default_plot_display_grid = v;
				} else if(svar == "plot_full_border") {
					default_plot_full_border = v;
				} else if(svar == "plot_min") {
					default_plot_min = svalue;
				} else if(svar == "plot_max") {
					default_plot_max = svalue;
				} else if(svar == "plot_step") {
					default_plot_step = svalue;
				} else if(svar == "plot_sampling_rate") {
					default_plot_sampling_rate = v;
				} else if(svar == "plot_use_sampling_rate") {
					default_plot_use_sampling_rate = v;
				} else if(svar == "plot_variable") {
					default_plot_variable = svalue;
				} else if(svar == "plot_rows") {
					default_plot_rows = v;
				} else if(svar == "plot_type") {
					if(v >= 0 && v <= 2) default_plot_type = v;
				} else if(svar == "plot_color") {
					default_plot_color = v;
				} else if(svar == "plot_linewidth") {
					default_plot_linewidth = v;
				} else if(svar == "max_plot_time") {
					max_plot_time = v;
				} else if(svar == "check_version") {
					check_version = v;
				} else if(svar == "last_version_check") {
					last_version_check_date.set(svalue);
				} else if(svar == "last_found_version") {
					last_found_version = svalue;
				/*} else if(svar == "bit_width") {
					default_bits = v;
				} else if(svar == "signed_integer") {
					default_signed = v;*/
				} else if(svar == "min_deci") {
					printops.min_decimals = v;
				} else if(svar == "use_min_deci") {
					printops.use_min_decimals = v;
				} else if(svar == "max_deci") {
					printops.max_decimals = v;
				} else if(svar == "use_max_deci") {
					printops.use_max_decimals = v;
				} else if(svar == "precision") {
					CALCULATOR->setPrecision(v);
				} else if(svar == "min_exp") {
					printops.min_exp = v;
				} else if(svar == "interval_arithmetic") {
					CALCULATOR->useIntervalArithmetic(v);
				} else if(svar == "interval_display") {
					if(v == 0) {
						printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; adaptive_interval_display = true;
					} else {
						v--;
						if(v >= INTERVAL_DISPLAY_SIGNIFICANT_DIGITS && v <= INTERVAL_DISPLAY_UPPER) {
							printops.interval_display = (IntervalDisplay) v; adaptive_interval_display = false;
						}
					}
				} else if(svar == "negative_exponents") {
					printops.negative_exponents = v;
				} else if(svar == "sort_minus_last") {
					printops.sort_options.minus_last = v;
				} else if(svar == "place_units_separately") {
					printops.place_units_separately = v;
				} else if(svar == "use_prefixes") {
					printops.use_unit_prefixes = v;
				} else if(svar == "use_prefixes_for_all_units") {
					printops.use_prefixes_for_all_units = v;
				} else if(svar == "use_prefixes_for_currencies") {
					printops.use_prefixes_for_currencies = v;
				} else if(svar == "prefixes_default") {
					prefixes_default = v;
				} else if(svar == "number_fraction_format") {
					if(v >= FRACTION_DECIMAL && v <= FRACTION_COMBINED) {
						printops.number_fraction_format = (NumberFractionFormat) v;
						printops.restrict_fraction_length = (v >= FRACTION_FRACTIONAL);
						dual_fraction = 0;
					} else if(v == FRACTION_COMBINED + 1) {
						printops.number_fraction_format = FRACTION_FRACTIONAL;
						printops.restrict_fraction_length = false;
						dual_fraction = 0;
					} else if(v == FRACTION_COMBINED + 2) {
						printops.number_fraction_format = FRACTION_DECIMAL;
						dual_fraction = 1;
					} else if(v < 0) {
						printops.number_fraction_format = FRACTION_DECIMAL;
						dual_fraction = -1;
					}
				} else if(svar == "complex_number_form") {
					if(v == COMPLEX_NUMBER_FORM_CIS + 1) {
						evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
						complex_angle_form = true;
					} else if(v >= COMPLEX_NUMBER_FORM_RECTANGULAR && v <= COMPLEX_NUMBER_FORM_CIS) {
						evalops.complex_number_form = (ComplexNumberForm) v;
						complex_angle_form = false;
					}
				} else if(svar == "number_base") {
					printops.base = v;
				} else if(svar == "custom_number_base") {
					CALCULATOR->beginTemporaryStopMessages();
					MathStructure m;
					CALCULATOR->calculate(&m, svalue, 500);
					CALCULATOR->endTemporaryStopMessages();
					CALCULATOR->setCustomOutputBase(m.number());
				} else if(svar == "number_base_expression") {
					evalops.parse_options.base = v;
				} else if(svar == "custom_number_base_expression") {
					CALCULATOR->beginTemporaryStopMessages();
					MathStructure m;
					CALCULATOR->calculate(&m, svalue, 500);
					CALCULATOR->endTemporaryStopMessages();
					CALCULATOR->setCustomInputBase(m.number());
				} else if(svar == "read_precision") {
					if(v >= DONT_READ_PRECISION && v <= READ_PRECISION_WHEN_DECIMALS) {
						evalops.parse_options.read_precision = (ReadPrecisionMode) v;
					}
				} else if(svar == "assume_denominators_nonzero") {
					evalops.assume_denominators_nonzero = v;
				} else if(svar == "warn_about_denominators_assumed_nonzero") {
					evalops.warn_about_denominators_assumed_nonzero = v;
				} else if(svar == "structuring") {
					if(v >= STRUCTURING_NONE && v <= STRUCTURING_FACTORIZE) {
						evalops.structuring = (StructuringMode) v;
						printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
					}
				} else if(svar == "angle_unit") {
					if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_GRADIANS) {
						evalops.parse_options.angle_unit = (AngleUnit) v;
					}
				} else if(svar == "functions_enabled") {
					evalops.parse_options.functions_enabled = v;
				} else if(svar == "variables_enabled") {
					evalops.parse_options.variables_enabled = v;
				} else if(svar == "donot_calculate_variables") {
					evalops.calculate_variables = !v;
				} else if(svar == "calculate_variables") {
					evalops.calculate_variables = v;
				} else if(svar == "variable_units_enabled") {
					CALCULATOR->setVariableUnitsEnabled(v);
				} else if(svar == "calculate_functions") {
					evalops.calculate_functions = v;
				} else if(svar == "sync_units") {
					evalops.sync_units = v;
				} else if(svar == "temperature_calculation") {
					CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) v);
					tc_set = true;
				} else if(svar == "unknownvariables_enabled") {
					evalops.parse_options.unknowns_enabled = v;
				} else if(svar == "units_enabled") {
					evalops.parse_options.units_enabled = v;
				} else if(svar == "allow_complex") {
					evalops.allow_complex = v;
				} else if(svar == "allow_infinite") {
					evalops.allow_infinite = v;
				} else if(svar == "use_short_units") {
					printops.abbreviate_names = v;
				} else if(svar == "abbreviate_names") {
					printops.abbreviate_names = v;
				} else if(svar == "all_prefixes_enabled") {
					printops.use_all_prefixes = v;
				} else if(svar == "denominator_prefix_enabled") {
					printops.use_denominator_prefix = v;
				} else if(svar == "auto_post_conversion") {
					if(v >= POST_CONVERSION_NONE && v <= POST_CONVERSION_OPTIMAL) {
						evalops.auto_post_conversion = (AutoPostConversion) v;
					}
				} else if(svar == "mixed_units_conversion") {
					if(v >= MIXED_UNITS_CONVERSION_NONE || v <= MIXED_UNITS_CONVERSION_FORCE_ALL) {
						evalops.mixed_units_conversion = (MixedUnitsConversion) v;
					}
				} else if(svar == "local_currency_conversion") {
					evalops.local_currency_conversion = v;
				} else if(svar == "use_binary_prefixes") {
					CALCULATOR->useBinaryPrefixes(v);
				} else if(svar == "indicate_infinite_series") {
					printops.indicate_infinite_series = v;
				} else if(svar == "show_ending_zeroes") {
					printops.show_ending_zeroes = v;
				} else if(svar == "digit_grouping") {
					if(v >= DIGIT_GROUPING_NONE && v <= DIGIT_GROUPING_LOCALE) {
						printops.digit_grouping = (DigitGrouping) v;
					}
				} else if(svar == "round_halfway_to_even") {
					printops.round_halfway_to_even = v;
				} else if(svar == "approximation") {
					if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
						evalops.approximation = (ApproximationMode) v;
					} else if(v == APPROXIMATION_APPROXIMATE + 1) {
						evalops.approximation = APPROXIMATION_TRY_EXACT;
						dual_approximation = 1;
					} else if(v < 0) {
						evalops.approximation = APPROXIMATION_TRY_EXACT;
						dual_approximation = -1;
					}
				} else if(svar == "interval_calculation") {
					if(v >= INTERVAL_CALCULATION_NONE && v <= INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
						evalops.interval_calculation = (IntervalCalculation) v;
					}
				} else if(svar == "chain_mode") {
					chain_mode = v;
				} else if(svar == "rpn_mode") {
					rpn_mode = v;
				} else if(svar == "rpn_keys") {
					rpn_keys = v;
				} else if(svar == "rpn_shown") {
					rpn_shown = v;
				} else if(svar == "limit_implicit_multiplication") {
					evalops.parse_options.limit_implicit_multiplication = v;
					printops.limit_implicit_multiplication = v;
				} else if(svar == "parsing_mode") {
					evalops.parse_options.parsing_mode = (ParsingMode) v;
				} else if(svar == "default_assumption_type") {
					if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_BOOLEAN) {
						CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
					}
				} else if(svar == "default_assumption_sign") {
					if(v >= ASSUMPTION_SIGN_UNKNOWN && v <= ASSUMPTION_SIGN_NONZERO) {
						CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
					}
				} else if(svar == "spacious") {
					printops.spacious = v;
				} else if(svar == "excessive_parenthesis") {
					printops.excessive_parenthesis = v;
				} else if(svar == "short_multiplication") {
					printops.short_multiplication = v;
				} else if(svar == "use_unicode_signs") {
					printops.use_unicode_signs = v;
				} else if(svar == "e_notation") {
					printops.lower_case_e = v;
				} else if(svar == "lower_case_numbers") {
					printops.lower_case_numbers = v;
				} else if(svar == "imaginary_j") {
					do_imaginary_j = v;
				} else if(svar == "base_display") {
					if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_SUFFIX) printops.base_display = (BaseDisplay) v;
				} else if(svar == "twos_complement") {
					printops.twos_complement = v;
				} else if(svar == "hexadecimal_twos_complement") {
					printops.hexadecimal_twos_complement = v;
				/*} else if(svar == "twos_complement_input") {
					twos_complement_in = v;
				} else if(svar == "hexadecimal_twos_complement_input") {
					hexadecimal_twos_complement_in = v;*/
				} else if(svar == "spell_out_logical_operators") {
					printops.spell_out_logical_operators = v;
				} else if(svar == "caret_as_xor") {
					caret_as_xor = v;
				} else if(svar == "decimal_comma") {
					decimal_comma = v;
					if(v == 0) CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
					else if(v > 0) CALCULATOR->useDecimalComma();
				} else if(svar == "dot_as_separator") {
					if(v < 0) {
						evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
						dot_question_asked = false;
					} else {
						evalops.parse_options.dot_as_separator = v;
						dot_question_asked = true;
					}
				} else if(svar == "comma_as_separator") {
					evalops.parse_options.comma_as_separator = v;
					if(CALCULATOR->getDecimalPoint() != COMMA) {
						CALCULATOR->useDecimalPoint(evalops.parse_options.comma_as_separator);
					}
				}
			}
		}
		fclose(file);
		first_time = false;
	} else {
		first_time = true;
	}

	updateMessagePrintOptions();

	std::string ans_str = "ans";
	vans[0] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str, m_undefined, QApplication::tr("Last Answer").toStdString(), false, true));
	vans[0]->addName(QApplication::tr("answer").toStdString());
	vans[0]->addName(ans_str + "1");
	vans[1] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "2", m_undefined, QApplication::tr("Answer 2").toStdString(), false, true));
	vans[2] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "3", m_undefined, QApplication::tr("Answer 3").toStdString(), false, true));
	vans[3] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "4", m_undefined, QApplication::tr("Answer 4").toStdString(), false, true));
	vans[4] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "5", m_undefined, QApplication::tr("Answer 5").toStdString(), false, true));
	v_memory = new KnownVariable(CALCULATOR->temporaryCategory(), "", m_zero, QApplication::tr("Memory").toStdString(), false, true);
	ExpressionName ename;
	ename.name = "MR";
	ename.case_sensitive = true;
	ename.abbreviation = true;
	v_memory->addName(ename);
	ename.name = "MRC";
	v_memory->addName(ename);
	CALCULATOR->addVariable(v_memory);
	if(do_imaginary_j && CALCULATOR->v_i->hasName("j") == 0) {
		ExpressionName ename = CALCULATOR->v_i->getName(1);
		ename.name = "j";
		ename.reference = false;
		CALCULATOR->v_i->addName(ename, 1, true);
		CALCULATOR->v_i->setChanged(false);
	}

	if(style >= 0) updateStyle();
	else if(palette >= 0) updatePalette();

}
void QalculateQtSettings::updatePalette() {
	QPalette p;
	if(palette == 1) {
		p.setColor(QPalette::Active, QPalette::Window, QColor(42, 46, 50));
		p.setColor(QPalette::Active, QPalette::WindowText, QColor(252, 252, 252));
		p.setColor(QPalette::Active, QPalette::Base, QColor(27, 30, 32));
		p.setColor(QPalette::Active, QPalette::AlternateBase, QColor(35, 38, 41));
		p.setColor(QPalette::Active, QPalette::ToolTipBase, QColor(49, 54, 59));
		p.setColor(QPalette::Active, QPalette::ToolTipText, QColor(252, 252, 252));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		p.setColor(QPalette::Active, QPalette::PlaceholderText, QColor(161, 169, 177));
#endif
		p.setColor(QPalette::Active, QPalette::Text, QColor(252, 252, 252));
		p.setColor(QPalette::Active, QPalette::Button, QColor(49, 54, 59));
		p.setColor(QPalette::Active, QPalette::ButtonText, QColor(252, 252, 252));
		p.setColor(QPalette::Active, QPalette::BrightText, QColor(39, 174, 96));
		p.setColor(QPalette::Inactive, QPalette::Window, QColor(42, 46, 50));
		p.setColor(QPalette::Inactive, QPalette::WindowText, QColor(252, 252, 252));
		p.setColor(QPalette::Inactive, QPalette::Base, QColor(27, 30, 32));
		p.setColor(QPalette::Inactive, QPalette::AlternateBase, QColor(35, 38, 41));
		p.setColor(QPalette::Inactive, QPalette::ToolTipBase, QColor(49, 54, 59));
		p.setColor(QPalette::Inactive, QPalette::ToolTipText, QColor(252, 252, 252));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		p.setColor(QPalette::Inactive, QPalette::PlaceholderText, QColor(161, 169, 177));
#endif
		p.setColor(QPalette::Inactive, QPalette::Text, QColor(252, 252, 252));
		p.setColor(QPalette::Inactive, QPalette::Button, QColor(49, 54, 59));
		p.setColor(QPalette::Inactive, QPalette::ButtonText, QColor(252, 252, 252));
		p.setColor(QPalette::Inactive, QPalette::BrightText, QColor(39, 174, 96));
		p.setColor(QPalette::Disabled, QPalette::Window, QColor(42, 46, 50));
		p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(101, 101, 101));
		p.setColor(QPalette::Disabled, QPalette::Base, QColor(27, 30, 32));
		p.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(35, 38, 41));
		p.setColor(QPalette::Disabled, QPalette::ToolTipBase, QColor(49, 54, 59));
		p.setColor(QPalette::Disabled, QPalette::ToolTipText, QColor(252, 252, 252));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		p.setColor(QPalette::Disabled, QPalette::PlaceholderText, QColor(161, 169, 177));
#endif
		p.setColor(QPalette::Disabled, QPalette::Text, QColor(101, 101, 101));
		p.setColor(QPalette::Disabled, QPalette::Button, QColor(47, 52, 56));
		p.setColor(QPalette::Disabled, QPalette::ButtonText,QColor(101, 101, 101));
		p.setColor(QPalette::Disabled, QPalette::BrightText, QColor(39, 174, 96));
	} else {
		p = QApplication::style()->standardPalette();
	}
	QApplication::setPalette(p);
}
void QalculateQtSettings::updateStyle() {
	if(style >= 0 && style < QStyleFactory::keys().count()) {
		QStyle *s = QStyleFactory::create(QStyleFactory::keys().at(style));
		if(!s) return;
		QApplication::setStyle(s);
	}
	updatePalette();
}
void QalculateQtSettings::savePreferences(bool) {

	FILE *file = NULL;
	std::string homedir = getLocalDir();
	recursiveMakeDir(homedir);
	std::string filename = buildPath(homedir, "qalculate-qt.cfg");
	file = fopen(filename.c_str(), "w+");
	if(file == NULL) {
		QMessageBox::critical(NULL, QApplication::tr("Error"), QApplication::tr("Couldn't write preferences to\n%1").arg(QString::fromStdString(filename)), QMessageBox::Ok);
		return;
	}
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", VERSION);
	fprintf(file, "allow_multiple_instances=%i\n", allow_multiple_instances);
	fprintf(file, "ignore_locale=%i\n", ignore_locale);
	fprintf(file, "check_version=%i\n", check_version);
	if(check_version) {
		fprintf(file, "last_version_check=%s\n", last_version_check_date.toISOString().c_str());
		if(!last_found_version.empty()) fprintf(file, "last_found_version=%s\n", last_found_version.c_str());
	}
	fprintf(file, "window_state=%s\n", window_state.toBase64().data());
	fprintf(file, "window_geometry=%s\n", window_geometry.toBase64().data());
	if(rpn_shown) fprintf(file, "rpn_shown=%i\n", rpn_shown);
	fprintf(file, "splitter_state=%s\n", splitter_state.toBase64().data());
	if(!functions_geometry.isEmpty()) fprintf(file, "functions_geometry=%s\n", functions_geometry.toBase64().data());
	if(!functions_vsplitter_state.isEmpty()) fprintf(file, "functions_vsplitter_state=%s\n", functions_vsplitter_state.toBase64().data());
	if(!functions_hsplitter_state.isEmpty()) fprintf(file, "functions_hsplitter_state=%s\n", functions_hsplitter_state.toBase64().data());
	fprintf(file, "keep_function_dialog_open=%i\n", keep_function_dialog_open);
	if(!units_geometry.isEmpty()) fprintf(file, "units_geometry=%s\n", units_geometry.toBase64().data());
	if(!units_vsplitter_state.isEmpty()) fprintf(file, "units_vsplitter_state=%s\n", units_vsplitter_state.toBase64().data());
	if(!units_hsplitter_state.isEmpty()) fprintf(file, "units_hsplitter_state=%s\n", units_hsplitter_state.toBase64().data());
	if(!variables_geometry.isEmpty()) fprintf(file, "variables_geometry=%s\n", variables_geometry.toBase64().data());
	if(!variables_vsplitter_state.isEmpty()) fprintf(file, "variables_vsplitter_state=%s\n", variables_vsplitter_state.toBase64().data());
	if(!variables_hsplitter_state.isEmpty()) fprintf(file, "variables_hsplitter_state=%s\n", variables_hsplitter_state.toBase64().data());
	fprintf(file, "always_on_top=%i\n", always_on_top);
	if(title_type != TITLE_APP) fprintf(file, "window_title_mode=%i\n", title_type);
	fprintf(file, "save_mode_on_exit=%i\n", save_mode_on_exit);
	fprintf(file, "save_definitions_on_exit=%i\n", save_defs_on_exit);
	fprintf(file, "clear_history_on_exit=%i\n", clear_history_on_exit);
	fprintf(file, "enable_input_method=%i\n", enable_input_method);
	fprintf(file, "display_expression_status=%i\n", display_expression_status);
	fprintf(file, "expression_status_delay=%i\n", expression_status_delay);
	fprintf(file, "enable_completion=%i\n", enable_completion);
	fprintf(file, "enable_completion2=%i\n", enable_completion2);
	fprintf(file, "completion_min=%i\n", completion_min);
	fprintf(file, "completion_min2=%i\n", completion_min2);
	fprintf(file, "completion_delay=%i\n", completion_delay);
	fprintf(file, "style=%i\n", style);
	fprintf(file, "palette=%i\n", palette);
	fprintf(file, "color=%i\n", colorize_result);
	fprintf(file, "use_custom_result_font=%i\n", use_custom_result_font);
	fprintf(file, "use_custom_expression_font=%i\n", use_custom_expression_font);
	fprintf(file, "use_custom_keypad_font=%i\n", use_custom_keypad_font);
	fprintf(file, "use_custom_application_font=%i\n", use_custom_app_font);
	if(use_custom_result_font || save_custom_result_font) fprintf(file, "custom_result_font=%s\n", custom_result_font.c_str());
	if(use_custom_expression_font || save_custom_expression_font) fprintf(file, "custom_expression_font=%s\n", custom_expression_font.c_str());
	if(use_custom_keypad_font || save_custom_keypad_font) fprintf(file, "custom_keypad_font=%s\n", custom_keypad_font.c_str());
	if(use_custom_app_font || save_custom_app_font) fprintf(file, "custom_application_font=%s\n", custom_app_font.c_str());
	fprintf(file, "replace_expression=%i\n", replace_expression);
	fprintf(file, "rpn_keys=%i\n", rpn_keys);
	/*if(default_bits >= 0) fprintf(file, "bit_width=%i\n", default_bits);
	if(default_signed >= 0) fprintf(file, "signed_integer=%i\n", default_signed);*/
	fprintf(file, "spell_out_logical_operators=%i\n", printops.spell_out_logical_operators);
	fprintf(file, "caret_as_xor=%i\n", caret_as_xor);
	fprintf(file, "digit_grouping=%i\n", printops.digit_grouping);
	fprintf(file, "decimal_comma=%i\n", decimal_comma);
	fprintf(file, "dot_as_separator=%i\n", dot_question_asked ? evalops.parse_options.dot_as_separator : -1);
	fprintf(file, "comma_as_separator=%i\n", evalops.parse_options.comma_as_separator);
	fprintf(file, "twos_complement=%i\n", printops.twos_complement);
	fprintf(file, "hexadecimal_twos_complement=%i\n", printops.hexadecimal_twos_complement);
	/*fprintf(file, "twos_complement_input=%i\n", twos_complement_in);
	fprintf(file, "hexadecimal_twos_complement_input=%i\n", hexadecimal_twos_complement_in);*/
	fprintf(file, "use_unicode_signs=%i\n", printops.use_unicode_signs);
	fprintf(file, "lower_case_numbers=%i\n", printops.lower_case_numbers);
	fprintf(file, "e_notation=%i\n", printops.lower_case_e);
	fprintf(file, "imaginary_j=%i\n", CALCULATOR->v_i->hasName("j") > 0);
	fprintf(file, "base_display=%i\n", printops.base_display);
	if(tc_set) fprintf(file, "temperature_calculation=%i\n", CALCULATOR->getTemperatureCalculationMode());
	fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
	fprintf(file, "local_currency_conversion=%i\n", evalops.local_currency_conversion);
	fprintf(file, "use_binary_prefixes=%i\n", CALCULATOR->usesBinaryPrefixes());
	fprintf(file, "prefixes_default=%i\n", prefixes_default);
	fprintf(file, "\n[Mode]\n");
	fprintf(file, "min_deci=%i\n", printops.min_decimals);
	fprintf(file, "use_min_deci=%i\n", printops.use_min_decimals);
	fprintf(file, "max_deci=%i\n", printops.max_decimals);
	fprintf(file, "use_max_deci=%i\n", printops.use_max_decimals);
	fprintf(file, "precision=%i\n", CALCULATOR->getPrecision());
	//fprintf(file, "interval_arithmetic=%i\n", CALCULATOR->usesIntervalArithmetic());
	fprintf(file, "interval_display=%i\n", adaptive_interval_display ? 0 : printops.interval_display + 1);
	fprintf(file, "min_exp=%i\n", printops.min_exp);
	fprintf(file, "negative_exponents=%i\n", printops.negative_exponents);
	fprintf(file, "sort_minus_last=%i\n", printops.sort_options.minus_last);
	if(dual_fraction < 0) fprintf(file, "number_fraction_format=%i\n", -1);
	else if(dual_fraction > 0) fprintf(file, "number_fraction_format=%i\n", FRACTION_COMBINED + 2);
	else fprintf(file, "number_fraction_format=%i\n", printops.restrict_fraction_length && printops.number_fraction_format == FRACTION_FRACTIONAL ? FRACTION_COMBINED + 1 : printops.number_fraction_format);
	fprintf(file, "complex_number_form=%i\n", (complex_angle_form && evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) ? evalops.complex_number_form + 1 : evalops.complex_number_form);
	fprintf(file, "use_prefixes=%i\n", printops.use_unit_prefixes);
	fprintf(file, "use_prefixes_for_all_units=%i\n", printops.use_prefixes_for_all_units);
	fprintf(file, "use_prefixes_for_currencies=%i\n", printops.use_prefixes_for_currencies);
	fprintf(file, "use_binary_prefixes=%i\n", CALCULATOR->usesBinaryPrefixes());
	fprintf(file, "abbreviate_names=%i\n", printops.abbreviate_names);
	fprintf(file, "all_prefixes_enabled=%i\n", printops.use_all_prefixes);
	fprintf(file, "denominator_prefix_enabled=%i\n", printops.use_denominator_prefix);
	fprintf(file, "place_units_separately=%i\n", printops.place_units_separately);
	fprintf(file, "auto_post_conversion=%i\n", evalops.auto_post_conversion);
	fprintf(file, "mixed_units_conversion=%i\n", evalops.mixed_units_conversion);
	fprintf(file, "number_base=%i\n", printops.base);
	if(!CALCULATOR->customOutputBase().isZero()) fprintf(file, "custom_number_base=%s\n", CALCULATOR->customOutputBase().print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "number_base_expression=%i\n", evalops.parse_options.base);
	if(!CALCULATOR->customInputBase().isZero()) fprintf(file, "custom_number_base_expression=%s\n", CALCULATOR->customInputBase().print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "read_precision=%i\n", evalops.parse_options.read_precision);
	fprintf(file, "assume_denominators_nonzero=%i\n", evalops.assume_denominators_nonzero);
	fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", evalops.warn_about_denominators_assumed_nonzero);
	fprintf(file, "structuring=%i\n", evalops.structuring);
	fprintf(file, "angle_unit=%i\n", evalops.parse_options.angle_unit);
	fprintf(file, "functions_enabled=%i\n", evalops.parse_options.functions_enabled);
	fprintf(file, "variables_enabled=%i\n", evalops.parse_options.variables_enabled);
	fprintf(file, "calculate_functions=%i\n", evalops.calculate_functions);
	fprintf(file, "calculate_variables=%i\n", evalops.calculate_variables);
	fprintf(file, "variable_units_enabled=%i\n", CALCULATOR->variableUnitsEnabled());
	fprintf(file, "sync_units=%i\n", evalops.sync_units);
	fprintf(file, "unknownvariables_enabled=%i\n", evalops.parse_options.unknowns_enabled);
	fprintf(file, "units_enabled=%i\n", evalops.parse_options.units_enabled);
	fprintf(file, "allow_complex=%i\n", evalops.allow_complex);
	fprintf(file, "allow_infinite=%i\n", evalops.allow_infinite);
	fprintf(file, "indicate_infinite_series=%i\n", printops.indicate_infinite_series);
	fprintf(file, "show_ending_zeroes=%i\n", printops.show_ending_zeroes);
	fprintf(file, "round_halfway_to_even=%i\n", printops.round_halfway_to_even);
	if(dual_approximation < 0) fprintf(file, "approximation=%i\n", -1);
	else if(dual_approximation > 0) fprintf(file, "approximation=%i\n", APPROXIMATION_APPROXIMATE + 1);
	else fprintf(file, "approximation=%i\n", evalops.approximation);
	fprintf(file, "interval_calculation=%i\n", evalops.interval_calculation);
	fprintf(file, "rpn_mode=%i\n", rpn_mode);
	fprintf(file, "chain_mode=%i\n", chain_mode);
	fprintf(file, "limit_implicit_multiplication=%i\n", evalops.parse_options.limit_implicit_multiplication);
	fprintf(file, "parsing_mode=%i\n", evalops.parse_options.parsing_mode);
	fprintf(file, "spacious=%i\n", printops.spacious);
	fprintf(file, "excessive_parenthesis=%i\n", printops.excessive_parenthesis);
	fprintf(file, "default_assumption_type=%i\n", CALCULATOR->defaultAssumptions()->type());
	if(CALCULATOR->defaultAssumptions()->type() != ASSUMPTION_TYPE_BOOLEAN) fprintf(file, "default_assumption_sign=%i\n", CALCULATOR->defaultAssumptions()->sign());
	fprintf(file, "\n[Plotting]\n");
	fprintf(file, "plot_legend_placement=%i\n", default_plot_legend_placement);
	fprintf(file, "plot_style=%i\n", default_plot_style);
	fprintf(file, "plot_smoothing=%i\n", default_plot_smoothing);
	fprintf(file, "plot_display_grid=%i\n", default_plot_display_grid);
	fprintf(file, "plot_full_border=%i\n", default_plot_full_border);
	fprintf(file, "plot_min=%s\n", default_plot_min.c_str());
	fprintf(file, "plot_max=%s\n", default_plot_max.c_str());
	fprintf(file, "plot_step=%s\n", default_plot_step.c_str());
	fprintf(file, "plot_sampling_rate=%i\n", default_plot_sampling_rate);
	fprintf(file, "plot_use_sampling_rate=%i\n", default_plot_use_sampling_rate);
	fprintf(file, "plot_variable=%s\n", default_plot_variable.c_str());
	fprintf(file, "plot_rows=%i\n", default_plot_rows);
	fprintf(file, "plot_type=%i\n", default_plot_type);
	fprintf(file, "plot_color=%i\n", default_plot_color);
	fprintf(file, "plot_linewidth=%i\n", default_plot_linewidth);
	if(max_plot_time != 5) fprintf(file, "max_plot_time=%i\n", max_plot_time);
	if(!clear_history_on_exit) {
		fprintf(file, "\n[History]\n");
		if(!v_expression.empty()) {
			long int n = 0;
			size_t i = v_expression.size() - 1;
			while(true) {
				n++;
				for(size_t i2 = 0; i2 < settings->v_result[i].size(); i2++) {
					if(v_result[i][i2].length() > 300 && (v_result[i][i2].length() <= 6000 || unhtmlize(v_result[i][i2]).length() <= 5000)) {
						n += v_result[i][i2].length() / 300;
					}
					n++;
				}
				if(n >= 300 || i == 0) break;
				i--;
			}
			for(; i < v_expression.size(); i++) {
				fprintf(file, "history_expression=%s\n", v_expression[i].c_str());
				n++;
				for(size_t i2 = 0; i2 < settings->v_result[i].size(); i2++) {
					fprintf(file, "history_exact=%i\n", v_exact[i][i2]);
					if(v_result[i][i2].length() > 6000) {
						std::string str = unhtmlize(v_result[i][i2]);
						if(str.length() > 5000) {
							int index = 50;
							while(index >= 0 && str[index] < 0 && (unsigned char) str[index + 1] < 0xC0) index--;
							gsub("\n", "<br>", str);
							fprintf(file, "history_result=%s …\n", str.substr(0, index + 1).c_str());
						} else {
							fprintf(file, "history_result=%s\n", v_result[i][i2].c_str());
						}
					} else {
						fprintf(file, "history_result=%s\n", v_result[i][i2].c_str());
					}
				}
			}
		}
		for(size_t i = 0; i < expression_history.size(); i++) {
			gsub("\n", " ", expression_history[i]);
			fprintf(file, "expression_history=%s\n", expression_history[i].c_str());
		}
	}
	fclose(file);

}

void QalculateQtSettings::updateMessagePrintOptions() {
	PrintOptions message_printoptions = printops;
	message_printoptions.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	message_printoptions.show_ending_zeroes = false;
	message_printoptions.base = 10;
	if(printops.min_exp < -10 || printops.min_exp > 10 || ((printops.min_exp == EXP_PRECISION || printops.min_exp == EXP_NONE) && PRECISION > 10)) message_printoptions.min_exp = 10;
	else if(printops.min_exp == EXP_NONE) message_printoptions.min_exp = EXP_PRECISION;
	if(PRECISION > 10) {
		message_printoptions.use_max_decimals = true;
		message_printoptions.max_decimals = 10;
	}
	CALCULATOR->setMessagePrintOptions(message_printoptions);
}

MathLineEdit::MathLineEdit(QWidget *parent) : QLineEdit(parent) {
#ifndef _WIN32
	setAttribute(Qt::WA_InputMethodEnabled, settings->enable_input_method);
#endif
}
MathLineEdit::~MathLineEdit() {}
void MathLineEdit::keyPressEvent(QKeyEvent *event) {
	if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::GroupSwitchModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
		switch(event->key()) {
			case Qt::Key_Asterisk: {
				insert(SIGN_MULTIPLICATION);
				return;
			}
			case Qt::Key_Minus: {
				insert(SIGN_MINUS);
				return;
			}
			case Qt::Key_Dead_Circumflex: {
				insert(settings->caret_as_xor ? " xor " : "^");
				return;
			}
			case Qt::Key_Dead_Tilde: {
				insert("~");
				return;
			}
			case Qt::Key_AsciiCircum: {
				if(settings->caret_as_xor) {
					insert(" xor ");
					return;
				}
				break;
			}
		}
	}
	if(event->key() == Qt::Key_Asterisk && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::KeypadModifier) || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
		insert("^");
		return;
	}
	QLineEdit::keyPressEvent(event);
	if(event->key() == Qt::Key_Return) event->accept();
}
bool QalculateQtSettings::checkExchangeRates(QWidget *parent) {
	int i = CALCULATOR->exchangeRatesUsed();
	if(i == 0) return false;
	if(auto_update_exchange_rates == 0) return false;
	if(CALCULATOR->checkExchangeRatesDate(auto_update_exchange_rates > 0 ? auto_update_exchange_rates : 7, false, auto_update_exchange_rates == 0, i)) return false;
	if(auto_update_exchange_rates == 0) return false;
	bool b = false;
	if(auto_update_exchange_rates < 0) {
		int days = (int) floor(difftime(time(NULL), CALCULATOR->getExchangeRatesTime(i)) / 86400);
		if(QMessageBox::question(parent, tr("Update exchange rates?"), tr("It has been %n day(s) since the exchange rates last were updated.\n\nDo you wish to update the exchange rates now?", "", days), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
			b = true;
		}
	}
	if(b || auto_update_exchange_rates > 0) {
		if(auto_update_exchange_rates <= 0) i = -1;
		fetchExchangeRates(b ? 15 : 8, i);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}
class FetchExchangeRatesThread : public Thread {
	protected:
		void run() {
			int timeout = 15;
			int n = -1;
			if(!read(&timeout)) return;
			if(!read(&n)) return;
			CALCULATOR->fetchExchangeRates(timeout, n);
		}
};

void QalculateQtSettings::fetchExchangeRates(int timeout, int n, QWidget *parent) {
	b_busy++;
	FetchExchangeRatesThread fetch_thread;
	if(fetch_thread.start() && fetch_thread.write(timeout) && fetch_thread.write(n)) {
		if(fetch_thread.running) {
			QProgressDialog *dialog = new QProgressDialog(tr("Fetching exchange rates…"), QString(), 0, 0, parent);
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
bool QalculateQtSettings::displayMessages(QWidget *parent) {
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

#ifdef AUTO_UPDATE
#	include <QProcess>
void QalculateQtSettings::autoUpdate(std::string new_version, QWidget *parent) {
	char selfpath[1000];
	ssize_t n = readlink("/proc/self/exe", selfpath, 999);
	if(n < 0 || n >= 999) {
		QMessageBox::critical(parent, tr("Error"), tr("Path of executable not found."));
		return;
	}
	selfpath[n] = '\0';
	std::string selfdir = selfpath;
	size_t i = selfdir.rfind("/");
	if(i != string::npos) selfdir.substr(0, i);
	FILE *pipe = popen("curl --version 1>/dev/null", "w");
	if(!pipe) {
		QMessageBox::critical(parent, tr("Error"), tr("curl not found."));
		return;
	}
	pclose(pipe);
	std::string tmpdir = getLocalTmpDir();
	recursiveMakeDir(tmpdir);
	string script = "#!/bin/sh\n\n";
	script += "echo \"Updating Qalculate!...\";\n";
	script += "sleep 1;\n";
	script += "new_version="; script += new_version; script += ";\n";
	script += "if cd \""; script += tmpdir; script += "\"; then\n";
	script += "\tif curl -L -o qalculate-${new_version}-x86_64.tar.xz https://github.com/Qalculate/qalculate-gtk/releases/download/v${new_version}/qalculate-${new_version}-x86_64.tar.xz; then\n";
	script += "\t\techo \"Extracting files...\";\n";
	script += "\t\tif tar -xJf qalculate-${new_version}-x86_64.tar.xz; then\n";
	script += "\t\t\tcd  qalculate-${new_version};\n";
	script += "\t\t\tif cp -f qalculate-qt \""; script += selfpath; script += "\"; then\n";
	script += "\t\t\t\tcp -f qalc \""; script += selfdir; script += "/\";\n";
	script += "\t\t\t\tcd ..;\n\t\t\trm -r qalculate-${new_version};\n\t\t\trm qalculate-${new_version}-x86_64.tar.xz;\n";
	script += "\t\t\t\texit 0;\n";
	script += "\t\t\tfi\n";
	script += "\t\t\tcd ..;\n\t\trm -r qalculate-${new_version};\n";
	script += "\t\tfi\n";
	script += "\t\trm qalculate-${new_version}-x86_64.tar.xz;\n";
	script += "\tfi\n";
	script += "fi\n";
	script += "echo \"Update failed\";\n";
	script += "echo \"Press Enter to continue\";\n";
	script += "read _;\n";
	script += "exit 1\n";
	std::ofstream ofs;
	std::string scriptpath = tmpdir; scriptpath += "/update.sh";
	ofs.open(scriptpath.c_str(), std::ofstream::out | std::ofstream::trunc);
	ofs << script;
	ofs.close();
	chmod(scriptpath.c_str(), S_IRWXU);
	std::string termcom = "#!/bin/sh\n\n";
	termcom += "if [ $(command -v gnome-terminal) ]; then\n";
	termcom += "\tif gnome-terminal --wait --version; then\n\t\tdetected_term=\"gnome-terminal --wait -- \";\n";
	termcom += "\telse\n\t\tdetected_term=\"gnome-terminal --disable-factory -- \";\n\tfi\n";
	termcom += "elif [ $(command -v xfce4-terminal) ]; then\n\tdetected_term=\"xfce4-terminal --disable-server -e \";\n";
	termcom += "else\n";
	termcom += "\tfor t in x-terminal-emulator konsole alacritty qterminal xterm urxvt rxvt kitty sakura terminology termite tilix; do\n\t\tif [ $(command -v $t) ]; then\n\t\t\tdetected_term=\"$t -e \";\n\t\t\tbreak\n\t\tfi\n\tdone\nfi\n";
	termcom += "$detected_term "; termcom += scriptpath; termcom += ";\n";
	termcom += "exec "; termcom += selfpath; termcom += "\n";
	std::ofstream ofs2;
	std::string scriptpath2 = tmpdir; scriptpath2 += "/terminal.sh";
	ofs2.open(scriptpath2.c_str(), std::ofstream::out | std::ofstream::trunc);
	ofs2 << termcom;
	ofs2.close();
	chmod(scriptpath2.c_str(), S_IRWXU);
	if(QProcess::execute(scriptpath2.c_str()) != 0) {
		QMessageBox::critical(parent, tr("Error"), tr("Failed to run update script.\n%1").arg(error->message));
		return;
	}
	qApp->closeAllWindows();
}
#else
void QalculateQtSettings::autoUpdate(std::string, QWidget*) {}
#endif

void QalculateQtSettings::checkVersion(bool force, QWidget *parent) {
	if(!force) {
		if(!check_version) return;
		QalculateDateTime next_version_check_date(last_version_check_date);
		next_version_check_date.addDays(14);
		if(next_version_check_date.isFutureDate()) return;
	}
	std::string new_version;
#ifdef _WIN32
	int ret = checkAvailableVersion("windows", VERSION, &new_version, force ? 10 : 5);
#else
	int ret = checkAvailableVersion("qalculate-qt", VERSION, &new_version, force ? 10 : 5);
#endif
	if(force && ret <= 0) {
		if(ret < 0) QMessageBox::critical(parent, tr("Error"), tr("Failed to check for updates."));
		else QMessageBox::information(parent, QString(), tr("No updates found."));
		if(ret < 0) return;
	}
	if(ret > 0 && (force || new_version != last_found_version)) {
		last_found_version = new_version;
#ifdef AUTO_UPDATE
		if(QMessageBox::question(parent, QString(), tr("A new version of %1 is available at %2.\n\nDo you wish to update to version %3?").arg("Qalculate!").arg("<a href=\"http://qalculate.github.io/downloads.html\">qalculate.github.io</a>").arg(QString::fromStdString(new_version)) == QMessageBox::Yes)) {
			autoUpdate(new_version);
		}
#else
		QMessageBox::information(parent, QString(), tr("A new version of %1 is available.\n\nYou can get version %3 at %2.").arg("Qalculate!").arg("<a href=\"http://qalculate.github.io/downloads.html\">qalculate.github.io</a>").arg(QString::fromStdString(new_version)));
#endif
	}
	last_version_check_date.setToCurrentDate();
}

