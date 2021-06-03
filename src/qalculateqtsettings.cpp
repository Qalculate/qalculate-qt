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

void QalculateQtSettings::loadPreferences() {

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
	printops.can_display_unicode_string_function = NULL;
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

	dot_question_asked = false;
	complex_angle_form = false;
	b_decimal_comma = -1;
	adaptive_interval_display = true;
	tc_set = false;
	dual_fraction = -1;
	dual_approximation = -1;
	auto_update_exchange_rates = -1;
	rpn_mode = false;
	caret_as_xor = false;
	do_imaginary_j = false;

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

class FetchExchangeRatesThread : public Thread {
protected:
	virtual void run();
};

void QalculateQtSettings::fetchExchangeRates(int timeout, int n, QWidget *parent) {
	//bool b_busy_bak = b_busy;
	//block_error_timeout++;
	//b_busy = true;
	FetchExchangeRatesThread fetch_thread;
	if(fetch_thread.start() && fetch_thread.write(timeout) && fetch_thread.write(n)) {
		int i = 0;
		while(fetch_thread.running && i < 50) {
			qApp->processEvents();
			sleep_ms(10);
			i++;
		}
		if(fetch_thread.running) {
			//GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(main_builder, "main_window")), (GtkDialogFlags) (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL), GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, _("Fetching exchange rates."));
			//if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
			//gtk_widget_show(dialog);
			while(fetch_thread.running) {
				qApp->processEvents();
				sleep_ms(10);
			}
			//gtk_widget_destroy(dialog);
		}
	}
	//b_busy = b_busy_bak;
	//block_error_timeout--;
}

void FetchExchangeRatesThread::run() {
	int timeout = 15;
	int n = -1;
	if(!read(&timeout)) return;
	if(!read(&n)) return;
	CALCULATOR->fetchExchangeRates(timeout, n);
}

