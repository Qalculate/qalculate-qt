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
class QAction;

bool can_display_unicode_string_function(const char *str, void *w);

#define EQUALS_IGNORECASE_AND_LOCAL(x,y,z)	(equalsIgnoreCase(x, y) || equalsIgnoreCase(x, z.toStdString()))
#define EQUALS_IGNORECASE_AND_LOCAL_NR(x,y,z,a)	(equalsIgnoreCase(x, y a) || (x.length() == z.length() + strlen(a) && equalsIgnoreCase(x.substr(0, x.length() - strlen(a)), z.toStdString()) && equalsIgnoreCase(x.substr(x.length() - strlen(a)), a)))

#define LOAD_APP_ICON(x) QIcon(":/icons/apps/scalable/" x ".svg")
#define LOAD_ICON(x) load_icon(x, this)

#define USE_QUOTES(arg, f) (arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && f->id() != FUNCTION_ID_BASE && f->id() != FUNCTION_ID_BIN && f->id() != FUNCTION_ID_OCT && f->id() != FUNCTION_ID_DEC && f->id() != FUNCTION_ID_HEX)

std::string to_html_escaped(const std::string str);
std::string unhtmlize(std::string str, bool b_ascii = false);
QString unhtmlize(QString str, bool b_ascii = false);
std::string unformat(std::string str, bool restorable = false);
std::string uncolorize(std::string str, bool remove_class = true);
std::string replace_first_minus(const std::string &str);
QIcon load_icon(const QString &str, QWidget*);
bool last_is_operator(std::string str, bool allow_exp = false);
bool string_is_less(std::string str1, std::string str2);
bool item_in_calculator(ExpressionItem *item);
bool name_matches(ExpressionItem *item, const std::string &str);
bool country_matches(Unit *u, const std::string &str, size_t minlength = 0);
bool title_matches(ExpressionItem *item, const std::string &str, size_t minlength = 0);
bool contains_plot_or_save(const std::string &str);
void base_from_string(std::string str, int &base, Number &nbase, bool input_base = false);
long int get_fixed_denominator_qt(const std::string &str, int &to_fraction, const QString &localized_fraction, bool qalc_command = false);

enum {
	TITLE_APP,
	TITLE_RESULT,
	TITLE_APP_RESULT,
	TITLE_WORKSPACE,
	TITLE_APP_WORKSPACE
};

enum {
	KEEP_EXPRESSION,
	REPLACE_EXPRESSION_WITH_RESULT,
	REPLACE_EXPRESSION_WITH_RESULT_IF_SHORTER,
	CLEAR_EXPRESSION
};

typedef enum {
	SHORTCUT_TYPE_FUNCTION,
	SHORTCUT_TYPE_FUNCTION_WITH_DIALOG,
	SHORTCUT_TYPE_VARIABLE,
	SHORTCUT_TYPE_UNIT,
	SHORTCUT_TYPE_OPERATOR,
	SHORTCUT_TYPE_TEXT,
	SHORTCUT_TYPE_DATE,
	SHORTCUT_TYPE_MATRIX,
	SHORTCUT_TYPE_SMART_PARENTHESES,
	SHORTCUT_TYPE_CONVERT_TO,
	SHORTCUT_TYPE_CONVERT,
	SHORTCUT_TYPE_OPTIMAL_UNIT,
	SHORTCUT_TYPE_BASE_UNITS,
	SHORTCUT_TYPE_OPTIMAL_PREFIX,
	SHORTCUT_TYPE_TO_NUMBER_BASE,
	SHORTCUT_TYPE_FACTORIZE,
	SHORTCUT_TYPE_EXPAND,
	SHORTCUT_TYPE_PARTIAL_FRACTIONS,
	SHORTCUT_TYPE_APPROXIMATE,
	SHORTCUT_TYPE_NEGATE,
	SHORTCUT_TYPE_INVERT,
	SHORTCUT_TYPE_RPN_UP,
	SHORTCUT_TYPE_RPN_DOWN,
	SHORTCUT_TYPE_RPN_SWAP,
	SHORTCUT_TYPE_RPN_COPY,
	SHORTCUT_TYPE_RPN_LASTX,
	SHORTCUT_TYPE_RPN_DELETE,
	SHORTCUT_TYPE_RPN_CLEAR,
	SHORTCUT_TYPE_OUTPUT_BASE,
	SHORTCUT_TYPE_INPUT_BASE,
	SHORTCUT_TYPE_DEGREES,
	SHORTCUT_TYPE_RADIANS,
	SHORTCUT_TYPE_GRADIANS,
	SHORTCUT_TYPE_NORMAL_NOTATION,
	SHORTCUT_TYPE_SCIENTIFIC_NOTATION,
	SHORTCUT_TYPE_ENGINEERING_NOTATION,
	SHORTCUT_TYPE_SIMPLE_NOTATION,
	SHORTCUT_TYPE_COPY_RESULT,
	SHORTCUT_TYPE_INSERT_RESULT,
	SHORTCUT_TYPE_RPN_MODE,
	SHORTCUT_TYPE_CHAIN_MODE,
	SHORTCUT_TYPE_GENERAL_KEYPAD,
	SHORTCUT_TYPE_PROGRAMMING_KEYPAD,
	SHORTCUT_TYPE_ALGEBRA_KEYPAD,
	SHORTCUT_TYPE_CUSTOM_KEYPAD,
	SHORTCUT_TYPE_KEYPAD,
	SHORTCUT_TYPE_HISTORY_SEARCH,
	SHORTCUT_TYPE_ALWAYS_ON_TOP,
	SHORTCUT_TYPE_COMPLETE,
	SHORTCUT_TYPE_STORE,
	SHORTCUT_TYPE_NEW_VARIABLE,
	SHORTCUT_TYPE_NEW_FUNCTION,
	SHORTCUT_TYPE_MANAGE_VARIABLES,
	SHORTCUT_TYPE_MANAGE_FUNCTIONS,
	SHORTCUT_TYPE_MANAGE_UNITS,
	SHORTCUT_TYPE_MANAGE_DATA_SETS,
	SHORTCUT_TYPE_MEMORY_CLEAR,
	SHORTCUT_TYPE_MEMORY_RECALL,
	SHORTCUT_TYPE_MEMORY_STORE,
	SHORTCUT_TYPE_MEMORY_ADD,
	SHORTCUT_TYPE_MEMORY_SUBTRACT,
	SHORTCUT_TYPE_EXPRESSION_CLEAR,
	SHORTCUT_TYPE_EXPRESSION_DELETE,
	SHORTCUT_TYPE_EXPRESSION_BACKSPACE,
	SHORTCUT_TYPE_EXPRESSION_START,
	SHORTCUT_TYPE_EXPRESSION_END,
	SHORTCUT_TYPE_EXPRESSION_RIGHT,
	SHORTCUT_TYPE_EXPRESSION_LEFT,
	SHORTCUT_TYPE_EXPRESSION_UP,
	SHORTCUT_TYPE_EXPRESSION_DOWN,
	SHORTCUT_TYPE_EXPRESSION_HISTORY_NEXT,
	SHORTCUT_TYPE_EXPRESSION_HISTORY_PREVIOUS,
	SHORTCUT_TYPE_EXPRESSION_UNDO,
	SHORTCUT_TYPE_EXPRESSION_REDO,
	SHORTCUT_TYPE_CALCULATE_EXPRESSION,
	SHORTCUT_TYPE_PLOT,
	SHORTCUT_TYPE_NUMBER_BASES,
	SHORTCUT_TYPE_FLOATING_POINT,
	SHORTCUT_TYPE_CALENDARS,
	SHORTCUT_TYPE_PERCENTAGE_TOOL,
	SHORTCUT_TYPE_PERIODIC_TABLE,
	SHORTCUT_TYPE_UPDATE_EXRATES,
	SHORTCUT_TYPE_MODE,
	SHORTCUT_TYPE_MENU,
	SHORTCUT_TYPE_HELP,
	SHORTCUT_TYPE_QUIT,
	SHORTCUT_TYPE_HISTORY_CLEAR,
	SHORTCUT_TYPE_PRECISION,
	SHORTCUT_TYPE_MIN_DECIMALS,
	SHORTCUT_TYPE_MAX_DECIMALS,
	SHORTCUT_TYPE_MINMAX_DECIMALS,
	SHORTCUT_TYPE_FUNCTIONS_MENU,
	SHORTCUT_TYPE_UNITS_MENU,
	SHORTCUT_TYPE_VARIABLES_MENU
} shortcut_type;

#define LAST_SHORTCUT_TYPE SHORTCUT_TYPE_VARIABLES_MENU

#define SHORTCUT_REQUIRES_VALUE(x) (x == SHORTCUT_TYPE_FUNCTION || x == SHORTCUT_TYPE_FUNCTION_WITH_DIALOG || x == SHORTCUT_TYPE_UNIT || x == SHORTCUT_TYPE_VARIABLE || x == SHORTCUT_TYPE_TEXT || x == SHORTCUT_TYPE_OPERATOR || x == SHORTCUT_TYPE_CONVERT_TO || x == SHORTCUT_TYPE_TO_NUMBER_BASE || x == SHORTCUT_TYPE_INPUT_BASE || x == SHORTCUT_TYPE_OUTPUT_BASE || x == SHORTCUT_TYPE_PRECISION || x == SHORTCUT_TYPE_MAX_DECIMALS || x == SHORTCUT_TYPE_MIN_DECIMALS || x == SHORTCUT_TYPE_MINMAX_DECIMALS)

#define SHORTCUT_USES_VALUE(x) (SHORTCUT_REQUIRES_VALUE(x) || x == SHORTCUT_TYPE_COPY_RESULT)

struct keyboard_shortcut {
	QString key;
	std::vector<shortcut_type> type;
	std::vector<std::string> value;
	QAction *action;
	bool new_action;
};

struct custom_button {
	int c, r;
	int type[3];
	std::string value[3];
	QString label;
};

DECLARE_BUILTIN_FUNCTION(AnswerFunction, 0)

class QalculateQtSettings : QObject {

	Q_OBJECT

	public:

		QalculateQtSettings();
		~QalculateQtSettings();

		void loadPreferences();
		void readPreferenceValue(const std::string &svar, const std::string &svalue, bool is_workspace = false);
		int savePreferences(bool save_mode = true);
		bool savePreferences(const char *filename, bool is_workspace = false, bool save_mode = true);
		void updateStyle();
		void updatePalette(bool force_update = false);
		void updateMessagePrintOptions();
		void updateFavourites();
		void setCustomAngleUnit();
		bool checkExchangeRates(QWidget *parent);
		void fetchExchangeRates(int timeout, int n = -1, QWidget *parent = NULL);
		bool displayMessages(QWidget *parent);
		bool isAnswerVariable(Variable *v);
		QString shortcutTypeText(shortcut_type type);
		QString shortcutText(int type, const std::string &value);
		QString shortcutText(const std::vector<shortcut_type> &type, const std::vector<std::string> &value);
		void updateActionValueTexts();
		bool testShortcutValue(int type, QString &value, QWidget *parent = NULL);
		void checkVersion(bool force, QWidget *parent);
		void autoUpdate(std::string new_version, QWidget *parent);
		const char *multiplicationSign(bool units = false);
		const char *divisionSign(bool output = true);
		std::string localizeExpression(std::string, bool unit_expression = false);
		std::string unlocalizeExpression(std::string);
		bool loadWorkspace(const char *filename);
		bool saveWorkspace(const char *filename);
		QString workspaceTitle();
		QStringList copy_action_value_texts;

		EvaluationOptions evalops;
		PrintOptions printops;
		bool complex_angle_form, dot_question_asked, implicit_question_asked, adaptive_interval_display, tc_set, rpn_mode, chain_mode, caret_as_xor, ignore_locale, do_imaginary_j, fetch_exchange_rates_at_startup, always_on_top, display_expression_status, prefixes_default, rpn_keys, simplified_percentage, sinc_set;
		bool programming_base_changed;
		int previous_precision;
		std::string custom_angle_unit;
		QString custom_lang;
		int rounding_mode;
		int allow_multiple_instances;
		int decimal_comma, dual_fraction, dual_approximation, auto_update_exchange_rates, title_type;
		int completion_delay, expression_status_delay;
		int completion_min, completion_min2;
		int style, palette;
		bool enable_completion, enable_completion2;
		int enable_tooltips;
		int color;
		bool colorize_result;
		bool format_result;
		bool first_time;
		bool enable_input_method;
		bool use_custom_result_font, use_custom_expression_font, use_custom_keypad_font, use_custom_app_font;
		bool save_custom_result_font, save_custom_expression_font, save_custom_keypad_font, save_custom_app_font;
		int replace_expression;
		bool autocopy_result;
		int default_signed = -1, default_bits = -1;
		int keypad_type;
		bool separate_keypad_menu_buttons;
		int toolbar_style;
		int show_keypad;
		int show_bases;
		bool hide_numpad;
		bool keep_function_dialog_open;
		bool save_defs_on_exit, save_mode_on_exit, clear_history_on_exit;
		bool rpn_shown;
		bool auto_calculate;
		int history_expression_type;
		bool copy_ascii, copy_ascii_without_units;
		bool close_with_esc;
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
		QByteArray datasets_geometry, datasets_vsplitter_state, datasets_hsplitter_state;
		bool wayland_platform;

		std::string volume_category;
		std::vector<std::string> alternative_volume_categories;

		PlotLegendPlacement default_plot_legend_placement;
		PlotStyle default_plot_style;
		PlotSmoothing default_plot_smoothing;
		bool default_plot_display_grid, default_plot_full_border, default_plot_use_sampling_rate, default_plot_rows, default_plot_color;
		std::string default_plot_min, default_plot_max, default_plot_step, default_plot_variable;
		int default_plot_sampling_rate, default_plot_linewidth, default_plot_type, max_plot_time, default_plot_complex;

		QalculateDateTime last_version_check_date;
		bool check_version;
		std::string last_found_version;
		int preferences_version[3];

		std::string current_workspace;
		std::vector<std::string> recent_workspaces;
		int save_workspace;

		std::vector<std::string> v_expression;
		std::vector<std::string> v_parse;
		std::vector<bool> v_pexact;
		std::vector<bool> v_protected;
		std::vector<bool> v_delexpression;
		std::vector<std::vector<std::string> > v_result;
		std::vector<std::vector<bool> > v_delresult;
		std::vector<std::vector<int> > v_exact;
		std::vector<std::vector<size_t> > v_value;
		std::vector<QString> v_messages;
		std::vector<bool> v_parseerror;
		std::vector<MathFunction*> favourite_functions, recent_functions;
		std::vector<Variable*> favourite_variables, recent_variables;
		std::vector<Unit*> favourite_units, recent_units;
		std::string latest_button_unit;
		std::vector<std::string> favourite_functions_pre, recent_functions_pre;
		std::vector<std::string> favourite_variables_pre, recent_variables_pre;
		std::vector<std::string> favourite_units_pre, recent_units_pre;
		bool default_shortcuts;
		std::vector<keyboard_shortcut*> keyboard_shortcuts;
		int custom_button_columns, custom_button_rows;
		std::vector<custom_button> custom_buttons;
		bool favourite_functions_changed, favourite_variables_changed, favourite_units_changed;
		bool show_all_functions, show_all_variables, show_all_units, use_function_dialog;

};

class MathLineEdit : public QLineEdit {

	public:

		MathLineEdit(QWidget *parent = NULL, bool unit_expression = false, bool function_expression = false);
		virtual ~MathLineEdit();

	protected:

		bool b_unit, b_function;
		void keyPressEvent(QKeyEvent*) override;

};

extern QalculateQtSettings *settings;

#endif //QALCULATE_Q_SETTINGS_H
