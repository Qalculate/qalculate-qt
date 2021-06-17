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

#include <QString>
#include <QIcon>
#include <libqalculate/qalculate.h>

class QWidget;
class QByteArray;

bool can_display_unicode_string_function(const char *str, void *w);

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z.toStdString()))
#define EQUALS_IGNORECASE_AND_LOCAL_NR(x,y,z,a)	(equalsIgnoreCase(x, y a) || (x.length() == z.length() + strlen(a) && equalsIgnoreCase(x.substr(0, x.length() - strlen(a)), z.toStdString()) && equalsIgnoreCase(x.substr(x.length() - strlen(a)), a)))

#define LOAD_APP_ICON(x) QIcon(":/icons/apps/scalable/" x ".svg")
#define LOAD_ICON(x) load_icon(x, this)

std::string unhtmlize(std::string str, bool replace_all_i = false);
QIcon load_icon(const QString &str, QWidget*);
bool last_is_operator(std::string str, bool allow_exp = false);

enum {
	TITLE_APP,
	TITLE_RESULT,
	TITLE_APP_RESULT
};

DECLARE_BUILTIN_FUNCTION(AnswerFunction, 0)

class QalculateQtSettings {

	public:

		QalculateQtSettings();
		~QalculateQtSettings();

		void loadPreferences();
		void savePreferences();

		void updateMessagePrintOptions();

		bool isAnswerVariable(Variable *v);

		EvaluationOptions evalops;
		PrintOptions printops;
		bool complex_angle_form, dot_question_asked, adaptive_interval_display, tc_set, rpn_mode, caret_as_xor, ignore_locale, do_imaginary_j, fetch_exchange_rates_at_startup, always_on_top, display_expression_status, prefixes_default;
		int decimal_comma, dual_fraction, dual_approximation, auto_update_exchange_rates, title_type;
		int completion_delay;
		int completion_min, completion_min2;
		bool enable_completion, enable_completion2;
		int color;
		bool colorize_result;
		bool first_time;
		bool enable_input_method;
		KnownVariable *vans[5], *v_memory;
		MathStructure *current_result;
		MathFunction *f_answer;
		std::vector<MathStructure*> history_answer;
		std::vector<std::string> expression_history;
		QByteArray window_geometry, window_state, splitter_state;

};

extern QalculateQtSettings *settings;

#endif //QALCULATE_Q_SETTINGS_H
