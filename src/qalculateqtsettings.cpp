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
#include <QKeySequence>
#include <QDebug>

#define PREFERENCES_VERSION_BEFORE(i1, i2, i3) (preferences_version[0] < i1 || (preferences_version[0] == i1 && (preferences_version[1] < i2 || (preferences_version[1] == i2 && preferences_version[2] < i3))))
#define PREFERENCES_VERSION_AFTER(i1, i2, i3) (preferences_version[0] > i1 || (preferences_version[0] == i1 && (preferences_version[1] > i2 || (preferences_version[1] == i2 && preferences_version[2] > i3))))

#define RESET_TZ 	printops.custom_time_zone = (rounding_mode == 2 ? TZ_TRUNCATE : 0);\
			if(use_duo_syms) printops.custom_time_zone += TZ_DOZENAL;\
			printops.time_zone = TIME_ZONE_LOCAL;

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
		gsub("...", "…", str);
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
		else if((signed char) str1[i] < 0 || (signed char) str2[i] < 0) {b_uni = true; break;}
		else return str1[i] < str2[i];
	}
	if(b_uni) return QString::fromStdString(str1).compare(QString::fromStdString(str2)) < 0;
	return str1 < str2;
}
bool item_in_calculator(ExpressionItem *item) {
	if(!CALCULATOR->stillHasVariable((Variable*) item) || !CALCULATOR->stillHasFunction((MathFunction*) item) || !CALCULATOR->stillHasUnit((Unit*) item)) return false;
	if(item->type() == TYPE_VARIABLE) return CALCULATOR->hasVariable((Variable*) item);
	if(item->type() == TYPE_UNIT) return CALCULATOR->hasUnit((Unit*) item);
	if(item->type() == TYPE_FUNCTION) return CALCULATOR->hasFunction((MathFunction*) item);
	return false;
}

AnswerFunction::AnswerFunction() : MathFunction("answer", 1, 1, "", QApplication::tr("History Answer Value").toStdString()) {
	if(QApplication::tr("answer") != "answer") addName(QApplication::tr("answer").toStdString(), 1);
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
			} else if(strcmp(line, "ignore_locale=0\n") == 0) {
				b1 = true; if(b2) break;
			} else if(strlen(line) > 9 && strncmp(line, "language=", 9) == 0) {
				custom_lang = (line + sizeof(char) * 9);
				custom_lang.chop(1);
				b1 = true; if(b2) break;
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
	wayland_platform = qApp->platformName().contains("wayland");
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

void QalculateQtSettings::readPreferenceValue(const std::string &svar, const std::string &svalue, bool is_workspace) {
	int v = s2i(svalue);
	if(svar == "history_expression" || svar == "history_expression*") {
		v_expression.push_back(svalue);
		v_delexpression.push_back(false);
		v_protected.push_back(svar[svar.length() - 1] == '*');
		v_result.push_back(std::vector<std::string>());
		v_exact.push_back(std::vector<int>());
		v_delresult.push_back(std::vector<bool>());
		v_value.push_back(std::vector<size_t>());
	} else if(svar == "history_parse") {
		if(v_expression.size() > v_parse.size()) {
			v_parse.push_back(svalue);
			v_pexact.push_back(true);
		}
	} else if(svar == "history_parse_approximate") {
		if(v_expression.size() > v_parse.size()) {
			v_parse.push_back(svalue);
			v_pexact.push_back(false);
		}
	} else if(svar == "history_result" || svar == "history_result_approximate") {
		if(v_parse.size() < v_expression.size()) {
			v_parse.push_back(v_expression[v_expression.size() - 1]);
			v_expression[v_expression.size() - 1] = "";
			v_pexact.push_back(true);
		}
		if(!v_result.empty()) {
			v_result[v_result.size() - 1].push_back(svalue);
			v_value[v_value.size() - 1].push_back(0);
			v_delresult[v_result.size() - 1].push_back(false);
			if(v_exact[v_exact.size() - 1].size() < v_result[v_result.size() - 1].size()) {
				v_exact[v_exact.size() - 1].push_back(svar.length() < 20);
			}
		}
	} else if(svar == "history_exact") {
		if(!v_exact.empty()) v_exact[settings->v_exact.size() - 1].push_back(v);
	} else if(svar == "expression_history") {
		expression_history.push_back(svalue);
	} else if(!is_workspace && svar == "favourite_function") {
		favourite_functions_pre.push_back(svalue);
		favourite_functions_changed = true;
	} else if(!is_workspace && svar == "favourite_unit") {
		favourite_units_pre.push_back(svalue);
		favourite_units_changed = true;
	} else if(!is_workspace && svar == "favourite_variable") {
		favourite_variables_pre.push_back(svalue);
		favourite_variables_changed = true;
	} else if(!is_workspace && svar == "keyboard_shortcut") {
		default_shortcuts = false;
		char str1[svalue.length()];
		char str2[svalue.length()];
		int ks_type = 0;
		int n = sscanf(svalue.c_str(), "%s %i %[^\n]", str1, &ks_type, str2);
		if(n >= 2 && ks_type >= SHORTCUT_TYPE_FUNCTION && ks_type <= LAST_SHORTCUT_TYPE) {
			keyboard_shortcut *ks = NULL;
			for(size_t i = 0; i < keyboard_shortcuts.size(); i++) {
				if(keyboard_shortcuts[i]->key == str1) {
					ks = keyboard_shortcuts[i];
					break;
				}
			}
			if(!ks) {
				ks = new keyboard_shortcut;
				ks->key = str1;
				ks->new_action = false;
				ks->action = NULL;
				keyboard_shortcuts.push_back(ks);
			}
			if(n == 3) {ks->value.push_back(str2); remove_blank_ends(ks->value[ks->value.size() - 1]);}
			else ks->value.push_back("");
			ks->type.push_back((shortcut_type) ks_type);
		}
	} else if(!is_workspace && svar == "custom_button_label") {
		int c = 0, r = 0;
		char str[svalue.length()];
		int n = sscanf(svalue.c_str(), "%i %i %[^\n]", &r, &c, str);
		if(n == 3 && c > 0 && r > 0) {
			size_t index = 0;
			for(size_t i = custom_buttons.size(); i > 0; i--) {
				if(custom_buttons[i - 1].r == r && custom_buttons[i - 1].c == c) {
					index = i;
					break;
				}
			}
			if(index == 0) {
				custom_button cb;
				cb.c = c; cb.r = r; cb.type[0] = -1; cb.type[1] = -1; cb.type[2] = -1;
				custom_buttons.push_back(cb);
				index = custom_buttons.size();
			}
			index--;
			custom_buttons[index].label = QString::fromUtf8(str).trimmed();
		}
	} else if(!is_workspace && svar == "custom_button") {
		int c = 0, r = 0;
		unsigned int bi = 0;
		char str[svalue.length()];
		int cb_type = -1;
		int n = sscanf(svalue.c_str(), "%i %i %u %i %[^\n]", &c, &r, &bi, &cb_type, str);
		if((n == 4 || n == 5) && bi <= 2 && c > 0 && r > 0) {
			size_t index = 0;
			for(size_t i = custom_buttons.size(); i > 0; i--) {
				if(custom_buttons[i - 1].r == r && custom_buttons[i - 1].c == c) {
					index = i;
					break;
				}
			}
			if(index == 0) {
				custom_button cb;
				cb.c = c; cb.r = r; cb.type[0] = -1; cb.type[1] = -1; cb.type[2] = -1;
				custom_buttons.push_back(cb);
				index = custom_buttons.size();
			}
			index--;
			custom_buttons[index].type[bi] = cb_type;
			if(n == 5) {custom_buttons[index].value[bi] = str; remove_blank_ends(custom_buttons[index].value[bi]);}
			else custom_buttons[index].value[bi] = "";
		}
	} else if(svar == "keypad_type") {
		if(v >= 0 && v <= 3) keypad_type = v;
	} else if(svar == "hide_numpad") {
		hide_numpad = v;
	} else if(svar == "show_keypad") {
		show_keypad = v;
	} else if(svar == "show_bases") {
		show_bases = v;
	} else if(svar == "version") {
		parse_qalculate_version(svalue, preferences_version);
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
	} else if(svar == "previous_precision") {
		previous_precision = v;
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
		if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_CUSTOM) {
			evalops.parse_options.angle_unit = (AngleUnit) v;
		}
	} else if(svar == "custom_angle_unit") {
		custom_angle_unit = svalue;
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
		rounding_mode = (v ? 1 : 0);
		RESET_TZ
	} else if(svar == "rounding_mode") {
		if(v >= 0 && v <= 2) {
			rounding_mode = v;
			RESET_TZ
			printops.round_halfway_to_even = (v == 1);
		}
	} else if(svar == "approximation") {
		if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
			evalops.approximation = (ApproximationMode) v;
			dual_approximation = 0;
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
	} else if(svar == "limit_implicit_multiplication") {
		evalops.parse_options.limit_implicit_multiplication = v;
		printops.limit_implicit_multiplication = v;
	} else if(svar == "parsing_mode") {
		evalops.parse_options.parsing_mode = (ParsingMode) v;
		if(evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) implicit_question_asked = true;
	} else if(svar == "simplified_percentage") {
		simplified_percentage = v;
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
	} else if(!is_workspace) {
		if(svar == "custom_button_rows") {
			if(v > 0 && v <= 100) custom_button_rows = v;
		} else if(svar == "custom_button_columns") {
			if(v > 0 && v <= 100) custom_button_columns = v;
		} else if(svar == "recent_workspace") {
			recent_workspaces.push_back(svalue);
		} else if(svar == "last_workspace") {
			current_workspace = svalue;
		} else if(svar == "save_workspace") {
			save_workspace = v;
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
		} else if(svar == "autocopy_result") {
			autocopy_result = v;
		} else if(svar == "history_expression_type") {
			history_expression_type = v;
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
		} else if(svar == "units_geometry") {
			units_geometry = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "units_vsplitter_state") {
			units_vsplitter_state = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "units_hsplitter_state") {
			units_hsplitter_state = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "variables_geometry") {
			variables_geometry = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "variables_vsplitter_state") {
			variables_vsplitter_state = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "variables_hsplitter_state") {
			variables_hsplitter_state = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "datasets_geometry") {
			datasets_geometry = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "datasets_vsplitter_state") {
			datasets_vsplitter_state = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "datasets_hsplitter_state") {
			datasets_hsplitter_state = QByteArray::fromBase64(svalue.c_str());
		} else if(svar == "style") {
			style = v;
		} else if(svar == "light_style") {
			light_style = v;
		} else if(svar == "palette") {
			palette = v;
		} else if(svar == "color") {
			colorize_result = v;
		} else if(svar == "format") {
			format_result = v;
		} else if(svar == "language") {
			custom_lang = QString::fromStdString(svalue);
		} else if(svar == "ignore_locale") {
			ignore_locale = v;
		} else if(svar == "window_title_mode") {
			if(v >= 0 && v <= 4) title_type = v;
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
		} else if(svar == "multiplication_sign") {
			if(v >= MULTIPLICATION_SIGN_ASTERISK && v <= MULTIPLICATION_SIGN_ALTDOT) {
				printops.multiplication_sign = (MultiplicationSign) v;
			}
		} else if(svar == "division_sign") {
			if(v >= DIVISION_SIGN_SLASH && v <= DIVISION_SIGN_DIVISION) printops.division_sign = (DivisionSign) v;
		} else if(svar == "plot_legend_placement") {
			if(v >= PLOT_LEGEND_NONE && v <= PLOT_LEGEND_OUTSIDE) default_plot_legend_placement = (PlotLegendPlacement) v;
		} else if(svar == "plot_style") {
			if(v >= PLOT_STYLE_LINES && v <= PLOT_STYLE_POLAR) default_plot_style = (PlotStyle) v;
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
		} else if(svar == "plot_complex") {
			default_plot_complex = v;
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
		} else if(svar == "bit_width") {
			default_bits = v;
		} else if(svar == "signed_integer") {
			default_signed = v;
		} else if(svar == "rpn_keys") {
			rpn_keys = v;
		} else if(svar == "rpn_shown") {
			rpn_shown = v;
		} else if(svar == "use_unicode_signs") {
			printops.use_unicode_signs = v;
		} else if(svar == "e_notation") {
			printops.lower_case_e = v;
		} else if(svar == "lower_case_numbers") {
			printops.lower_case_numbers = v;
		} else if(svar == "duodecimal_symbols") {
			use_duo_syms = v;
			RESET_TZ
		} else if(svar == "imaginary_j") {
			do_imaginary_j = v;
		} else if(svar == "base_display") {
			if(v >= BASE_DISPLAY_NONE && v <= BASE_DISPLAY_SUFFIX) printops.base_display = (BaseDisplay) v;
		} else if(svar == "twos_complement") {
			printops.twos_complement = v;
		} else if(svar == "hexadecimal_twos_complement") {
			printops.hexadecimal_twos_complement = v;
		} else if(svar == "spell_out_logical_operators") {
			printops.spell_out_logical_operators = v;
		} else if(svar == "caret_as_xor") {
			caret_as_xor = v;
		} else if(svar == "copy_ascii") {
			copy_ascii = v;
		} else if(svar == "copy_ascii_without_units") {
			copy_ascii_without_units = v;
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
		} else if(svar == "temperature_calculation") {
			CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) v);
			tc_set = true;
		} else if(svar == "sinc_function") {
			if(v == 1) {
				CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "pi");
				sinc_set = true;
			} else if(v == 0) {
				sinc_set = true;
			}
		} else if(svar == "implicit_question_asked") {
			implicit_question_asked = true;
		} else if(svar == "calculate_as_you_type") {
			auto_calculate = v;
		}
	}
}

void QalculateQtSettings::loadPreferences() {

	f_answer = CALCULATOR->addFunction(new AnswerFunction());

	CALCULATOR->setPrecision(10);
	previous_precision = 0;
	CALCULATOR->useIntervalArithmetic(true);
	CALCULATOR->setTemperatureCalculationMode(TEMPERATURE_CALCULATION_HYBRID);
	CALCULATOR->useBinaryPrefixes(0);

	current_workspace = "";
	save_workspace = -1;

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
	rounding_mode = 0;
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
	use_duo_syms = false;
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
	custom_angle_unit = "";
	evalops.parse_options.dot_as_separator = CALCULATOR->default_dot_as_separator;
	evalops.parse_options.comma_as_separator = false;
	evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	evalops.local_currency_conversion = true;
	evalops.interval_calculation = INTERVAL_CALCULATION_VARIANCE_FORMULA;

	title_type = TITLE_APP;
	auto_calculate = true;
	dot_question_asked = false;
	implicit_question_asked = false;
	complex_angle_form = false;
	decimal_comma = -1;
	adaptive_interval_display = true;
	tc_set = false;
	sinc_set = false;
	dual_fraction = -1;
	dual_approximation = -1;
	auto_update_exchange_rates = 7;
	rpn_mode = false;
	rpn_keys = true;
	rpn_shown = false;
	caret_as_xor = false;
	copy_ascii = false;
	copy_ascii_without_units = false;
	do_imaginary_j = false;
	simplified_percentage = true;
	color = 1;
	colorize_result = true;
	format_result = true;
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
	keypad_type = 0;
	show_keypad = -1;
	hide_numpad = false;
	show_bases = -1;
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
	light_style = -1;
	palette = -1;
	replace_expression = KEEP_EXPRESSION;
	autocopy_result = false;
	save_mode_on_exit = true;
	save_defs_on_exit = true;
	clear_history_on_exit = false;
	history_expression_type = 0;
	keep_function_dialog_open = false;
#ifdef _WIN32
	check_version = true;
#else
	check_version = false;
#endif
	last_version_check_date.setToCurrentDate();

	current_workspace = "";
	recent_workspaces.clear();
	v_expression.clear();
	v_parse.clear();
	v_value.clear();
	v_protected.clear();
	v_delexpression.clear();
	v_result.clear();
	v_exact.clear();
	v_pexact.clear();
	v_delresult.clear();
	expression_history.clear();

	default_shortcuts = true;
	for(size_t i = 0; i < keyboard_shortcuts.size(); i++) delete keyboard_shortcuts[i];
	keyboard_shortcuts.clear();
	custom_buttons.clear();
	custom_button_rows = 5;
	custom_button_columns = 4;

	default_signed = -1;
	default_bits = -1;

	favourite_functions_changed = false;
	favourite_variables_changed = false;
	favourite_units_changed = false;

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
	default_plot_complex = -1;
	max_plot_time = 5;

	preferences_version[0] = 4;
	preferences_version[1] = 6;
	preferences_version[2] = 1;

	if(file) {
		char line[1000000L];
		std::string stmp, svalue, svar;
		size_t i;
		while(true) {
			if(fgets(line, 1000000L, file) == NULL) break;
			stmp = line;
			remove_blank_ends(stmp);
			if((i = stmp.find_first_of("=")) != std::string::npos) {
				svar = stmp.substr(0, i);
				remove_blank_ends(svar);
				svalue = stmp.substr(i + 1);
				remove_blank_ends(svalue);
				readPreferenceValue(svar, svalue);
			}
		}
		fclose(file);
		first_time = false;
	} else {
		first_time = true;
	}

	while(v_expression.size() > v_parse.size()) {
		v_parse.push_back("");
		v_pexact.push_back(true);
	}

	keyboard_shortcut *ks;
#define ADD_SHORTCUT(k, t, v) ks = new keyboard_shortcut; ks->key = k; ks->type.push_back(t); ks->value.push_back(v); ks->action = NULL; ks->new_action = false; keyboard_shortcuts.push_back(ks);
	if(default_shortcuts) {
#ifdef _WIN32
		ADD_SHORTCUT("Ctrl+Q", SHORTCUT_TYPE_QUIT, "")
#else
		ADD_SHORTCUT(QKeySequence(QKeySequence::Quit).toString(), SHORTCUT_TYPE_QUIT, "")
#endif
		ADD_SHORTCUT(QKeySequence(QKeySequence::HelpContents).toString(), SHORTCUT_TYPE_HELP, "")
		ADD_SHORTCUT("Ctrl+Alt+C", SHORTCUT_TYPE_COPY_RESULT, "")
		ADD_SHORTCUT(QKeySequence(QKeySequence::Save).toString(), SHORTCUT_TYPE_STORE, "")
		ADD_SHORTCUT("Ctrl+M", SHORTCUT_TYPE_MANAGE_VARIABLES, "")
		ADD_SHORTCUT("Ctrl+F", SHORTCUT_TYPE_MANAGE_FUNCTIONS, "")
		ADD_SHORTCUT("Ctrl+U", SHORTCUT_TYPE_MANAGE_UNITS, "")
		ADD_SHORTCUT("Ctrl+P", SHORTCUT_TYPE_PLOT, "")
		ADD_SHORTCUT("Ctrl+R", SHORTCUT_TYPE_RPN_MODE, "")
		ADD_SHORTCUT("Ctrl+K", SHORTCUT_TYPE_KEYPAD, "")
		ADD_SHORTCUT("Alt+K", SHORTCUT_TYPE_KEYPAD, "")
		ADD_SHORTCUT("Ctrl+T", SHORTCUT_TYPE_CONVERT, "")
		ADD_SHORTCUT("Ctrl+B", SHORTCUT_TYPE_NUMBER_BASES, "")
		ADD_SHORTCUT("Ctrl+-", SHORTCUT_TYPE_NEGATE, "")
		ADD_SHORTCUT("Ctrl+Enter", SHORTCUT_TYPE_APPROXIMATE, "")
		ADD_SHORTCUT("Ctrl+Return", SHORTCUT_TYPE_APPROXIMATE, "")
		ADD_SHORTCUT("Alt+M", SHORTCUT_TYPE_MODE, "")
		ADD_SHORTCUT("F10", SHORTCUT_TYPE_MENU, "")
		ADD_SHORTCUT("Ctrl+)", SHORTCUT_TYPE_SMART_PARENTHESES, "")
		ADD_SHORTCUT("Ctrl+(", SHORTCUT_TYPE_SMART_PARENTHESES, "")
		ADD_SHORTCUT("Ctrl+Up", SHORTCUT_TYPE_RPN_UP, "")
		ADD_SHORTCUT("Ctrl+Down", SHORTCUT_TYPE_RPN_DOWN, "")
		ADD_SHORTCUT("Ctrl+Right", SHORTCUT_TYPE_RPN_SWAP, "")
		ADD_SHORTCUT("Ctrl+Left", SHORTCUT_TYPE_RPN_LASTX, "")
		ADD_SHORTCUT("Ctrl+Shift+C", SHORTCUT_TYPE_RPN_COPY, "")
		ADD_SHORTCUT("Ctrl+Delete", SHORTCUT_TYPE_RPN_DELETE, "")
		ADD_SHORTCUT("Ctrl+Shift+Delete", SHORTCUT_TYPE_RPN_CLEAR, "")
		ADD_SHORTCUT("Tab", SHORTCUT_TYPE_COMPLETE, "")
	} else if(PREFERENCES_VERSION_BEFORE(4, 1, 2)) {
		ADD_SHORTCUT("Tab", SHORTCUT_TYPE_COMPLETE, "")
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

	if(palette != 1) light_style = style;
	if(style >= 0) updateStyle();
	else if(palette >= 0) updatePalette();

	if(!current_workspace.empty()) {
		if(!loadWorkspace(current_workspace.c_str())) current_workspace = "";
	}

}
void QalculateQtSettings::setCustomAngleUnit() {
	if(!custom_angle_unit.empty()) {
		CALCULATOR->setCustomAngleUnit(CALCULATOR->getActiveUnit(custom_angle_unit));
		if(CALCULATOR->customAngleUnit()) custom_angle_unit = CALCULATOR->customAngleUnit()->referenceName();
	}
	if(evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()) evalops.parse_options.angle_unit = ANGLE_UNIT_NONE;
}
void QalculateQtSettings::updateFavourites() {
	if(favourite_functions_pre.empty()) {
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			if(CALCULATOR->functions[i]->isLocal() && !CALCULATOR->functions[i]->isHidden()) favourite_functions.push_back(CALCULATOR->functions[i]);
		}
	} else {
		for(size_t i = 0; i < favourite_functions_pre.size(); i++) {
			MathFunction *f = CALCULATOR->getActiveFunction(favourite_functions_pre[i]);
			if(f) favourite_functions.push_back(f);
		}
	}
	if(favourite_units_pre.empty()) {
		const char *si_units[] = {"m", "kg_c", "s", "A", "K", "mol", "cd"};
		for(size_t i = 0; i < 7; i++) {
			Unit *u = CALCULATOR->getActiveUnit(si_units[i]);
			if(!u) u = CALCULATOR->getCompositeUnit(si_units[i]);
			if(u) favourite_units.push_back(u);
		}
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i]->isLocal() && !CALCULATOR->units[i]->isHidden()) favourite_units.push_back(CALCULATOR->units[i]);
		}
	} else {
		for(size_t i = 0; i < favourite_units_pre.size(); i++) {
			Unit *u = CALCULATOR->getActiveUnit(favourite_units_pre[i]);
			if(!u) u = CALCULATOR->getCompositeUnit(favourite_units_pre[i]);
			if(u) favourite_units.push_back(u);
		}
	}
	if(favourite_variables_pre.empty()) {
		favourite_variables.push_back(CALCULATOR->getVariableById(VARIABLE_ID_PI));
		favourite_variables.push_back(CALCULATOR->getVariableById(VARIABLE_ID_E));
		favourite_variables.push_back(CALCULATOR->getVariableById(VARIABLE_ID_EULER));
		Variable *v = CALCULATOR->getActiveVariable("golden");
		if(v) favourite_variables.push_back(v);
		for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			if(CALCULATOR->variables[i]->isLocal() && !CALCULATOR->variables[i]->isHidden()) favourite_variables.push_back(CALCULATOR->variables[i]);
		}
	} else {
		for(size_t i = 0; i < favourite_variables_pre.size(); i++) {
			Variable *v = CALCULATOR->getActiveVariable(favourite_variables_pre[i]);
			if(v) favourite_variables.push_back(v);
		}
	}
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
		if(s) QApplication::setStyle(s);
	}
#ifdef _WIN32
	else {
		QStyle *s = QStyleFactory::create("windowsvista");
		if(s) QApplication::setStyle(s);
	}
#endif
	updatePalette();
}

void QalculateQtSettings::savePreferences(bool save_mode) {
	std::string homedir = getLocalDir();
	recursiveMakeDir(homedir);
	std::string filename = buildPath(homedir, "qalculate-qt.cfg");
	if(!savePreferences(filename.c_str(), false, save_mode)) {
		QMessageBox::critical(NULL, QApplication::tr("Error"), QApplication::tr("Couldn't write preferences to\n%1").arg(QString::fromStdString(filename)), QMessageBox::Ok);
	}
}
bool QalculateQtSettings::savePreferences(const char *filename, bool is_workspace, bool) {
	std::string shistory, smode, sgeneral;
	bool read_default = !is_workspace && !current_workspace.empty();
	if(read_default) {
		FILE *file = fopen(filename, "r");
		if(!file) return false;
		char line[1000000L];
		std::string stmp, svalue, svar;
		size_t i;
		bool b_mode = false, b_history = false;
		while(true) {
			if(fgets(line, 1000000L, file) == NULL) break;
			stmp = line;
			remove_blank_ends(stmp);
			if(!stmp.empty()) {
				if(stmp[0] == '[') {
					b_mode = false;
					b_history = false;
				} else if(b_mode) {
					smode += stmp;
					smode += "\n";
				} else if(b_history) {
					shistory += stmp;
					shistory += "\n";
				}
				if(!b_mode && !b_history) {
					if(stmp == "[Mode]") {
						b_mode = true;
					} else if(!clear_history_on_exit && stmp == "[History]") {
						b_history = true;
					} else if((i = stmp.find_first_of("=")) != std::string::npos) {
						if(stmp.substr(0, i) == "keypad_type" || stmp.substr(0, i) == "hide_numpad" || stmp.substr(0, i) == "show_keypad" || stmp.substr(0, i) == "show_bases") {
							sgeneral += stmp;
							sgeneral += "\n";
						}
					}
				}
			}
		}
		fclose(file);
	}
	FILE *file = fopen(filename, "w+");
	if(!file) return false;
	if(is_workspace) fputs("QALCULATE WORKSPACE FILE\n", file);
	fprintf(file, "\n[General]\n");
	fprintf(file, "version=%s\n", qApp->applicationVersion().toUtf8().data());
	if(!is_workspace) {
		fprintf(file, "allow_multiple_instances=%i\n", allow_multiple_instances);
		if(!custom_lang.isEmpty()) fprintf(file, "language=%s\n", custom_lang.toUtf8().data());
		fprintf(file, "ignore_locale=%i\n", ignore_locale);
		fprintf(file, "check_version=%i\n", check_version);
		if(check_version) {
			fprintf(file, "last_version_check=%s\n", last_version_check_date.toISOString().c_str());
			if(!last_found_version.empty()) fprintf(file, "last_found_version=%s\n", last_found_version.c_str());
		}
		if(!window_state.isEmpty()) fprintf(file, "window_state=%s\n", window_state.toBase64().data());
		if(!window_geometry.isEmpty()) fprintf(file, "window_geometry=%s\n", window_geometry.toBase64().data());
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
		if(!datasets_geometry.isEmpty()) fprintf(file, "datasets_geometry=%s\n", datasets_geometry.toBase64().data());
		if(!datasets_vsplitter_state.isEmpty()) fprintf(file, "datasets_vsplitter_state=%s\n", datasets_vsplitter_state.toBase64().data());
		if(!datasets_hsplitter_state.isEmpty()) fprintf(file, "datasets_hsplitter_state=%s\n", datasets_hsplitter_state.toBase64().data());
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
#ifdef _WIN32
		if(light_style != style && palette == 1) fprintf(file, "light_style=%i\n", light_style);
#endif
		fprintf(file, "palette=%i\n", palette);
		fprintf(file, "color=%i\n", colorize_result);
		if(!format_result) fprintf(file, "format=%i\n", format_result);
		fprintf(file, "use_custom_result_font=%i\n", use_custom_result_font);
		fprintf(file, "use_custom_expression_font=%i\n", use_custom_expression_font);
		fprintf(file, "use_custom_keypad_font=%i\n", use_custom_keypad_font);
		fprintf(file, "use_custom_application_font=%i\n", use_custom_app_font);
		if(use_custom_result_font || save_custom_result_font) fprintf(file, "custom_result_font=%s\n", custom_result_font.c_str());
		if(use_custom_expression_font || save_custom_expression_font) fprintf(file, "custom_expression_font=%s\n", custom_expression_font.c_str());
		if(use_custom_keypad_font || save_custom_keypad_font) fprintf(file, "custom_keypad_font=%s\n", custom_keypad_font.c_str());
		if(use_custom_app_font || save_custom_app_font) fprintf(file, "custom_application_font=%s\n", custom_app_font.c_str());
		if(printops.multiplication_sign != MULTIPLICATION_SIGN_X) fprintf(file, "multiplication_sign=%i\n", printops.multiplication_sign);
		if(printops.division_sign != DIVISION_SIGN_DIVISION_SLASH) fprintf(file, "division_sign=%i\n", printops.division_sign);
		if(implicit_question_asked) fprintf(file, "implicit_question_asked=%i\n", implicit_question_asked);
		fprintf(file, "replace_expression=%i\n", replace_expression);
		fprintf(file, "autocopy_result=%i\n", autocopy_result);
		fprintf(file, "history_expression_type=%i\n", history_expression_type);
	}
	if(read_default) {
		fputs(sgeneral.c_str(), file);
	} else {
		fprintf(file, "keypad_type=%i\n", keypad_type);
		if(show_keypad >= 0) fprintf(file, "show_keypad=%i\n", show_keypad);
		fprintf(file, "hide_numpad=%i\n", hide_numpad);
		fprintf(file, "show_bases=%i\n", show_bases);
	}
	if(!is_workspace) {
		fprintf(file, "rpn_keys=%i\n", rpn_keys);
		if(!current_workspace.empty()) fprintf(file, "last_workspace=%s\n", current_workspace.c_str());
		if(save_workspace >= 0) fprintf(file, "save_workspace=%i\n", save_workspace);
		for(size_t i = 0; i < recent_workspaces.size(); i++) {
			fprintf(file, "recent_workspace=%s\n", recent_workspaces[i].c_str());
		}
		if(!custom_buttons.empty()) {
			fprintf(file, "custom_button_columns=%i\n", custom_button_columns);
			fprintf(file, "custom_button_rows=%i\n", custom_button_rows);
			for(unsigned int i = 0; i < custom_buttons.size(); i++) {
				if(!custom_buttons[i].label.isEmpty()) fprintf(file, "custom_button_label=%i %i %s\n", custom_buttons[i].r, custom_buttons[i].c, custom_buttons[i].label.toUtf8().data());
				for(unsigned int bi = 0; bi <= 2; bi++) {
					if(custom_buttons[i].type[bi] != -1) {
						if(custom_buttons[i].value[bi].empty()) fprintf(file, "custom_button=%i %i %u %i\n", custom_buttons[i].c, custom_buttons[i].r, bi, custom_buttons[i].type[bi]);
						else fprintf(file, "custom_button=%i %i %u %i %s\n", custom_buttons[i].c, custom_buttons[i].r, bi, custom_buttons[i].type[bi], custom_buttons[i].value[bi].c_str());
					}
				}
			}
		}
		if(!default_shortcuts) {
			for(size_t i = 0; i < keyboard_shortcuts.size(); i++) {
				for(size_t i2 = 0; i2 < keyboard_shortcuts[i]->type.size(); i2++) {
					if(keyboard_shortcuts[i]->value[i2].empty()) fprintf(file, "keyboard_shortcut=%s %i\n", keyboard_shortcuts[i]->key.toUtf8().data(), keyboard_shortcuts[i]->type[i2]);
					else fprintf(file, "keyboard_shortcut=%s %i %s\n", keyboard_shortcuts[i]->key.toUtf8().data(), keyboard_shortcuts[i]->type[i2], keyboard_shortcuts[i]->value[i2].c_str());
				}
			}
		}
		if(favourite_functions_changed) {
			for(size_t i = 0; i < favourite_functions.size(); i++) {
				if(CALCULATOR->stillHasFunction(favourite_functions[i]) && favourite_functions[i]->isActive() && favourite_functions[i]->category() != CALCULATOR->temporaryCategory()) {
					fprintf(file, "favourite_function=%s\n", favourite_functions[i]->referenceName().c_str());
				}
			}
		}
		if(favourite_units_changed) {
			for(size_t i = 0; i < favourite_units.size(); i++) {
				if(CALCULATOR->stillHasUnit(favourite_units[i]) && favourite_units[i]->isActive() && favourite_units[i]->category() != CALCULATOR->temporaryCategory()) {
					fprintf(file, "favourite_unit=%s\n", favourite_units[i]->referenceName().c_str());
				}
			}
		}
		if(favourite_variables_changed) {
			for(size_t i = 0; i < favourite_variables.size(); i++) {
				if(CALCULATOR->stillHasVariable(favourite_variables[i]) && favourite_variables[i]->isActive() && favourite_variables[i]->category() != CALCULATOR->temporaryCategory()) {
					fprintf(file, "favourite_variable=%s\n", favourite_variables[i]->referenceName().c_str());
				}
			}
		}
		if(default_bits >= 0) fprintf(file, "bit_width=%i\n", default_bits);
		if(default_signed >= 0) fprintf(file, "signed_integer=%i\n", default_signed);
		fprintf(file, "spell_out_logical_operators=%i\n", printops.spell_out_logical_operators);
		fprintf(file, "caret_as_xor=%i\n", caret_as_xor);
		fprintf(file, "copy_ascii=%i\n", copy_ascii);
		fprintf(file, "copy_ascii_without_units=%i\n", copy_ascii_without_units);
		fprintf(file, "digit_grouping=%i\n", printops.digit_grouping);
		fprintf(file, "decimal_comma=%i\n", decimal_comma);
		fprintf(file, "dot_as_separator=%i\n", dot_question_asked ? evalops.parse_options.dot_as_separator : -1);
		fprintf(file, "comma_as_separator=%i\n", evalops.parse_options.comma_as_separator);
		fprintf(file, "twos_complement=%i\n", printops.twos_complement);
		fprintf(file, "hexadecimal_twos_complement=%i\n", printops.hexadecimal_twos_complement);
		fprintf(file, "use_unicode_signs=%i\n", printops.use_unicode_signs);
		fprintf(file, "lower_case_numbers=%i\n", printops.lower_case_numbers);
		fprintf(file, "duodecimal_symbols=%i\n", use_duo_syms);
		fprintf(file, "e_notation=%i\n", printops.lower_case_e);
		fprintf(file, "imaginary_j=%i\n", CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0);
		fprintf(file, "base_display=%i\n", printops.base_display);
		if(tc_set) fprintf(file, "temperature_calculation=%i\n", CALCULATOR->getTemperatureCalculationMode());
		if(sinc_set) fprintf(file, "sinc_function=%i\n", CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->getDefaultValue(2) == "pi" ? 1 : 0);
		fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
		fprintf(file, "local_currency_conversion=%i\n", evalops.local_currency_conversion);
		fprintf(file, "use_binary_prefixes=%i\n", CALCULATOR->usesBinaryPrefixes());
		fprintf(file, "calculate_as_you_type=%i\n", auto_calculate);
		if(previous_precision > 0) fprintf(file, "previous_precision=%i\n", previous_precision);
	}
	if(read_default) {
		fprintf(file, "\n[Mode]\n");
		fputs(smode.c_str(), file);
	} else {
		fprintf(file, "\n[Mode]\n");
		fprintf(file, "min_deci=%i\n", printops.min_decimals);
		fprintf(file, "use_min_deci=%i\n", printops.use_min_decimals);
		fprintf(file, "max_deci=%i\n", printops.max_decimals);
		fprintf(file, "use_max_deci=%i\n", printops.use_max_decimals);
		fprintf(file, "precision=%i\n", CALCULATOR->getPrecision());
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
		fprintf(file, "prefixes_default=%i\n", prefixes_default);
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
		if(evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && CALCULATOR->customAngleUnit()) fprintf(file, "custom_angle_unit=%s\n", CALCULATOR->customAngleUnit()->referenceName().c_str());
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
		fprintf(file, "rounding_mode=%i\n", rounding_mode);
		if(dual_approximation < 0) fprintf(file, "approximation=%i\n", -1);
		else if(dual_approximation > 0) fprintf(file, "approximation=%i\n", APPROXIMATION_APPROXIMATE + 1);
		else fprintf(file, "approximation=%i\n", evalops.approximation);
		fprintf(file, "interval_calculation=%i\n", evalops.interval_calculation);
		fprintf(file, "rpn_mode=%i\n", rpn_mode);
		fprintf(file, "chain_mode=%i\n", chain_mode);
		fprintf(file, "limit_implicit_multiplication=%i\n", evalops.parse_options.limit_implicit_multiplication);
		fprintf(file, "parsing_mode=%i\n", evalops.parse_options.parsing_mode);
		fprintf(file, "simplified_percentage=%i\n", simplified_percentage);
		fprintf(file, "spacious=%i\n", printops.spacious);
		fprintf(file, "excessive_parenthesis=%i\n", printops.excessive_parenthesis);
		fprintf(file, "default_assumption_type=%i\n", CALCULATOR->defaultAssumptions()->type());
		if(CALCULATOR->defaultAssumptions()->type() != ASSUMPTION_TYPE_BOOLEAN) fprintf(file, "default_assumption_sign=%i\n", CALCULATOR->defaultAssumptions()->sign());
	}
	if(!is_workspace) {
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
		if(default_plot_complex >= 0) fprintf(file, "plot_complex=%i\n", default_plot_complex);
		fprintf(file, "plot_variable=%s\n", default_plot_variable.c_str());
		fprintf(file, "plot_rows=%i\n", default_plot_rows);
		fprintf(file, "plot_type=%i\n", default_plot_type);
		fprintf(file, "plot_color=%i\n", default_plot_color);
		fprintf(file, "plot_linewidth=%i\n", default_plot_linewidth);
		if(max_plot_time != 5) fprintf(file, "max_plot_time=%i\n", max_plot_time);
	}
	if(read_default) {
		fprintf(file, "\n[History]\n");
		fputs(shistory.c_str(), file);
	} else if(!clear_history_on_exit || is_workspace) {
		fprintf(file, "\n[History]\n");
		if(!v_expression.empty()) {
			long int n = 0;
			size_t i = v_expression.size() - 1;
			while(true) {
				n++;
				for(size_t i2 = 0; i2 < v_result[i].size(); i2++) {
					if(v_result[i][i2].length() > 300 && (v_result[i][i2].length() <= 6000 || unhtmlize(v_result[i][i2]).length() <= 5000)) {
						n += v_result[i][i2].length() / 300;
					}
					n++;
				}
				if(n >= 300 || i == 0) break;
				i--;
			}
			size_t i_first = i;
			for(i = 0; i < v_expression.size(); i++) {
				if((i >= i_first || v_protected[i]) && !v_delexpression[i]) {
					if(v_expression[i].empty()) {
						if(v_protected[i]) fprintf(file, "history_expression*=%s\n", v_parse[i].c_str());
						else fprintf(file, "history_expression=%s\n", v_parse[i].c_str());
					} else {
						if(v_protected[i]) fprintf(file, "history_expression*=%s\n", v_expression[i].c_str());
						else fprintf(file, "history_expression=%s\n", v_expression[i].c_str());
						if(!v_parse[i].empty()) {
							if(v_pexact[i]) fprintf(file, "history_parse=%s\n", v_parse[i].c_str());
							else fprintf(file, "history_parse_approximate=%s\n", v_parse[i].c_str());
						}
					}
					n++;
					for(size_t i2 = 0; i2 < v_result[i].size(); i2++) {
						if(!v_delresult[i][i2]) {
							if(v_exact[i][i2]) fprintf(file, "history_result");
							else fprintf(file, "history_result_approximate");
							if(v_result[i][i2].length() > 6000 && !v_protected[i]) {
								std::string str = unhtmlize(v_result[i][i2]);
								if(str.length() > 5000) {
									int index = 50;
									while(index >= 0 && (signed char) str[index] < 0 && (unsigned char) str[index + 1] < 0xC0) index--;
									gsub("\n", "<br>", str);
									fprintf(file,  "=%s …\n", str.substr(0, index + 1).c_str());
								} else {
									fprintf(file, "=%s\n", v_result[i][i2].c_str());
								}
							} else {
								fprintf(file, "=%s\n", v_result[i][i2].c_str());
							}
						}
					}
				}
			}
		}
		for(size_t i = 0; i < expression_history.size(); i++) {
			gsub("\n", " ", expression_history[i]);
			fprintf(file, "expression_history=%s\n", expression_history[i].c_str());
		}
	}
	if(is_workspace) {
		fprintf(file, "\n[Definitions]\n");
		fputs(CALCULATOR->saveTemporaryDefinitions().c_str(), file);
	}
	fclose(file);
	return true;
}

const char *QalculateQtSettings::multiplicationSign(bool units) {
	if(!printops.use_unicode_signs) return "*";
	switch(printops.multiplication_sign) {
		case MULTIPLICATION_SIGN_X: {if(units) {return SIGN_MIDDLEDOT;} return SIGN_MULTIPLICATION;}
		case MULTIPLICATION_SIGN_DOT: {return SIGN_MULTIDOT;}
		case MULTIPLICATION_SIGN_ALTDOT: {return SIGN_MIDDLEDOT;}
		default: {return "*";}
	}
}
const char *QalculateQtSettings::divisionSign(bool output) {
	if(printops.division_sign == DIVISION_SIGN_DIVISION && printops.use_unicode_signs) return SIGN_DIVISION;
	else if(output && printops.division_sign == DIVISION_SIGN_DIVISION_SLASH && printops.use_unicode_signs) return SIGN_DIVISION_SLASH;
	return "/";
}

std::string QalculateQtSettings::localizeExpression(std::string str, bool unit_expression) {
	ParseOptions pa = evalops.parse_options; pa.base = 10;
	str = CALCULATOR->localizeExpression(str, pa);
	gsub("*", multiplicationSign(unit_expression), str);
	gsub("/", divisionSign(false), str);
	gsub("-", SIGN_MINUS, str);
	return str;
}

std::string QalculateQtSettings::unlocalizeExpression(std::string str) {
	ParseOptions pa = evalops.parse_options; pa.base = 10;
	str = CALCULATOR->unlocalizeExpression(str, pa);
	CALCULATOR->parseSigns(str);
	return str;
}

void QalculateQtSettings::updateMessagePrintOptions() {
	PrintOptions message_printoptions = printops;
	message_printoptions.is_approximate = NULL;
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

MathLineEdit::MathLineEdit(QWidget *parent, bool unit_expression, bool function_expression) : QLineEdit(parent), b_unit(unit_expression), b_function(function_expression) {
#ifndef _WIN32
	setAttribute(Qt::WA_InputMethodEnabled, settings->enable_input_method);
#endif
}
MathLineEdit::~MathLineEdit() {}
void MathLineEdit::keyPressEvent(QKeyEvent *event) {
	if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::GroupSwitchModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
		switch(event->key()) {
			case Qt::Key_Asterisk: {
				insert(settings->multiplicationSign(b_unit));
				return;
			}
			case Qt::Key_Slash: {
				insert(settings->divisionSign(false));
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
			case Qt::Key_BraceLeft: {
				if(!b_function) {return;}
				break;
			}
			case Qt::Key_BraceRight: {
				if(!b_function) {return;}
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
			dialog->setWindowTitle(tr("Fetching exchange rates…"));
			dialog->setWindowModality(Qt::WindowModal);
			dialog->setMinimumDuration(200);
			while(fetch_thread.running) {
				qApp->processEvents();
				sleep_ms(10);
			}
			dialog->cancel();
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
		if(CALCULATOR->message()->category() != MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION || !implicit_question_asked) {
			if(index > 0) {
				if(index == 1) str = "• " + str;
					str += "\n• ";
			}
			str += CALCULATOR->message()->message();
			if(mtype == MESSAGE_ERROR || (mtype_highest != MESSAGE_ERROR && mtype == MESSAGE_WARNING)) {
				mtype_highest = mtype;
			}
			index++;
		}
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
	script += "\tif curl -L -o qalculate-${new_version}-x86_64.tar.xz https://github.com/Qalculate/qalculate-qt/releases/download/v${new_version}/qalculate-${new_version}-x86_64.tar.xz; then\n";
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
	int ret = checkAvailableVersion("windows", qApp->applicationVersion().toUtf8().data(), &new_version, force ? 10 : 5);
#else
	int ret = checkAvailableVersion("qalculate-qt", qApp->applicationVersion().toUtf8().data(), &new_version, force ? 10 : 5);
#endif
	if(force && ret <= 0) {
		if(ret < 0) QMessageBox::critical(parent, tr("Error"), tr("Failed to check for updates."));
		else QMessageBox::information(parent, tr("Information"), tr("No updates found."));
		if(ret < 0) return;
	}
	if(ret > 0 && (force || new_version != last_found_version)) {
		last_found_version = new_version;
#ifdef AUTO_UPDATE
		if(QMessageBox::question(parent, tr("Information"), "<div>" + tr("A new version of %1 is available at %2.\n\nDo you wish to update to version %3?").arg("Qalculate!").arg("<a href=\"https://qalculate.github.io/downloads.html\">qalculate.github.io</a>").arg(QString::fromStdString(new_version)) == QMessageBox::Yes) + "</div>") {
			autoUpdate(new_version);
		}
#else
		QMessageBox::information(parent, tr("Information"), "<div>" + tr("A new version of %1 is available.\n\nYou can get version %3 at %2.").arg("Qalculate!").arg("<a href=\"https://qalculate.github.io/downloads.html\">qalculate.github.io</a>").arg(QString::fromStdString(new_version)) + "</div>");
#endif
	}
	last_version_check_date.setToCurrentDate();
}

QString QalculateQtSettings::shortcutText(const std::vector<shortcut_type> &type, const std::vector<std::string> &value) {
	if(type.size() == 1) return shortcutText(type[0], value[0]);
	QString str;
	for(size_t i = 0; i < type.size(); i++) {
		if(!str.isEmpty()) str += ", ";
		str += shortcutText(type[i], value[i]);
	}
	return str;
}
QString QalculateQtSettings::shortcutText(int type, const std::string &value) {
	if(type < 0) return QString();
	switch(type) {
		case SHORTCUT_TYPE_FUNCTION: {
			MathFunction *f = CALCULATOR->getActiveFunction(value);
			return QString::fromStdString(f->title(true, printops.use_unicode_signs));
		}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
			MathFunction *f = CALCULATOR->getActiveFunction(value);
			return QString::fromStdString(f->title(true, printops.use_unicode_signs));
		}
		case SHORTCUT_TYPE_VARIABLE: {
			Variable *v = CALCULATOR->getActiveVariable(value);
			return QString::fromStdString(v->title(true, printops.use_unicode_signs));
		}
		case SHORTCUT_TYPE_UNIT: {
			Unit *u = CALCULATOR->getActiveUnit(value);
			return QString::fromStdString(u->title(true, printops.use_unicode_signs));
		}
		default: {}
	}
	if(value.empty()) return shortcutTypeText((shortcut_type) type);
	return tr("%1: %2").arg(shortcutTypeText((shortcut_type) type)).arg(QString::fromStdString(value));
}
QString QalculateQtSettings::shortcutTypeText(shortcut_type type) {
	switch(type) {
		case SHORTCUT_TYPE_FUNCTION: {return tr("Insert function");}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {return tr("Insert function (dialog)");}
		case SHORTCUT_TYPE_VARIABLE: {return tr("Insert variable");}
		case SHORTCUT_TYPE_APPROXIMATE: {return tr("Approximate result");}
		case SHORTCUT_TYPE_NEGATE: {return tr("Negate");}
		case SHORTCUT_TYPE_INVERT: {return tr("Invert");}
		case SHORTCUT_TYPE_UNIT: {return tr("Insert unit");}
		case SHORTCUT_TYPE_TEXT: {return tr("Insert text");}
		case SHORTCUT_TYPE_OPERATOR: {return tr("Insert operator");}
		case SHORTCUT_TYPE_DATE: {return tr("Insert date");}
		case SHORTCUT_TYPE_MATRIX: {return tr("Insert matrix");}
		case SHORTCUT_TYPE_SMART_PARENTHESES: {return tr("Insert smart parentheses");}
		case SHORTCUT_TYPE_CONVERT_TO: {return tr("Convert to unit");}
		case SHORTCUT_TYPE_CONVERT: {return tr("Convert");}
		case SHORTCUT_TYPE_OPTIMAL_UNIT: {return tr("Convert to optimal unit");}
		case SHORTCUT_TYPE_BASE_UNITS: {return tr("Convert to base units");}
		case SHORTCUT_TYPE_OPTIMAL_PREFIX: {return tr("Convert to optimal prefix");}
		case SHORTCUT_TYPE_TO_NUMBER_BASE: {return tr("Convert to number base");}
		case SHORTCUT_TYPE_FACTORIZE: {return tr("Factorize result");}
		case SHORTCUT_TYPE_EXPAND: {return tr("Expand result");}
		case SHORTCUT_TYPE_PARTIAL_FRACTIONS: {return tr("Expand partial fractions");}
		case SHORTCUT_TYPE_RPN_DOWN: {return tr("RPN: down");}
		case SHORTCUT_TYPE_RPN_UP: {return tr("RPN: up");}
		case SHORTCUT_TYPE_RPN_SWAP: {return tr("RPN: swap");}
		case SHORTCUT_TYPE_RPN_COPY: {return tr("RPN: copy");}
		case SHORTCUT_TYPE_RPN_LASTX: {return tr("RPN: lastx");}
		case SHORTCUT_TYPE_RPN_DELETE: {return tr("RPN: delete register");}
		case SHORTCUT_TYPE_RPN_CLEAR: {return tr("RPN: clear stack");}
		case SHORTCUT_TYPE_INPUT_BASE: {return tr("Set expression base");}
		case SHORTCUT_TYPE_OUTPUT_BASE: {return tr("Set result base");}
		case SHORTCUT_TYPE_DEGREES: {return tr("Set angle unit to degrees");}
		case SHORTCUT_TYPE_RADIANS: {return tr("Set angle unit to radians");}
		case SHORTCUT_TYPE_GRADIANS: {return tr("Set angle unit to gradians");}
		case SHORTCUT_TYPE_NORMAL_NOTATION: {return tr("Active normal display mode");}
		case SHORTCUT_TYPE_SCIENTIFIC_NOTATION: {return tr("Activate scientific display mode");}
		case SHORTCUT_TYPE_ENGINEERING_NOTATION: {return tr("Activate engineering display mode");}
		case SHORTCUT_TYPE_SIMPLE_NOTATION: {return tr("Activate simple display mode");}
		case SHORTCUT_TYPE_PRECISION: {return tr("Toggle precision");}
		case SHORTCUT_TYPE_MAX_DECIMALS: {return tr("Toggle max decimals");}
		case SHORTCUT_TYPE_MIN_DECIMALS: {return tr("Toggle min decimals");}
		case SHORTCUT_TYPE_MINMAX_DECIMALS: {return tr("Toggle max/min decimals");}
		case SHORTCUT_TYPE_RPN_MODE: {return tr("Toggle RPN mode");}
		case SHORTCUT_TYPE_GENERAL_KEYPAD: {return tr("Show general keypad");}
		case SHORTCUT_TYPE_PROGRAMMING_KEYPAD: {return tr("Toggle programming keypad");}
		case SHORTCUT_TYPE_ALGEBRA_KEYPAD: {return tr("Toggle algebra keypad");}
		case SHORTCUT_TYPE_CUSTOM_KEYPAD: {return tr("Toggle custom keypad");}
		case SHORTCUT_TYPE_KEYPAD: {return tr("Show/hide keypad");}
		case SHORTCUT_TYPE_HISTORY_SEARCH: {return tr("Search history");}
		case SHORTCUT_TYPE_HISTORY_CLEAR: {return tr("Clear history");}
		case SHORTCUT_TYPE_MANAGE_VARIABLES: {return tr("Show variables");}
		case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {return tr("Show functions");}
		case SHORTCUT_TYPE_MANAGE_UNITS: {return tr("Show units");}
		case SHORTCUT_TYPE_MANAGE_DATA_SETS: {return tr("Show data sets");}
		case SHORTCUT_TYPE_STORE: {return tr("Store result");}
		case SHORTCUT_TYPE_MEMORY_CLEAR: {return tr("MC (memory clear)");}
		case SHORTCUT_TYPE_MEMORY_RECALL: {return tr("MR (memory recall)");}
		case SHORTCUT_TYPE_MEMORY_STORE: {return tr("MS (memory store)");}
		case SHORTCUT_TYPE_MEMORY_ADD: {return tr("M+ (memory plus)");}
		case SHORTCUT_TYPE_MEMORY_SUBTRACT: {return tr("M− (memory minus)");}
		case SHORTCUT_TYPE_NEW_VARIABLE: {return tr("New variable");}
		case SHORTCUT_TYPE_NEW_FUNCTION: {return tr("New function");}
		case SHORTCUT_TYPE_PLOT: {return tr("Open plot functions/data");}
		case SHORTCUT_TYPE_NUMBER_BASES: {return tr("Open convert number bases");}
		case SHORTCUT_TYPE_FLOATING_POINT: {return tr("Open floating point conversion");}
		case SHORTCUT_TYPE_CALENDARS: {return tr("Open calender conversion");}
		case SHORTCUT_TYPE_PERCENTAGE_TOOL: {return tr("Open percentage calculation tool");}
		case SHORTCUT_TYPE_PERIODIC_TABLE: {return tr("Open periodic table");}
		case SHORTCUT_TYPE_UPDATE_EXRATES: {return tr("Update exchange rates");}
		case SHORTCUT_TYPE_COPY_RESULT: {return tr("Copy result");}
		case SHORTCUT_TYPE_INSERT_RESULT: {return tr("Insert result");}
		case SHORTCUT_TYPE_MODE: {return tr("Open mode menu");}
		case SHORTCUT_TYPE_MENU: {return tr("Open menu");}
		case SHORTCUT_TYPE_HELP: {return tr("Help");}
		case SHORTCUT_TYPE_QUIT: {return tr("Quit");}
		case SHORTCUT_TYPE_CHAIN_MODE: {return tr("Toggle chain mode");}
		case SHORTCUT_TYPE_ALWAYS_ON_TOP: {return tr("Toggle keep above");}
		case SHORTCUT_TYPE_COMPLETE: {return tr("Show completion (activate first item)");}
		case SHORTCUT_TYPE_EXPRESSION_CLEAR: {return tr("Clear expression");}
		case SHORTCUT_TYPE_EXPRESSION_DELETE: {return tr("Delete");}
		case SHORTCUT_TYPE_EXPRESSION_BACKSPACE: {return tr("Backspace");}
		case SHORTCUT_TYPE_EXPRESSION_START: {return tr("Home");}
		case SHORTCUT_TYPE_EXPRESSION_END: {return tr("End");}
		case SHORTCUT_TYPE_EXPRESSION_RIGHT: {return tr("Right");}
		case SHORTCUT_TYPE_EXPRESSION_LEFT: {return tr("Left");}
		case SHORTCUT_TYPE_EXPRESSION_UP: {return tr("Up");}
		case SHORTCUT_TYPE_EXPRESSION_DOWN: {return tr("Down");}
		case SHORTCUT_TYPE_EXPRESSION_UNDO: {return tr("Undo");}
		case SHORTCUT_TYPE_EXPRESSION_REDO: {return tr("Redo");}
		case SHORTCUT_TYPE_CALCULATE_EXPRESSION: {return tr("Calculate expression");}
		case SHORTCUT_TYPE_EXPRESSION_HISTORY_NEXT: {return tr("Expression history next");}
		case SHORTCUT_TYPE_EXPRESSION_HISTORY_PREVIOUS: {return tr("Expression history previous");}
	}
	return "-";
}

bool QalculateQtSettings::testShortcutValue(int type, QString &value, QWidget *w) {
	switch(type) {
		case SHORTCUT_TYPE_FUNCTION: {}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
			value = value.trimmed();
			if(value.length() > 2 && value.right(2) == "()") value.chop(2);
			if(!CALCULATOR->getActiveFunction(value.toStdString())) {
				QMessageBox::critical(w, QApplication::tr("Error"), QApplication::tr("Function not found."), QMessageBox::Ok);
				return false;
			}
			break;
		}
		case SHORTCUT_TYPE_VARIABLE: {
			value = value.trimmed();
			if(!CALCULATOR->getActiveVariable(value.toStdString())) {
				QMessageBox::critical(w, QApplication::tr("Error"), QApplication::tr("Variable not found."), QMessageBox::Ok);
				return false;
			}
			break;
		}
		case SHORTCUT_TYPE_UNIT: {
			value = value.trimmed();
			if(!CALCULATOR->getActiveUnit(value.toStdString())) {
				QMessageBox::critical(w, QApplication::tr("Error"), QApplication::tr("Unit not found."), QMessageBox::Ok);
				return false;
			}
			break;
		}
		case SHORTCUT_TYPE_OPERATOR: {
			value = value.trimmed();
			break;
		}
		case SHORTCUT_TYPE_TO_NUMBER_BASE: {}
		case SHORTCUT_TYPE_INPUT_BASE: {}
		case SHORTCUT_TYPE_OUTPUT_BASE: {
			value = value.trimmed();
			Number nbase; int base;
			base_from_string(value.toStdString(), base, nbase, type == SHORTCUT_TYPE_INPUT_BASE);
			if(base == BASE_CUSTOM && nbase.isZero()) {
				QMessageBox::critical(w, QApplication::tr("Error"), QApplication::tr("Unsupported base."), QMessageBox::Ok);
				return false;
			}
			break;
		}
		case SHORTCUT_TYPE_PRECISION: {}
		case SHORTCUT_TYPE_MIN_DECIMALS: {}
		case SHORTCUT_TYPE_MAX_DECIMALS: {}
		case SHORTCUT_TYPE_MINMAX_DECIMALS: {
			bool ok = false;
			int v = value.toInt(&ok, 10);
			if(!ok || v < -1 || (type == SHORTCUT_TYPE_PRECISION && v < 2)) {
				QMessageBox::critical(w, QApplication::tr("Error"), QApplication::tr("Unsupported value."), QMessageBox::Ok);
				return false;
			}
			break;
		}
	}
	return true;
}

bool QalculateQtSettings::loadWorkspace(const char *filename) {
	FILE *file = NULL;
	std::string stmp, svalue, svar;
	char line[1000000L];
	if(!filename || strlen(filename) == 0) {
		file = fopen(buildPath(getLocalDir(), "qalculate-qt.cfg").c_str(), "r");
		if(!file) return false;
	} else {
		file = fopen(filename, "r");
		if(!file) return false;
		if(fgets(line, 1000000L, file) == NULL) return false;
		stmp = line;
		if(stmp.find("QALCULATE") == std::string::npos) return false;
	}
	size_t i;
	v_expression.clear();
	v_parse.clear();
	v_value.clear();
	v_protected.clear();
	v_delexpression.clear();
	v_result.clear();
	v_exact.clear();
	v_pexact.clear();
	v_delresult.clear();
	expression_history.clear();
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isLocal() && CALCULATOR->variables[i]->category() == CALCULATOR->temporaryCategory()) CALCULATOR->variables[i]->destroy();
	}
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(CALCULATOR->functions[i]->isLocal() && CALCULATOR->functions[i]->category() == CALCULATOR->temporaryCategory()) CALCULATOR->functions[i]->destroy();
	}
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->isLocal() && CALCULATOR->units[i]->category() == CALCULATOR->temporaryCategory()) CALCULATOR->units[i]->destroy();
	}
	for(size_t i = 0; i < history_answer.size(); i++) {
		history_answer[i]->unref();
	}
	history_answer.clear();
	current_result = NULL;
	v_memory->set(m_zero);
	while(true) {
		if(fgets(line, 1000000L, file) == NULL) break;
		stmp = line;
		remove_blank_ends(stmp);
		if((i = stmp.find_first_of("=")) != std::string::npos) {
			svar = stmp.substr(0, i);
			remove_blank_ends(svar);
			svalue = stmp.substr(i + 1);
			remove_blank_ends(svalue);
			readPreferenceValue(svar, svalue, true);
		} else if(stmp == "[Definitions]") {
			stmp = "";
			while(true) {
				if(fgets(line, 1000000L, file) == NULL) break;
				stmp += line;
			}
			remove_blank_ends(stmp);
			CALCULATOR->loadDefinitions(stmp.c_str(), true);
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				if(CALCULATOR->variables[i]->isLocal() && !CALCULATOR->variables[i]->isHidden() && CALCULATOR->variables[i]->isActive() && CALCULATOR->variables[i]->category() == CALCULATOR->temporaryCategory()) favourite_variables.push_back(CALCULATOR->variables[i]);
			}
			for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
				if(CALCULATOR->functions[i]->isLocal() && !CALCULATOR->functions[i]->isHidden() && CALCULATOR->functions[i]->isActive() && CALCULATOR->functions[i]->category() == CALCULATOR->temporaryCategory()) favourite_functions.push_back(CALCULATOR->functions[i]);
			}
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if(CALCULATOR->units[i]->isLocal() && !CALCULATOR->units[i]->isHidden() && CALCULATOR->units[i]->isActive() && CALCULATOR->units[i]->category() == CALCULATOR->temporaryCategory()) favourite_units.push_back(CALCULATOR->units[i]);
			}
		}
	}
	while(v_expression.size() > v_parse.size()) {
		v_parse.push_back("");
		v_pexact.push_back(true);
	}
	if(!filename || strlen(filename) == 0) {
		current_workspace = "";
	} else {
		current_workspace = filename;
		for(size_t i = 0; i < recent_workspaces.size(); i++) {
			if(recent_workspaces[i] == current_workspace) {
				recent_workspaces.erase(recent_workspaces.begin() + i);
				break;
			}
		}
		if(recent_workspaces.size() >= 9) recent_workspaces.erase(recent_workspaces.begin());
		recent_workspaces.push_back(filename);
	}
	fclose(file);
	return true;
}
bool QalculateQtSettings::saveWorkspace(const char *filename) {
	if(savePreferences(filename, true)) {
		current_workspace = filename;
		for(size_t i = 0; i < recent_workspaces.size(); i++) {
			if(recent_workspaces[i] == current_workspace) {
				recent_workspaces.erase(recent_workspaces.begin() + i);
				break;
			}
		}
		if(recent_workspaces.size() >= 9) recent_workspaces.erase(recent_workspaces.begin());
		recent_workspaces.push_back(filename);
		return true;
	}
	return false;
}
QString QalculateQtSettings::workspaceTitle() {
	if(current_workspace.empty()) return QString();
#ifdef _WIN32
	size_t index = current_workspace.rfind('\\');
#else
	size_t index = current_workspace.rfind('/');
#endif
	if(index != std::string::npos) return QString::fromStdString(current_workspace.substr(index + 1));
	return QString::fromStdString(current_workspace);
}

