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
#include <QWidget>
#include <QMessageBox>
#include <QColor>
#include <QDebug>

bool can_display_unicode_string_function(const char*, void*) {
	return true;
}

AnswerFunction::AnswerFunction() : MathFunction(QApplication::tr("answer").toStdString(), 1, 1, CALCULATOR->f_warning->category(), QApplication::tr("History Answer Value").toStdString()) {
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
	std::string filename = buildPath(getLocalDir(), "qalculate-qt.cfg");
	FILE *file = fopen(filename.c_str(), "r");
	char line[10000];
	std::string stmp;
	if(file) {
		while(true) {
			if(fgets(line, 10000, file) == NULL) break;
			if(strcmp(line, "ignore_locale=1\n") == 0) {
				ignore_locale = true;
				break;
			} else if(strcmp(line, "ignore_locale=0\n") == 0) {
				break;
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
	printops.base_display = BASE_DISPLAY_NORMAL;
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
	b_decimal_comma = -1;
	adaptive_interval_display = true;
	tc_set = false;
	dual_fraction = -1;
	dual_approximation = 1;
	auto_update_exchange_rates = 7;
	rpn_mode = false;
	caret_as_xor = false;
	do_imaginary_j = false;
	color = 1;
	rpn_mode = false;
	enable_input_method = false;
	enable_completion = true;
	enable_completion2 = true;
	completion_min = 1;
	completion_min2 = 1;
	completion_delay = 500;

	FILE *file = NULL;
	std::string filename = buildPath(getLocalDir(), "qalculate-qt.cfg");
	file = fopen(filename.c_str(), "r");

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
				if(svar == "version") {
					parse_qalculate_version(svalue, version_numbers);
				/*} else if(svar == "allow_multiple_instances") {
					if(v == 0 && version_numbers[0] < 3) v = -1;
					allow_multiple_instances = v;
				} else if(svar == "always_on_top") {
					always_on_top = v;
				} else if(svar == "keep_function_dialog_open") {
					keep_function_dialog_open = v;
				} else if(svar == "save_mode_on_exit") {
					save_mode_on_exit = v;
				} else if(svar == "save_definitions_on_exit") {
					save_defs_on_exit = v;
				} else if(svar == "clear_history_on_exit") {
					clear_history_on_exit = v;*/
				} else if(svar == "window_state") {
					window_state = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "window_geometry") {
					window_geometry = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "splitter_state") {
					splitter_state = QByteArray::fromBase64(svalue.c_str());
				} else if(svar == "ignore_locale") {
					ignore_locale = v;
				} else if(svar == "window_title_mode") {
					if(v >= 0 && v <= 4) title_type = v;
				} else if(svar == "auto_update_exchange_rates") {
					auto_update_exchange_rates = v;
				} else if(svar == "expression_history") {
					expression_history.push_back(svalue);
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
				/*} else if(svar == "check_version") {
					check_version = v;
				} else if(svar == "last_version_check") {
					last_version_check_date.set(svalue);
				} else if(svar == "last_found_version") {
					last_found_version = svalue;
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
				} else if(svar == "programming_outbase") {
					programming_outbase = v;
				} else if(svar == "programming_inbase") {
					programming_inbase = v;
				} else if(svar == "bit_width") {
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
				} else if(svar == "number_fraction_format") {
					if(v >= FRACTION_DECIMAL && v <= FRACTION_COMBINED) {
						printops.number_fraction_format = (NumberFractionFormat) v;
					}
					printops.restrict_fraction_length = (printops.number_fraction_format >= FRACTION_FRACTIONAL);
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
					}
				} else if(svar == "interval_calculation") {
					if(v >= INTERVAL_CALCULATION_NONE && v <= INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
						evalops.interval_calculation = (IntervalCalculation) v;
					}
				/*} else if(svar == "chain_mode") {
					chain_mode = v;*/
				} else if(svar == "in_rpn_mode") {
					rpn_mode = v;
				/*} else if(svar == "rpn_keys") {
					rpn_keys = v;*/
				} else if(svar == "rpn_syntax") {
					if(v) {
						evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
					}
				} else if(svar == "limit_implicit_multiplication") {
					evalops.parse_options.limit_implicit_multiplication = v;
					printops.limit_implicit_multiplication = v;
				} else if(svar == "parsing_mode") {
					if((evalops.parse_options.parsing_mode != PARSING_MODE_RPN || version_numbers[0] > 3 || (version_numbers[0] == 3 && version_numbers[1] > 15)) && v >= PARSING_MODE_ADAPTIVE && v <= PARSING_MODE_RPN) {
						evalops.parse_options.parsing_mode = (ParsingMode) v;
					}
				} else if(svar == "default_assumption_type") {
					if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_BOOLEAN) {
						if(v < ASSUMPTION_TYPE_NUMBER && version_numbers[0] < 1) v = ASSUMPTION_TYPE_NUMBER;
						if(v == ASSUMPTION_TYPE_COMPLEX && version_numbers[0] < 2) v = ASSUMPTION_TYPE_NUMBER;
						CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
					}
				} else if(svar == "default_assumption_sign") {
					if(v >= ASSUMPTION_SIGN_UNKNOWN && v <= ASSUMPTION_SIGN_NONZERO) {
						if(v == ASSUMPTION_SIGN_NONZERO && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
							v = ASSUMPTION_SIGN_UNKNOWN;
						}
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
				} else if(svar == "lower_case_numbers") {
					printops.lower_case_numbers = v;
				} else if(svar == "lower_case_e") {
					printops.lower_case_e = v;
				} else if(svar == "imaginary_j") {
					do_imaginary_j = v;
				} else if(svar == "base_display") {
					if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_ALTERNATIVE) printops.base_display = (BaseDisplay) v;
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
					b_decimal_comma = v;
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
	vans[0] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str, m_undefined, QApplication::tr("Last Answer").toStdString(), false));
	vans[0]->addName(QApplication::tr("answer").toStdString());
	vans[0]->addName(ans_str + "1");
	vans[1] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "2", m_undefined, QApplication::tr("Answer 2").toStdString(), false));
	vans[2] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "3", m_undefined, QApplication::tr("Answer 3").toStdString(), false));
	vans[3] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "4", m_undefined, QApplication::tr("Answer 4").toStdString(), false));
	vans[4] = (KnownVariable*) CALCULATOR->addVariable(new KnownVariable(CALCULATOR->temporaryCategory(), ans_str + "5", m_undefined, QApplication::tr("Answer 5").toStdString(), false));
	v_memory = new KnownVariable(CALCULATOR->temporaryCategory(), "", m_zero, QApplication::tr("Memory").toStdString(), true, true);
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

}
void QalculateQtSettings::savePreferences() {

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
	fprintf(file, "window_state=%s\n", window_state.toBase64().data());
	fprintf(file, "window_geometry=%s\n", window_geometry.toBase64().data());
	fprintf(file, "splitter_state=%s\n", splitter_state.toBase64().data());
	fprintf(file, "enable_input_method=%i\n", enable_input_method);
	fprintf(file, "enable_completion=%i\n", enable_completion);
	fprintf(file, "enable_completion2=%i\n", enable_completion2);
	fprintf(file, "completion_min=%i\n", completion_min);
	fprintf(file, "completion_min2=%i\n", completion_min2);
	fprintf(file, "completion_delay=%i\n", completion_delay);
	for(size_t i = 0; i < expression_history.size(); i++) {
		gsub("\n", " ", expression_history[i]);
		fprintf(file, "expression_history=%s\n", expression_history[i].c_str());
	}
	fprintf(file, "\n[Mode]\n");
	fprintf(file, "precision=%i\n", CALCULATOR->getPrecision());
	fprintf(file, "min_exp=%i\n", settings->printops.min_exp);
	fprintf(file, "negative_exponents=%i\n", settings->printops.negative_exponents);
	fprintf(file, "sort_minus_last=%i\n", settings->printops.sort_options.minus_last);
	fprintf(file, "use_prefixes=%i\n", settings->printops.use_unit_prefixes);
	fprintf(file, "use_prefixes_for_all_units=%i\n", settings->printops.use_prefixes_for_all_units);
	fprintf(file, "use_prefixes_for_currencies=%i\n", settings->printops.use_prefixes_for_currencies);
	fprintf(file, "use_binary_prefixes=%i\n", CALCULATOR->usesBinaryPrefixes());
	fprintf(file, "abbreviate_names=%i\n", settings->printops.abbreviate_names);
	fprintf(file, "all_prefixes_enabled=%i\n", settings->printops.use_all_prefixes);
	fprintf(file, "denominator_prefix_enabled=%i\n", settings->printops.use_denominator_prefix);
	fprintf(file, "place_units_separately=%i\n", settings->printops.place_units_separately);
	fprintf(file, "auto_post_conversion=%i\n", settings->evalops.auto_post_conversion);
	fprintf(file, "mixed_units_conversion=%i\n", settings->evalops.mixed_units_conversion);
	fprintf(file, "local_currency_conversion=%i\n", settings->evalops.local_currency_conversion);
	fprintf(file, "number_base=%i\n", settings->printops.base);
	if(!CALCULATOR->customOutputBase().isZero()) fprintf(file, "custom_number_base=%s\n", CALCULATOR->customOutputBase().print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "number_base_expression=%i\n", settings->evalops.parse_options.base);
	if(!CALCULATOR->customInputBase().isZero()) fprintf(file, "custom_number_base_expression=%s\n", CALCULATOR->customInputBase().print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "read_precision=%i\n", settings->evalops.parse_options.read_precision);
	fprintf(file, "assume_denominators_nonzero=%i\n", settings->evalops.assume_denominators_nonzero);
	fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", settings->evalops.warn_about_denominators_assumed_nonzero);
	fprintf(file, "structuring=%i\n", settings->evalops.structuring);
	fprintf(file, "angle_unit=%i\n", settings->evalops.parse_options.angle_unit);
	fprintf(file, "show_ending_zeroes=%i\n", settings->printops.show_ending_zeroes);
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

