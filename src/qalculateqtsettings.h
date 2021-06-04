/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef QALCULATE_Q_SETTINGS_H
#define QALCULATE_Q_SETTINGS_H

#include <libqalculate/qalculate.h>

class QWidget;

bool can_display_unicode_string_function(const char *str, void *w);

class QalculateQtSettings {
	public:

		QalculateQtSettings();
		~QalculateQtSettings();

		void loadPreferences();
		void savePreferences();

		void updateMessagePrintOptions();

		void fetchExchangeRates(int timeout, int n = -1, QWidget *parent = NULL);

		bool isAnswerVariable(Variable *v);

		EvaluationOptions evalops;
		PrintOptions printops;
		bool complex_angle_form, dot_question_asked, adaptive_interval_display, tc_set, rpn_mode, caret_as_xor, ignore_locale, do_imaginary_j, fetch_exchange_rates_at_startup;
		int b_decimal_comma, dual_fraction, dual_approximation, auto_update_exchange_rates;
		KnownVariable *vans[5], *v_memory;

};

#endif //QALCULATE_Q_SETTINGS_H
