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
#include <QLineEdit>
#include <QIcon>
#include <libqalculate/qalculate.h>

class QWidget;
class QByteArray;

bool can_display_unicode_string_function(const char *str, void *w);

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z.toStdString()))
#define EQUALS_IGNORECASE_AND_LOCAL_NR(x,y,z,a)	(equalsIgnoreCase(x, y a) || (x.length() == z.length() + strlen(a) && equalsIgnoreCase(x.substr(0, x.length() - strlen(a)), z.toStdString()) && equalsIgnoreCase(x.substr(x.length() - strlen(a)), a)))

#define LOAD_APP_ICON(x) QIcon(":/icons/apps/scalable/" x ".svg")
#define LOAD_ICON(x) load_icon(x, this)

std::string to_html_escaped(const std::string str);
std::string unhtmlize(std::string str);
QIcon load_icon(const QString &str, QWidget*);
bool last_is_operator(std::string str, bool allow_exp = false);
bool string_is_less(std::string str1, std::string str2);
bool item_in_calculator(ExpressionItem *item);
bool name_matches(ExpressionItem *item, const std::string &str);
bool country_matches(Unit *u, const std::string &str, size_t minlength = 0);
bool title_matches(ExpressionItem *item, const std::string &str, size_t minlength = 0);

enum {
	TITLE_APP,
	TITLE_RESULT,
	TITLE_APP_RESULT
};

enum {
	KEEP_EXPRESSION,
	REPLACE_EXPRESSION_WITH_RESULT,
	REPLACE_EXPRESSION_WITH_RESULT_IF_SHORTER,
	CLEAR_EXPRESSION
};

DECLARE_BUILTIN_FUNCTION(AnswerFunction, 0)

class QalculateQtSettings : QObject {

	Q_OBJECT

	public:

		QalculateQtSettings();
		~QalculateQtSettings();

		void loadPreferences();
		void savePreferences(bool save_mode = true);
		void updateStyle();
		void updatePalette();
		void updateMessagePrintOptions();
		bool checkExchangeRates(QWidget *parent);
		void fetchExchangeRates(int timeout, int n = -1, QWidget *parent = NULL);
		bool displayMessages(QWidget *parent);
		bool isAnswerVariable(Variable *v);
		void checkVersion(bool force, QWidget *parent);
		void autoUpdate(std::string new_version, QWidget *parent);
		const char *multiplicationSign();
		const char *divisionSign(bool output = true);

		EvaluationOptions evalops;
		PrintOptions printops;
		bool complex_angle_form, dot_question_asked, implicit_question_asked, adaptive_interval_display, tc_set, rpn_mode, chain_mode, caret_as_xor, ignore_locale, do_imaginary_j, fetch_exchange_rates_at_startup, always_on_top, display_expression_status, prefixes_default, rpn_keys;
		int allow_multiple_instances;
		int decimal_comma, dual_fraction, dual_approximation, auto_update_exchange_rates, title_type;
		int completion_delay, expression_status_delay;
		int completion_min, completion_min2;
		int style, palette;
		bool enable_completion, enable_completion2;
		int color;
		bool colorize_result;
		bool first_time;
		bool enable_input_method;
		bool use_custom_result_font, use_custom_expression_font, use_custom_keypad_font, use_custom_app_font;
		bool save_custom_result_font, save_custom_expression_font, save_custom_keypad_font, save_custom_app_font;
		int replace_expression;
		bool keep_function_dialog_open;
		bool save_defs_on_exit, save_mode_on_exit, clear_history_on_exit;
		bool rpn_shown;
		std::string custom_result_font, custom_expression_font, custom_keypad_font, custom_app_font;
		KnownVariable *vans[5], *v_memory;
		MathStructure *current_result;
		MathFunction *f_answer;
		std::vector<MathStructure*> history_answer;
		std::vector<std::string> expression_history;
		QByteArray window_geometry, window_state, splitter_state;
		QByteArray functions_geometry, functions_vsplitter_state, functions_hsplitter_state;
		QByteArray units_geometry, units_vsplitter_state, units_hsplitter_state;
		QByteArray variables_geometry, variables_vsplitter_state, variables_hsplitter_state;

		PlotLegendPlacement default_plot_legend_placement;
		PlotStyle default_plot_style;
		PlotSmoothing default_plot_smoothing;
		bool default_plot_display_grid, default_plot_full_border, default_plot_use_sampling_rate, default_plot_rows, default_plot_color;
		std::string default_plot_min, default_plot_max, default_plot_step, default_plot_variable;
		int default_plot_sampling_rate, default_plot_linewidth, default_plot_type, max_plot_time;

		QalculateDateTime last_version_check_date;
		bool check_version;
		std::string last_found_version;

		std::vector<std::string> v_expression;
		std::vector<bool> v_protected;
		std::vector<bool> v_delexpression;
		std::vector<std::vector<std::string> > v_result;
		std::vector<std::vector<bool> > v_delresult;
		std::vector<std::vector<int> > v_exact;

};

class MathLineEdit : public QLineEdit {

	public:

		MathLineEdit(QWidget *parent = NULL);
		virtual ~MathLineEdit();

	protected:

		void keyPressEvent(QKeyEvent*) override;

};

extern QalculateQtSettings *settings;

#endif //QALCULATE_Q_SETTINGS_H
