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
#include <QToolTip>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QMenu>
#include <QActionGroup>
#include <QToolButton>
#include <QSpinBox>
#include <QWidgetAction>
#include <QStyle>
#include <QTimer>
#include <QComboBox>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QDebug>

#include "qalculatewindow.h"
#include "qalculateqtsettings.h"
#include "expressionedit.h"
#include "historyview.h"
#include "keypadwidget.h"
#include "variableeditdialog.h"
#include "preferencesdialog.h"
#include "functionsdialog.h"
#include "fpconversiondialog.h"

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
	COMMAND_EXPAND_PARTIAL_FRACTIONS,
	COMMAND_EXPAND,
	COMMAND_TRANSFORM,
	COMMAND_CONVERT_UNIT,
	COMMAND_CONVERT_STRING,
	COMMAND_CONVERT_BASE,
	COMMAND_CONVERT_OPTIMAL,
	COMMAND_CALCULATE,
	COMMAND_EVAL
};

std::vector<std::string> alt_results;
int b_busy = 0, block_result_update = 0;
bool exact_comparison, command_aborted;
std::string original_expression, result_text, parsed_text, exact_text, previous_expression;
ViewThread *view_thread;
CommandThread *command_thread;
MathStructure *mstruct, *parsed_mstruct, *parsed_tostruct, mstruct_exact, lastx;
std::string command_convert_units_string;
Unit *command_convert_unit;
bool block_expression_history = false;
bool to_fraction = false;
char to_prefix = 0;
int to_base = 0;
int to_caf = -1;
unsigned int to_bits = 0;
Number to_nbase;
std::string result_bin, result_oct, result_dec, result_hex;
Number max_bases, min_bases;
bool title_modified = false;

extern void fix_to_struct(MathStructure &m);
extern void print_dual(const MathStructure &mresult, const std::string &original_expression, const MathStructure &mparse, MathStructure &mexact, std::string &result_str, std::vector<std::string> &results_v, PrintOptions &po, const EvaluationOptions &evalops, AutomaticFractionFormat auto_frac, AutomaticApproximation auto_approx, bool cplx_angle = false, bool *exact_cmp = NULL, bool b_parsed = true, bool format = false, int colorize = 0, int tagtype = TAG_TYPE_HTML, int max_length = -1);
extern void calculate_dual_exact(MathStructure &mstruct_exact, MathStructure *mstruct, const std::string &original_expression, const MathStructure *parsed_mstruct, EvaluationOptions &evalops, AutomaticApproximation auto_approx, int msecs = 0, int max_size = 10);
extern int has_information_unit(const MathStructure &m, bool top = true);

bool contains_unknown_variable(const MathStructure &m) {
	if(m.isVariable()) return !m.variable()->isKnown();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_unknown_variable(m[i])) return true;
	}
	return false;
}

std::string print_with_evalops(const Number &nr) {
	PrintOptions po;
	po.is_approximate = NULL;
	po.base = settings->evalops.parse_options.base;
	po.base_display = BASE_DISPLAY_NONE;
	po.twos_complement = settings->evalops.parse_options.twos_complement;
	Number nr_base;
	if(po.base == BASE_CUSTOM) {
		nr_base = CALCULATOR->customOutputBase();
		CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
	}
	if(po.base == BASE_CUSTOM && CALCULATOR->customInputBase().isInteger() && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
		nr_base = CALCULATOR->customOutputBase();
		CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
	} else if((po.base < BASE_CUSTOM && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26) || (po.base == BASE_CUSTOM && CALCULATOR->customInputBase() <= 12 && CALCULATOR->customInputBase() >= -12)) {
		po.base = 10;
		std::string str = "dec(";
		str += nr.print(po);
		str += ")";
		return str;
	} else if(po.base == BASE_CUSTOM) {
		po.base = 10;
	}
	std::string str = nr.print(po);
	if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
	return str;
}

std::string unhtmlize(std::string str) {
	size_t i = 0, i2;
	while(true) {
		i = str.find("<", i);
		if(i == std::string::npos) break;
		i2 = str.find(">", i + 1);
		if(i2 == std::string::npos) break;
		if(i2 - i == 4) {
			if(str.substr(i + 1, 3) == "sup") {
				size_t i3 = str.find("</sup>", i2 + 1);
				if(i3 != std::string::npos) {
					std::string str2 = unhtmlize(str.substr(i + 5, i3 - i - 5));
					if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else str.replace(i, i3 - i + 6, std::string("^(") + str2 + ")");
					continue;
				}
			} else if(str.substr(i + 1, 3) == "sub") {
				size_t i3 = str.find("</sub>", i + 4);
				if(i3 != std::string::npos) {
					if(i2 - i3 > 16 && str.substr(i2 + 1, 7) == "<small>" && str.substr(i3 - 9, 8) == "</small>") str.erase(i, i3 - i + 6);
					else str.replace(i, i3 - i + 6, std::string("_") + unhtmlize(str.substr(i + 5, i3 - i - 5)));
					continue;
				}
			}
		} else if(i2 - i == 17 && str.substr(i + 1, 16) == "i class=\"symbol\"") {
			size_t i3 = str.find("</i>", i2 + 1);
			if(i3 != std::string::npos) {
				std::string name = unhtmlize(str.substr(i2 + 1, i3 - i2 - 1));
				if(name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, 1, '\\');
				} else {
					name.insert(0, 1, '\"');
					name += '\"';
				}
				str.replace(i, i3 - i + 4, name);
				continue;
			}
		}
		str.erase(i, i2 - i + 1);
	}
	gsub(" " SIGN_DIVISION_SLASH " ", "/", str);
	gsub("&amp;", "&", str);
	gsub("&gt;", ">", str);
	gsub("&lt;", "<", str);
	gsub("&hairsp;", "", str);
	gsub("&thinsp;", THIN_SPACE, str);
	return str;
}

#define ADD_SECTION(str) \
	if(!menu->style()->styleHint(QStyle::SH_Menu_SupportsSections)) { \
		aw = new QWidgetAction(this); \
		QLabel *label = new QLabel(str, this); \
		label->setAlignment(Qt::AlignCenter); \
		aw->setDefaultWidget(label); \
		menu->addSeparator(); \
		menu->addAction(aw); \
		menu->addSeparator(); \
	} else { \
		menu->addSection(str); \
	}

QalculateWindow::QalculateWindow() : QMainWindow() {

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	send_event = true;

	ecTimer = NULL;
	rfTimer = NULL;
	preferencesDialog = NULL;
	functionsDialog = NULL;
	fpConversionDialog = NULL;

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	QHBoxLayout *hLayout = new QHBoxLayout();
	topLayout->addLayout(hLayout);
	ehSplitter = new QSplitter(Qt::Vertical, this);
	hLayout->addWidget(ehSplitter, 1);

	tb = new QToolBar(this);
	tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
	tb->setOrientation(Qt::Vertical);

	QAction *action; QActionGroup *group; QMenu *menu, *menu2;
	int w, w2; QWidgetAction *aw; QWidget *aww; QHBoxLayout *awl;

	menuAction = new QToolButton(this); menuAction->setIcon(LOAD_ICON("menu")); menuAction->setText(tr("Menu"));
	menuAction->setPopupMode(QToolButton::InstantPopup);
	menu = new QMenu(this);
	menuAction->setMenu(menu);
	menu->addAction(tr("Floating point conversion (IEEE 754)"), this, SLOT(openFPConversion()));
	menu->addSeparator();
	menu->addAction(tr("Update exchange rates"), this, SLOT(fetchExchangeRates()));
	menu->addSeparator();
	menu->addAction(tr("Preferences"), this, SLOT(editPreferences()));
	menu->addSeparator();
	menu->addAction(tr("About %1").arg(qApp->applicationDisplayName()), this, SLOT(showAbout()));
	tb->addWidget(menuAction);

	modeAction = new QToolButton(this); modeAction->setIcon(LOAD_ICON("configure")); modeAction->setText(tr("Mode"));
	modeAction->setPopupMode(QToolButton::InstantPopup);
	menu = new QMenu(this);
	modeAction->setMenu(menu);

	ADD_SECTION(tr("General display mode"));
	QFontMetrics fm1(menu->font());
	w = fm1.boundingRect(tr("General display mode")).width() * 1.5;
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
	action = menu->addAction(tr("Normal"), this, SLOT(normalActivated())); action->setCheckable(true); group->addAction(action);
	if(settings->printops.min_exp == EXP_PRECISION) action->setChecked(true);
	action = menu->addAction(tr("Scientific"), this, SLOT(scientificActivated())); action->setCheckable(true); group->addAction(action);
	if(settings->printops.min_exp == EXP_SCIENTIFIC) action->setChecked(true);
	action = menu->addAction(tr("Simple"), this, SLOT(simpleActivated())); action->setCheckable(true); group->addAction(action);
	if(settings->printops.min_exp == EXP_NONE) action->setChecked(true);

	ADD_SECTION(tr("Angle unit"));
	w2 = fm1.boundingRect(tr("Angle unit")).width() * 1.5; if(w2 > w) w = w2;
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
	action = menu->addAction(tr("Radians"), this, SLOT(radiansActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_radians");
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_RADIANS) action->setChecked(true);
	action = menu->addAction(tr("Degrees"), this, SLOT(degreesActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_degrees");
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_DEGREES) action->setChecked(true);
	action = menu->addAction(tr("Gradians"), this, SLOT(gradiansActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_gradians");
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_GRADIANS) action->setChecked(true);

	ADD_SECTION(tr("Approximation"));
	w2 = fm1.boundingRect(tr("Approximation")).width() * 1.5; if(w2 > w) w = w2;
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);
	action = menu->addAction(tr("Automatic", "Automatic approximation"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_autoappr");
	action->setData(-1); assumptionTypeActions[0] = action; if(settings->dual_approximation < 0) action->setChecked(true);
	action = menu->addAction(tr("Dual", "Dual approximation"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_dualappr");
	action->setData(-2); assumptionTypeActions[0] = action; if(settings->dual_approximation > 0) action->setChecked(true);
	action = menu->addAction(tr("Exact", "Exact approximation"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(APPROXIMATION_EXACT); assumptionTypeActions[0] = action; if(settings->evalops.approximation == APPROXIMATION_EXACT) action->setChecked(true); action->setObjectName("action_exact");
	action = menu->addAction(tr("Approximate"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(APPROXIMATION_APPROXIMATE); assumptionTypeActions[0] = action; if(settings->evalops.approximation == APPROXIMATION_APPROXIMATE) action->setChecked(true); action->setObjectName("action_approximate");

	menu->addSeparator();
	menu2 = menu;
	menu = menu2->addMenu(tr("Assumptions"));
	ADD_SECTION(tr("Type", "Assumptions type"));
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive); group->setObjectName("group_type");
	action = menu->addAction(tr("Number"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_NUMBER); assumptionTypeActions[0] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_NUMBER) action->setChecked(true); action->setObjectName("");
	action = menu->addAction(tr("Real"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_REAL); assumptionTypeActions[1] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_REAL) action->setChecked(true);
	action = menu->addAction(tr("Rational"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_RATIONAL); assumptionTypeActions[2] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_RATIONAL) action->setChecked(true);
	action = menu->addAction(tr("Integer"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_INTEGER); assumptionTypeActions[3] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_INTEGER) action->setChecked(true);
	action = menu->addAction(tr("Boolean"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_BOOLEAN); assumptionTypeActions[4] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_BOOLEAN) action->setChecked(true);
	ADD_SECTION(tr("Sign", "Assumptions sign"));
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive); group->setObjectName("group_sign");
	action = menu->addAction(tr("Unknown", "Unknown assumptions sign"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_UNKNOWN); assumptionSignActions[0] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_UNKNOWN) action->setChecked(true);
	action = menu->addAction(tr("Non-zero"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NONZERO); assumptionSignActions[1] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NONZERO) action->setChecked(true);
	action = menu->addAction(tr("Positive"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_POSITIVE); assumptionSignActions[2] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_POSITIVE) action->setChecked(true);
	action = menu->addAction(tr("Non-negative"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NONNEGATIVE); assumptionSignActions[3] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NONNEGATIVE) action->setChecked(true);
	action = menu->addAction(tr("Negative"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NEGATIVE); assumptionSignActions[4] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NEGATIVE) action->setChecked(true);
	action = menu->addAction(tr("Non-positive"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NONPOSITIVE); assumptionSignActions[5] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NONPOSITIVE) action->setChecked(true);
	menu = menu2;

	ADD_SECTION(tr("Output base"));
	w2 = fm1.boundingRect(tr("Output base")).width() * 1.5; if(w2 > w) w = w2;
	bool base_checked = false;
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive); group->setObjectName("group_outbase");
	action = menu->addAction(tr("Binary"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BINARY); if(settings->printops.base == BASE_BINARY) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Octal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_OCTAL); if(settings->printops.base == BASE_OCTAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Decimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DECIMAL); if(settings->printops.base == BASE_DECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Hexadecimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_HEXADECIMAL); if(settings->printops.base == BASE_HEXADECIMAL) {base_checked = true; action->setChecked(true);}
	menu2 = menu;
	menu = menu2->addMenu(tr("Other"));
	action = menu->addAction(tr("Duodecimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DUODECIMAL); if(settings->printops.base == BASE_DUODECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Sexagesimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SEXAGESIMAL); if(settings->printops.base == BASE_SEXAGESIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Time format"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_TIME); if(settings->printops.base == BASE_TIME) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Roman numerals"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_ROMAN_NUMERALS); if(settings->printops.base == BASE_ROMAN_NUMERALS) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Unicode"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_UNICODE); if(settings->printops.base == BASE_UNICODE) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Bijective base-26"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BIJECTIVE_26); if(settings->printops.base == BASE_BIJECTIVE_26) action->setChecked(true);
	action = menu->addAction("Float", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_FP32); if(settings->printops.base == BASE_FP32) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("Double", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_FP64); if(settings->printops.base == BASE_FP64) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("φ", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_GOLDEN_RATIO); if(settings->printops.base == BASE_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("ψ", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SUPER_GOLDEN_RATIO); if(settings->printops.base == BASE_SUPER_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("π", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_PI); if(settings->printops.base == BASE_PI) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("e"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_E); if(settings->printops.base == BASE_E) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("√2"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SQRT2); if(settings->printops.base == BASE_SQRT2) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Custom:", "Number base"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_CUSTOM); if(!base_checked) action->setChecked(true); customOutputBaseAction = action;
	aw = new QWidgetAction(this);
	aww = new QWidget(this);
	aw->setDefaultWidget(aww);
	awl = new QHBoxLayout(aww);
	QSpinBox *customOutputBaseEdit = new QSpinBox(this);
	customOutputBaseEdit->setRange(INT_MIN, INT_MAX);
	customOutputBaseEdit->setValue(settings->printops.base == BASE_CUSTOM ? (CALCULATOR->customOutputBase().isZero() ? 10 : CALCULATOR->customOutputBase().intValue()) : ((settings->printops.base >= 2 && settings->printops.base <= 36) ? settings->printops.base : 10)); customOutputBaseEdit->setObjectName("spinbox_outbase");
	customOutputBaseEdit->setAlignment(Qt::AlignRight);
	connect(customOutputBaseEdit, SIGNAL(valueChanged(int)), this, SLOT(onCustomOutputBaseChanged(int)));
	awl->addWidget(customOutputBaseEdit, 0);
	menu->addAction(aw);
	menu = menu2;

	ADD_SECTION(tr("Input base"));
	w2 = fm1.boundingRect(tr("Input base")).width() * 1.5; if(w2 > w) w = w2;
	base_checked = false;
	group = new QActionGroup(this); group->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive); group->setObjectName("group_inbase");
	action = menu->addAction(tr("Binary"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BINARY); if(settings->evalops.parse_options.base == BASE_BINARY) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Octal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_OCTAL); if(settings->evalops.parse_options.base == BASE_OCTAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Decimal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DECIMAL); if(settings->evalops.parse_options.base == BASE_DECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Hexadecimal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_HEXADECIMAL); if(settings->evalops.parse_options.base == BASE_HEXADECIMAL) {base_checked = true; action->setChecked(true);}
	menu2 = menu;
	menu = menu2->addMenu(tr("Other"));
	action = menu->addAction(tr("Duodecimal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DUODECIMAL); if(settings->evalops.parse_options.base == BASE_DUODECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Roman numerals"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_ROMAN_NUMERALS); if(settings->evalops.parse_options.base == BASE_ROMAN_NUMERALS) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Unicode"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_UNICODE); if(settings->evalops.parse_options.base == BASE_UNICODE) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Bijective base-26"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BIJECTIVE_26); if(settings->evalops.parse_options.base == BASE_BIJECTIVE_26) action->setChecked(true);
	action = menu->addAction("φ", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_GOLDEN_RATIO); if(settings->evalops.parse_options.base == BASE_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("ψ", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SUPER_GOLDEN_RATIO); if(settings->evalops.parse_options.base == BASE_SUPER_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("π", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_PI); if(settings->evalops.parse_options.base == BASE_PI) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("e"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_E); if(settings->evalops.parse_options.base == BASE_E) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("√2"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SQRT2); if(settings->evalops.parse_options.base == BASE_SQRT2) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Custom:", "Number base"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_CUSTOM); if(!base_checked) action->setChecked(true); customInputBaseAction = action;
	aw = new QWidgetAction(this);
	aww = new QWidget(this);
	aw->setDefaultWidget(aww);
	awl = new QHBoxLayout(aww);
	QSpinBox *customInputBaseEdit = new QSpinBox(this);
	customInputBaseEdit->setRange(INT_MIN, INT_MAX);
	customInputBaseEdit->setValue(settings->evalops.parse_options.base == BASE_CUSTOM ? (CALCULATOR->customInputBase().isZero() ? 10 : CALCULATOR->customInputBase().intValue()) : ((settings->evalops.parse_options.base >= 2 && settings->evalops.parse_options.base <= 36) ? settings->evalops.parse_options.base : 10)); customInputBaseEdit->setObjectName("spinbox_inbase");
	customInputBaseEdit->setAlignment(Qt::AlignRight);
	connect(customInputBaseEdit, SIGNAL(valueChanged(int)), this, SLOT(onCustomInputBaseChanged(int)));
	awl->addWidget(customInputBaseEdit, 0);
	menu->addAction(aw);
	menu = menu2;
	menu->addSeparator();

	aw = new QWidgetAction(this);
	aww = new QWidget(this);
	aw->setDefaultWidget(aww);
	QGridLayout *awg = new QGridLayout(aww);
	awg->addWidget(new QLabel(tr("Precision:"), this), 0, 0);
	QSpinBox *precisionEdit = new QSpinBox(this); precisionEdit->setObjectName("spinbox_precision");
	precisionEdit->setRange(2, 10000);
	precisionEdit->setValue(CALCULATOR->getPrecision());
	precisionEdit->setAlignment(Qt::AlignRight);
	connect(precisionEdit, SIGNAL(valueChanged(int)), this, SLOT(onPrecisionChanged(int)));
	awg->addWidget(precisionEdit, 0, 1);
	awg->addWidget(new QLabel(tr("Min decimals:"), this), 1, 0);
	QSpinBox *minDecimalsEdit = new QSpinBox(this); minDecimalsEdit->setObjectName("spinbox_mindecimals");
	minDecimalsEdit->setRange(0, 10000);
	minDecimalsEdit->setValue(settings->printops.use_min_decimals ? settings->printops.min_decimals : 0);
	minDecimalsEdit->setAlignment(Qt::AlignRight);
	connect(minDecimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(onMinDecimalsChanged(int)));
	awg->addWidget(minDecimalsEdit, 1, 1);
	awg->addWidget(new QLabel(tr("Max decimals:"), this), 2, 0);
	QSpinBox *maxDecimalsEdit = new QSpinBox(this); maxDecimalsEdit->setObjectName("spinbox_maxdecimals");
	maxDecimalsEdit->setRange(-1, 10000);
	maxDecimalsEdit->setSpecialValueText(tr("off", "Max decimals"));
	maxDecimalsEdit->setValue(settings->printops.use_max_decimals ? settings->printops.max_decimals : -1);
	maxDecimalsEdit->setAlignment(Qt::AlignRight);
	connect(maxDecimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(onMaxDecimalsChanged(int)));
	awg->addWidget(maxDecimalsEdit, 2, 1);
	menu->addAction(aw);

	menu->setMinimumWidth(w);
	tb->addWidget(modeAction);

	toAction = new QAction(LOAD_ICON("convert"), tr("Convert"));
	connect(toAction, SIGNAL(triggered(bool)), this, SLOT(onToActivated()));
	tb->addAction(toAction);
	storeAction = new QAction(LOAD_ICON("document-save"), tr("Store"));
	connect(storeAction, SIGNAL(triggered(bool)), this, SLOT(onStoreActivated()));
	tb->addAction(storeAction);
	functionsAction = new QAction(LOAD_ICON("function"), tr("Functions"));
	connect(functionsAction, SIGNAL(triggered(bool)), this, SLOT(openFunctions()));
	tb->addAction(functionsAction);
	keypadAction = new QAction(LOAD_ICON("keypad"), tr("Keypad"));
	connect(keypadAction, SIGNAL(triggered(bool)), this, SLOT(onKeypadActivated(bool)));
	keypadAction->setCheckable(true);
	tb->addAction(keypadAction);
	basesAction = new QAction(LOAD_ICON("number-bases"), tr("Number bases"));
	connect(basesAction, SIGNAL(triggered(bool)), this, SLOT(onBasesActivated(bool)));
	basesAction->setCheckable(true);
	tb->addAction(basesAction);
	hLayout->addWidget(tb, 0);

	expressionEdit = new ExpressionEdit(this);
	QFont font = expressionEdit->font();
	if(font.pixelSize() >= 0) {
		font.setPixelSize(font.pixelSize() * 1.35);
	} else {
		font.setPointSize(font.pointSize() * 1.35);
	}
	expressionEdit->setFont(font);
	expressionEdit->setFocus();
	ehSplitter->addWidget(expressionEdit);
	historyView = new HistoryView(this);
	historyView->expressionEdit = expressionEdit;

	ehSplitter->addWidget(historyView);
	ehSplitter->setStretchFactor(0, 0);
	ehSplitter->setStretchFactor(1, 1);
	ehSplitter->setCollapsible(0, false);
	ehSplitter->setCollapsible(1, false);

	basesDock = new QDockWidget(tr("Number bases"), this);
	basesDock->setObjectName("number-bases-dock");
	QWidget *basesWidget = new QWidget(this);
	QGridLayout *basesGrid = new QGridLayout(basesWidget);
	binLabel = new QLabel(tr("Binary:"));
	basesGrid->addWidget(binLabel, 0, 0, Qt::AlignTop);
	octLabel = new QLabel(tr("Octal:"));
	basesGrid->addWidget(octLabel, 1, 0);
	decLabel = new QLabel(tr("Decimal:"));
	basesGrid->addWidget(decLabel, 2, 0);
	hexLabel = new QLabel(tr("Hexadecimal:"));
	basesGrid->addWidget(hexLabel, 3, 0);
	binEdit = new QLabel("0000 0000 0000 0000 0000 0000 0000 0000\n0000 0000 0000 0000 0000 0000 0000 0000");
	binEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	binEdit->setFocusPolicy(Qt::NoFocus);
	QFontMetrics fm2(binEdit->font());
	binEdit->setMinimumWidth(fm2.boundingRect("0000 0000 0000 0000 0000 0000 0000 0000").width());
	binEdit->setFixedHeight(fm2.lineSpacing() * 2 + binEdit->frameWidth() * 2 + binEdit->contentsMargins().top() + binEdit->contentsMargins().bottom());
	binEdit->setAlignment(Qt::AlignRight | Qt::AlignTop);
	basesGrid->addWidget(binEdit, 0, 1);
	octEdit = new QLabel("0");
	octEdit->setAlignment(Qt::AlignRight);
	octEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	octEdit->setFocusPolicy(Qt::NoFocus);
	octEdit->setMinimumHeight(fm2.lineSpacing());
	basesGrid->addWidget(octEdit, 1, 1);
	decEdit = new QLabel("0");
	decEdit->setAlignment(Qt::AlignRight);
	decEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	decEdit->setFocusPolicy(Qt::NoFocus);
	decEdit->setMinimumHeight(fm2.lineSpacing());
	basesGrid->addWidget(decEdit, 2, 1);
	hexEdit = new QLabel("0");
	hexEdit->setAlignment(Qt::AlignRight);
	hexEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	hexEdit->setFocusPolicy(Qt::NoFocus);
	hexEdit->setMinimumHeight(fm2.lineSpacing());
	basesGrid->addWidget(hexEdit, 3, 1);
	basesGrid->setRowStretch(4, 1);
	basesDock->setWidget(basesWidget);
	addDockWidget(Qt::TopDockWidgetArea, basesDock);
	basesDock->hide();

	keypad = new KeypadWidget(this);
	keypadDock = new QDockWidget(tr("Keypad"), this);
	keypadDock->setObjectName("keypad-dock");
	keypadDock->setWidget(keypad);
	addDockWidget(Qt::BottomDockWidgetArea, keypadDock);
	keypadDock->hide();

	QLocalServer::removeServer("qalculate-qt");
	server = new QLocalServer(this);
	server->listen("qalculate-qt");
	connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));
	
	mstruct = new MathStructure();
	settings->current_result = mstruct;
	mstruct_exact.setUndefined();
	parsed_mstruct = new MathStructure();
	parsed_tostruct = new MathStructure();
	view_thread = new ViewThread;
	command_thread = new CommandThread;

	settings->printops.can_display_unicode_string_arg = (void*) historyView;

	b_busy = 0;

	expressionEdit->setFocus();

	QFont saved_app_font = QApplication::font();
	if(settings->custom_result_font.empty()) settings->custom_result_font = historyView->font().toString().toStdString();
	if(settings->custom_expression_font.empty()) settings->custom_expression_font = expressionEdit->font().toString().toStdString();
	if(settings->custom_keypad_font.empty()) settings->custom_keypad_font = keypad->font().toString().toStdString();
	if(settings->custom_app_font.empty()) settings->custom_app_font = QApplication::font().toString().toStdString();
	if(settings->use_custom_keypad_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_keypad_font)); keypad->setFont(font);}
	if(settings->use_custom_expression_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_expression_font)); expressionEdit->setFont(font);}
	if(settings->use_custom_result_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_result_font)); historyView->setFont(font);}

	connect(historyView, SIGNAL(insertTextRequested(std::string)), this, SLOT(onInsertTextRequested(std::string)));
	connect(historyView, SIGNAL(insertValueRequested(int)), this, SLOT(onInsertValueRequested(int)));
	connect(expressionEdit, SIGNAL(returnPressed()), this, SLOT(calculate()));
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onExpressionChanged()));
	connect(expressionEdit, SIGNAL(toConversionRequested(std::string)), this, SLOT(onToConversionRequested(std::string)));
	connect(keypadDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onKeypadVisibilityChanged(bool)));
	connect(basesDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onBasesVisibilityChanged(bool)));
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
	connect(keypad, SIGNAL(answerClicked()), this, SLOT(onAnswerClicked()));

	if(!settings->window_geometry.isEmpty()) restoreGeometry(settings->window_geometry);
	else resize(600, 650);
	if(!settings->window_state.isEmpty()) restoreState(settings->window_state);
	if(!settings->splitter_state.isEmpty()) ehSplitter->restoreState(settings->splitter_state);
	bases_shown = !settings->window_state.isEmpty();
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

	if(settings->use_custom_app_font) {
		QTimer *timer = new QTimer();
		timer->setSingleShot(true);
		connect(timer, SIGNAL(timeout()), this, SLOT(onAppFontChanged()));
		timer->start(1);
	}

}
QalculateWindow::~QalculateWindow() {}

void QalculateWindow::onInsertTextRequested(std::string str) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->insertPlainText(QString::fromStdString(unhtmlize(str)));
	expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
	expressionEdit->blockParseStatus(false);
}
void QalculateWindow::showAbout() {
	QMessageBox::about(this, tr("About %1").arg(qApp->applicationDisplayName()), QString("<font size=+2><b>%1 v3.20.0</b></font><br><font size=+1>%2</font><br><<font size=+1><i><a href=\"https://qalculate.github.io/\">https://qalculate.github.io/</a></i></font><br><br>Copyright © 2003-2007, 2008, 2016-2021 Hanna Knutsson<br>%3").arg(qApp->applicationDisplayName()).arg(tr("Powerful and easy to use calculator")).arg(tr("License: GNU General Public License version 2 or later")));
}
void QalculateWindow::onInsertValueRequested(int i) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	Number nr(i, 1, 0);
	expressionEdit->insertPlainText(QString("%1(%2)").arg(QString::fromStdString(settings->f_answer->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name)).arg(QString::fromStdString(print_with_evalops(nr))));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onSymbolClicked(const QString &str) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->insertPlainText(str);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onOperatorClicked(const QString &str) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	bool do_exec = false;
	if(str == "~") {
		expressionEdit->wrapSelection(str, true, false);
	} else if(str == "!") {
		QTextCursor cur = expressionEdit->textCursor();
		do_exec = (str == "!") && cur.hasSelection() && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
		expressionEdit->wrapSelection(str);
	} else if(str == "E") {
		if(expressionEdit->textCursor().hasSelection()) expressionEdit->wrapSelection(SIGN_MULTIPLICATION "10^");
		else expressionEdit->insertPlainText(settings->printops.lower_case_e ? "e" : str);
	} else {
		expressionEdit->wrapSelection(str);
	}
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	if(do_exec) calculate();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}

bool last_is_number(std::string str) {
	str = CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options);
	CALCULATOR->parseSigns(str);
	if(str.empty()) return false;
	return is_not_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1]);
}
void QalculateWindow::onFunctionClicked(MathFunction *f) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	QTextCursor cur = expressionEdit->textCursor();
	bool do_exec = false;
	if(cur.hasSelection()) {
		do_exec = cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
	} else if(last_is_number(expressionEdit->toPlainText().toStdString())) {
		expressionEdit->selectAll();
		do_exec = true;
	}
	expressionEdit->wrapSelection(f->referenceName() == "neg" ? SIGN_MINUS : QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name), true, true);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
	if(do_exec) calculate();
}
void QalculateWindow::onVariableClicked(Variable *v) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->insertPlainText(QString::fromStdString(v->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onUnitClicked(Unit *u) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->insertPlainText(QString::fromStdString(u->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onDelClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	QTextCursor cur = expressionEdit->textCursor();
	if(cur.atEnd()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onBackspaceClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	QTextCursor cur = expressionEdit->textCursor();
	if(!cur.atStart()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onClearClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->clear();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onEqualsClicked() {
	calculate();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onParenthesesClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->smartParentheses();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onBracketsClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->insertBrackets();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onLeftClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->moveCursor(QTextCursor::PreviousCharacter);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onRightClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->moveCursor(QTextCursor::NextCharacter);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onStartClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->moveCursor(QTextCursor::Start);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onEndClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->moveCursor(QTextCursor::End);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
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
void QalculateWindow::onAnswerClicked() {
	if(settings->history_answer.size() > 0) {
		onInsertValueRequested(settings->history_answer.size());
	}
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
	if(command.isEmpty()) return;
	if(command[0] == 'f') {
		command = command.mid(1).trimmed();
		if(!command.isEmpty()) {
			int i = command.indexOf(";");
			if(i > 0) {
				QString file = command.left(i);
				executeFromFile(file);
				command = command.mid(i);
			}
		}
	}
	command = command.mid(1).trimmed();
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
	calculateExpression();
}
void QalculateWindow::calculate(const QString &expression) {
	expressionEdit->setPlainText(expression);
	calculate();
}

void QalculateWindow::setPreviousExpression() {
	if(settings->rpn_mode) {
		expressionEdit->clear();
	} else {
		expressionEdit->blockCompletion();
		expressionEdit->blockParseStatus();
		expressionEdit->setPlainText(QString::fromStdString(previous_expression));
		expressionEdit->selectAll();
		expressionEdit->setExpressionHasChanged(false);
		expressionEdit->blockCompletion(false);
		expressionEdit->blockParseStatus(false);
	}
}

void QalculateWindow::resultFormatUpdated(int delay) {
	if(rfTimer) rfTimer->stop();
	if(block_result_update) return;
	if(delay > 0) {
		if(!rfTimer) {
			rfTimer = new QTimer();
			rfTimer->setSingleShot(true);
			connect(rfTimer, SIGNAL(timeout()), this, SLOT(resultFormatUpdated()));
		}
		rfTimer->start(delay);
		return;
	}
	settings->updateMessagePrintOptions();
	setResult(NULL, true, false, false);
	if(!QToolTip::text().isEmpty()) expressionEdit->displayParseStatus(true);
}
void QalculateWindow::resultDisplayUpdated() {
	resultFormatUpdated();
}
void QalculateWindow::expressionFormatUpdated(bool recalculate) {
	expressionEdit->displayParseStatus(true, !QToolTip::text().isEmpty());
	settings->updateMessagePrintOptions();
	if(!expressionEdit->expressionHasChanged() && !recalculate && !settings->rpn_mode) {
		expressionEdit->clear();
	} else if(!settings->rpn_mode && parsed_mstruct) {
		for(size_t i = 0; i < 5; i++) {
			if(parsed_mstruct->contains(settings->vans[i])) expressionEdit->clear();
		}
	}
	if(!settings->rpn_mode && recalculate) {
		calculateExpression(false);
	}
}
void QalculateWindow::expressionCalculationUpdated(int delay) {
	if(ecTimer) ecTimer->stop();
	if(delay > 0) {
		if(!ecTimer) {
			ecTimer = new QTimer();
			ecTimer->setSingleShot(true);
			connect(ecTimer, SIGNAL(timeout()), this, SLOT(expressionCalculationUpdated()));
		}
		ecTimer->start(delay);
		return;
	}
	expressionEdit->displayParseStatus(true, !QToolTip::text().isEmpty());
	settings->updateMessagePrintOptions();
	if(!settings->rpn_mode) {
		if(parsed_mstruct) {
			for(size_t i = 0; i < 5; i++) {
				if(parsed_mstruct->contains(settings->vans[i])) return;
			}
		}
		calculateExpression(false);
	}
}

int s2b(const std::string &str) {
	if(str.empty()) return -1;
	if(equalsIgnoreCase(str, "yes")) return 1;
	if(equalsIgnoreCase(str, "no")) return 0;
	if(equalsIgnoreCase(str, "true")) return 1;
	if(equalsIgnoreCase(str, "false")) return 0;
	if(equalsIgnoreCase(str, "on")) return 1;
	if(equalsIgnoreCase(str, "off")) return 0;
	if(str.find_first_not_of(SPACES NUMBERS) != std::string::npos) return -1;
	int i = s2i(str);
	if(i > 0) return 1;
	return 0;
}
void base_from_string(std::string str, int &base, Number &nbase, bool input_base = false) {
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") base = BASE_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "roman") || equalsIgnoreCase(str, "roman")) base = BASE_ROMAN_NUMERALS;
	else if(!input_base && (equalsIgnoreCase(str, "time") || equalsIgnoreCase(str, "time"))) base = BASE_TIME;
	else if(str == "b26" || str == "B26") base = BASE_BIJECTIVE_26;
	else if(equalsIgnoreCase(str, "unicode")) base = BASE_UNICODE;
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") base = BASE_SUPER_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "pi") || str == "π") base = BASE_PI;
	else if(str == "e") base = BASE_E;
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") base = BASE_SQRT2;
	else {
		EvaluationOptions eo = settings->evalops;
		eo.parse_options.base = 10;
		MathStructure m;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 350, eo);
		if(CALCULATOR->endTemporaryStopMessages()) {
			base = BASE_CUSTOM;
			nbase.clear();
		} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
			base = m.number().intValue();
		} else {
			base = BASE_CUSTOM;
			nbase = m.number();
		}
	}
}
void set_assumption(const std::string &str, AssumptionType &at, AssumptionSign &as, bool last_of_two = false) {
	if(equalsIgnoreCase(str, "unknown") || str == "0") {
		if(!last_of_two) as = ASSUMPTION_SIGN_UNKNOWN;
		else at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "real")) {
		at = ASSUMPTION_TYPE_REAL;
	} else if(equalsIgnoreCase(str, "number") || str == "num") {
		at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "rational") || str == "rat") {
		at = ASSUMPTION_TYPE_RATIONAL;
	} else if(equalsIgnoreCase(str, "integer") || str == "int") {
		at = ASSUMPTION_TYPE_INTEGER;
	} else if(equalsIgnoreCase(str, "boolean") || str == "bool") {
		at = ASSUMPTION_TYPE_BOOLEAN;
	} else if(equalsIgnoreCase(str, "non-zero") || str == "nz") {
		as = ASSUMPTION_SIGN_NONZERO;
	} else if(equalsIgnoreCase(str, "positive") || str == "pos") {
		as = ASSUMPTION_SIGN_POSITIVE;
	} else if(equalsIgnoreCase(str, "non-negative") || str == "nneg") {
		as = ASSUMPTION_SIGN_NONNEGATIVE;
	} else if(equalsIgnoreCase(str, "negative") || str == "neg") {
		as = ASSUMPTION_SIGN_NEGATIVE;
	} else if(equalsIgnoreCase(str, "non-positive") || str == "npos") {
		as = ASSUMPTION_SIGN_NONPOSITIVE;
	} else {
		CALCULATOR->error(true, "Unrecognized assumption: %s.", str.c_str(), NULL);
	}
}

#define SET_BOOL_D(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; resultDisplayUpdated();}}
#define SET_BOOL_E(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionCalculationUpdated();}}
#define SET_BOOL(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v;}}
#define SET_BOOL_PV(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionFormatUpdated(v);}}
#define SET_BOOL_PT(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionFormatUpdated(true);}}
#define SET_BOOL_PF(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionFormatUpdated(false);}}

QAction *find_child_data(QObject *parent, const QString &name, int v) {
	QActionGroup *group = parent->findChild<QActionGroup*>(name);
	if(!group) return NULL;
	QList<QAction*> actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == v) return actions.at(i);
	}
	return NULL;
}

void QalculateWindow::setOption(std::string str) {
	remove_blank_ends(str);
	gsub(SIGN_MINUS, "-", str);
	std::string svalue, svar;
	bool empty_value = false;
	size_t i_underscore = str.find("_");
	size_t index;
	if(i_underscore != std::string::npos) {
		index = str.find_first_of(SPACES);
		if(index != std::string::npos && i_underscore > index) i_underscore = std::string::npos;
	}
	if(i_underscore == std::string::npos) index = str.find_last_of(SPACES);
	if(index != std::string::npos) {
		svar = str.substr(0, index);
		remove_blank_ends(svar);
		svalue = str.substr(index + 1);
		remove_blank_ends(svalue);
	} else {
		svar = str;
	}
	if(i_underscore != std::string::npos) gsub("_", " ", svar);
	if(svalue.empty()) {
		empty_value = true;
		svalue = "1";
	}

	set_option_place:
	if(equalsIgnoreCase(svar, "base") || equalsIgnoreCase(svar, "input base") || svar == "inbase" || equalsIgnoreCase(svar, "output base") || svar == "outbase") {
		int v = 0;
		bool b_in = equalsIgnoreCase(svar, "input base") || svar == "inbase";
		bool b_out = equalsIgnoreCase(svar, "output base") || svar == "outbase";
		if(equalsIgnoreCase(svalue, "roman")) v = BASE_ROMAN_NUMERALS;
		else if(equalsIgnoreCase(svalue, "bijective") || str == "b26" || str == "B26") v = BASE_BIJECTIVE_26;
		else if(equalsIgnoreCase(svalue, "fp32") || equalsIgnoreCase(svalue, "binary32") || equalsIgnoreCase(svalue, "float")) {if(b_in) v = 0; else v = BASE_FP32;}
		else if(equalsIgnoreCase(svalue, "fp64") || equalsIgnoreCase(svalue, "binary64") || equalsIgnoreCase(svalue, "double")) {if(b_in) v = 0; else v = BASE_FP64;}
		else if(equalsIgnoreCase(svalue, "fp16") || equalsIgnoreCase(svalue, "binary16")) {if(b_in) v = 0; else v = BASE_FP16;}
		else if(equalsIgnoreCase(svalue, "fp80")) {if(b_in) v = 0; else v = BASE_FP80;}
		else if(equalsIgnoreCase(svalue, "fp128") || equalsIgnoreCase(svalue, "binary128")) {if(b_in) v = 0; else v = BASE_FP128;}
		else if(equalsIgnoreCase(svalue, "time")) {if(b_in) v = 0; else v = BASE_TIME;}
		else if(equalsIgnoreCase(svalue, "hex") || equalsIgnoreCase(svalue, "hexadecimal")) v = BASE_HEXADECIMAL;
		else if(equalsIgnoreCase(svalue, "golden") || equalsIgnoreCase(svalue, "golden ratio") || svalue == "φ") v = BASE_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "supergolden") || equalsIgnoreCase(svalue, "supergolden ratio") || svalue == "ψ") v = BASE_SUPER_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "pi") || svalue == "π") v = BASE_PI;
		else if(svalue == "e") v = BASE_E;
		else if(svalue == "sqrt(2)" || svalue == "sqrt 2" || svalue == "sqrt2" || svalue == "√2") v = BASE_SQRT2;
		else if(equalsIgnoreCase(svalue, "unicode")) v = BASE_UNICODE;
		else if(equalsIgnoreCase(svalue, "duo") || equalsIgnoreCase(svalue, "duodecimal")) v = 12;
		else if(equalsIgnoreCase(svalue, "bin") || equalsIgnoreCase(svalue, "binary")) v = BASE_BINARY;
		else if(equalsIgnoreCase(svalue, "oct") || equalsIgnoreCase(svalue, "octal")) v = BASE_OCTAL;
		else if(equalsIgnoreCase(svalue, "dec") || equalsIgnoreCase(svalue, "decimal")) v = BASE_DECIMAL;
		else if(equalsIgnoreCase(svalue, "sexa") || equalsIgnoreCase(svalue, "sexagesimal")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL;}
		else if(equalsIgnoreCase(svalue, "sexa2") || equalsIgnoreCase(svalue, "sexagesimal2")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_2;}
		else if(equalsIgnoreCase(svalue, "sexa3") || equalsIgnoreCase(svalue, "sexagesimal3")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_3;}
		else if(equalsIgnoreCase(svalue, "latitude")) {if(b_in) v = 0; else v = BASE_LATITUDE;}
		else if(equalsIgnoreCase(svalue, "latitude2")) {if(b_in) v = 0; else v = BASE_LATITUDE_2;}
		else if(equalsIgnoreCase(svalue, "longitude")) {if(b_in) v = 0; else v = BASE_LONGITUDE;}
		else if(equalsIgnoreCase(svalue, "longitude2")) {if(b_in) v = 0; else v = BASE_LONGITUDE_2;}
		else if(!b_in && !b_out && (index = svalue.find_first_of(SPACES)) != std::string::npos) {
			str = svalue;
			svalue = str.substr(index + 1, str.length() - (index + 1));
			remove_blank_ends(svalue);
			svar += " ";
			str = str.substr(0, index);
			remove_blank_ends(str);
			svar += str;
			gsub("_", " ", svar);
			if(equalsIgnoreCase(svar, "base display")) {
				goto set_option_place;
			}
			setOption(std::string("inbase ") + svalue);
			setOption(std::string("outbase ") + str);
			return;
		} else if(!empty_value) {
			MathStructure m;
			EvaluationOptions eo = settings->evalops;
			eo.parse_options.base = 10;
			eo.approximation = APPROXIMATION_TRY_EXACT;
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(svalue, eo.parse_options), 500, eo);
			if(CALCULATOR->endTemporaryStopMessages()) {
				v = 0;
			} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
				v = m.number().intValue();
			} else if(m.isNumber() && (b_in || ((!m.number().isNegative() || m.number().isInteger()) && (m.number() > 1 || m.number() < -1)))) {
				v = BASE_CUSTOM;
				if(b_in) CALCULATOR->setCustomInputBase(m.number());
				else CALCULATOR->setCustomOutputBase(m.number());
			}
		}
		if(v == 0) {
			CALCULATOR->error(true, "Illegal base: %s.", svalue.c_str(), NULL);
		} else if(b_in) {
			if(v == BASE_CUSTOM || v != settings->evalops.parse_options.base) {
				settings->evalops.parse_options.base = v;
				QAction *action = find_child_data(this, "group_intbase", v);
				if(!action) action = find_child_data(this, "group_inbase", BASE_CUSTOM);
				if(action) {
					action->blockSignals(true);
					action->setChecked(true);
					action->blockSignals(false);
					if(action->data().toInt() == BASE_CUSTOM && (v == BASE_CUSTOM || (v >= 2 && v <= 36))) {
						QSpinBox *w = findChild<QSpinBox*>("spinbox_inbase");
						if(w) {
							w->blockSignals(true);
							w->setValue(v == BASE_CUSTOM ? (CALCULATOR->customInputBase().isZero() ? 10 : CALCULATOR->customInputBase().intValue()) : v);
							w->blockSignals(false);
						}
					}
				}
				expressionFormatUpdated(false);
			}
		} else {
			if(v == BASE_CUSTOM || v != settings->printops.base) {
				settings->printops.base = v;
				to_base = 0;
				to_bits = 0;
				QAction *action = find_child_data(this, "group_outbase", v);
				if(!action) action = find_child_data(this, "group_outbase", BASE_CUSTOM);
				if(action) {
					action->blockSignals(true);
					action->setChecked(true);
					action->blockSignals(false);
					if(action->data().toInt() == BASE_CUSTOM && (v == BASE_CUSTOM || (v >= 2 && v <= 36))) {
						QSpinBox *w = findChild<QSpinBox*>("spinbox_outbase");
						if(w) {
							w->blockSignals(true);
							w->setValue(v == BASE_CUSTOM ? (CALCULATOR->customOutputBase().isZero() ? 10 : CALCULATOR->customOutputBase().intValue()) : v);
							w->blockSignals(false);
						}
					}
				}
				resultFormatUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "assumptions") || svar == "ass") {
		size_t i = svalue.find_first_of(SPACES);
		AssumptionType at = CALCULATOR->defaultAssumptions()->type();
		AssumptionSign as = CALCULATOR->defaultAssumptions()->sign();
		if(i != std::string::npos) {
			set_assumption(svalue.substr(0, i), at, as, false);
			set_assumption(svalue.substr(i + 1, svalue.length() - (i + 1)), at, as, true);
		} else {
			set_assumption(svalue, at, as, false);
		}
		QAction *action = find_child_data(this, "group_type", CALCULATOR->defaultAssumptions()->type());
		if(action) {
			action->blockSignals(true);
			action->setChecked(true);
			action->blockSignals(false);
		}
		action = find_child_data(this, "group_sign", CALCULATOR->defaultAssumptions()->sign());
		if(action) {
			action->blockSignals(true);
			action->setChecked(true);
			action->blockSignals(false);
		}
	} else if(equalsIgnoreCase(svar, "all prefixes") || svar == "allpref") SET_BOOL_D(settings->printops.use_all_prefixes)
	else if(equalsIgnoreCase(svar, "complex numbers") || svar == "cplx") SET_BOOL_E(settings->evalops.allow_complex)
	else if(equalsIgnoreCase(svar, "excessive parentheses") || svar == "expar") SET_BOOL_D(settings->printops.excessive_parenthesis)
	else if(equalsIgnoreCase(svar, "functions") || svar == "func") SET_BOOL_PV(settings->evalops.parse_options.functions_enabled)
	else if(equalsIgnoreCase(svar, "infinite numbers") || svar == "inf") SET_BOOL_E(settings->evalops.allow_infinite)
	else if(equalsIgnoreCase(svar, "show negative exponents") || svar == "negexp") SET_BOOL_D(settings->printops.negative_exponents)
	else if(equalsIgnoreCase(svar, "minus last") || svar == "minlast") {
		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(settings->printops.sort_options.minus_last != v) {settings->printops.sort_options.minus_last = v; resultDisplayUpdated();}}
	} else if(equalsIgnoreCase(svar, "assume nonzero denominators") || svar == "nzd") SET_BOOL_E(settings->evalops.assume_denominators_nonzero)
	else if(equalsIgnoreCase(svar, "warn nonzero denominators") || svar == "warnnzd") SET_BOOL_E(settings->evalops.warn_about_denominators_assumed_nonzero)
	else if(equalsIgnoreCase(svar, "prefixes") || svar == "pref") SET_BOOL_D(settings->printops.use_unit_prefixes)
	else if(equalsIgnoreCase(svar, "binary prefixes") || svar == "binpref") {
		bool b = CALCULATOR->usesBinaryPrefixes() > 0;
		SET_BOOL(b)
		if(b != (CALCULATOR->usesBinaryPrefixes() > 0)) {
			CALCULATOR->useBinaryPrefixes(b ? 1 : 0);
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "denominator prefixes") || svar == "denpref") SET_BOOL_D(settings->printops.use_denominator_prefix)
	else if(equalsIgnoreCase(svar, "place units separately") || svar == "unitsep") SET_BOOL_D(settings->printops.place_units_separately)
	else if(equalsIgnoreCase(svar, "calculate variables") || svar == "calcvar") SET_BOOL_E(settings->evalops.calculate_variables)
	else if(equalsIgnoreCase(svar, "calculate functions") || svar == "calcfunc") SET_BOOL_E(settings->evalops.calculate_functions)
	else if(equalsIgnoreCase(svar, "sync units") || svar == "sync") SET_BOOL_E(settings->evalops.sync_units)
	else if(equalsIgnoreCase(svar, "temperature calculation") || svar == "temp")  {
		int v = -1;
		if(equalsIgnoreCase(svalue, "relative")) v = TEMPERATURE_CALCULATION_RELATIVE;
		else if(equalsIgnoreCase(svalue, "hybrid")) v = TEMPERATURE_CALCULATION_HYBRID;
		else if(equalsIgnoreCase(svalue, "absolute")) v = TEMPERATURE_CALCULATION_ABSOLUTE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) v);
			settings->tc_set = true;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "round to even") || svar == "rndeven") SET_BOOL_D(settings->printops.round_halfway_to_even)
	else if(equalsIgnoreCase(svar, "rpn syntax") || svar == "rpnsyn") {
		bool b = (settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN);
		SET_BOOL(b)
		if(b != (settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN)) {
			if(b) {
				settings->evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
			} else {
				settings->evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
			}
			expressionFormatUpdated(false);
		}
	} else if(equalsIgnoreCase(svar, "rpn") && svalue.find(" ") == std::string::npos) {
		//SET_BOOL_MENU("menu_item_rpn_mode")
	} else if(equalsIgnoreCase(svar, "short multiplication") || svar == "shortmul") SET_BOOL_D(settings->printops.short_multiplication)
	else if(equalsIgnoreCase(svar, "lowercase e") || svar == "lowe") SET_BOOL_D(settings->printops.lower_case_e)
	else if(equalsIgnoreCase(svar, "lowercase numbers") || svar == "lownum") SET_BOOL_D(settings->printops.lower_case_numbers)
	else if(equalsIgnoreCase(svar, "imaginary j") || svar == "imgj") {
		Variable *v_i = CALCULATOR->getVariableById(VARIABLE_ID_I);
		if(v_i) {
		bool b = v_i->hasName("j") > 0;
			SET_BOOL(b)
			if(b) {
				ExpressionName ename = v_i->getName(1);
				ename.name = "j";
				ename.reference = false;
				v_i->addName(ename, 1, true);
				v_i->setChanged(false);
			} else {
				v_i->clearNonReferenceNames();
				v_i->setChanged(false);
			}
			expressionFormatUpdated(false);
		}
	} else if(equalsIgnoreCase(svar, "base display") || svar == "basedisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = BASE_DISPLAY_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "normal")) v = BASE_DISPLAY_NORMAL;
		else if(equalsIgnoreCase(svalue, "alternative")) v = BASE_DISPLAY_ALTERNATIVE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.base_display = (BaseDisplay) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "two's complement") || svar == "twos") SET_BOOL_D(settings->printops.twos_complement)
	else if(equalsIgnoreCase(svar, "hexadecimal two's") || svar == "hextwos") SET_BOOL_D(settings->printops.hexadecimal_twos_complement)
	else if(equalsIgnoreCase(svar, "digit grouping") || svar =="group") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DIGIT_GROUPING_NONE;
		else if(equalsIgnoreCase(svalue, "none")) v = DIGIT_GROUPING_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "standard") || equalsIgnoreCase(svalue, "on")) v = DIGIT_GROUPING_STANDARD;
		else if(equalsIgnoreCase(svalue, "locale")) v = DIGIT_GROUPING_LOCALE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < DIGIT_GROUPING_NONE || v > DIGIT_GROUPING_LOCALE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.digit_grouping = (DigitGrouping) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "spell out logical") || svar == "spellout") SET_BOOL_D(settings->printops.spell_out_logical_operators)
	else if((equalsIgnoreCase(svar, "ignore dot") || svar == "nodot") && CALCULATOR->getDecimalPoint() != DOT) {
		SET_BOOL_PF(settings->evalops.parse_options.dot_as_separator)
		settings->dot_question_asked = true;
	} else if((equalsIgnoreCase(svar, "ignore comma") || svar == "nocomma") && CALCULATOR->getDecimalPoint() != COMMA) {
		SET_BOOL(settings->evalops.parse_options.comma_as_separator)
		CALCULATOR->useDecimalPoint(settings->evalops.parse_options.comma_as_separator);
		expressionFormatUpdated(false);
	} else if(equalsIgnoreCase(svar, "decimal comma")) {
		int v = -2;
		if(equalsIgnoreCase(svalue, "off")) v = 0;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = 1;
		else if(equalsIgnoreCase(svalue, "locale")) v = -1;
		else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < -1 || v > 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->decimal_comma = v;
			if(settings->decimal_comma > 0) CALCULATOR->useDecimalComma();
			else if(settings->decimal_comma == 0) CALCULATOR->useDecimalPoint(settings->evalops.parse_options.comma_as_separator);
			if(v >= 0) {
				expressionFormatUpdated(false);
				resultDisplayUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "limit implicit multiplication") || svar == "limimpl") {
		int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str());} else {settings->printops.limit_implicit_multiplication = v; settings->evalops.parse_options.limit_implicit_multiplication = v; expressionFormatUpdated(true);}
	} else if(equalsIgnoreCase(svar, "spacious") || svar == "space") SET_BOOL_D(settings->printops.spacious)
	else if(equalsIgnoreCase(svar, "unicode") || svar == "uni") {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str());
		} else {
			settings->printops.use_unicode_signs = v; resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "units") || svar == "unit") SET_BOOL_PV(settings->evalops.parse_options.units_enabled)
	else if(equalsIgnoreCase(svar, "unknowns") || svar == "unknown") SET_BOOL_PV(settings->evalops.parse_options.unknowns_enabled)
	else if(equalsIgnoreCase(svar, "variables") || svar == "var") SET_BOOL_PV(settings->evalops.parse_options.variables_enabled)
	else if(equalsIgnoreCase(svar, "abbreviations") || svar == "abbr" || svar == "abbrev") SET_BOOL_D(settings->printops.abbreviate_names)
	else if(equalsIgnoreCase(svar, "show ending zeroes") || svar == "zeroes") SET_BOOL_D(settings->printops.show_ending_zeroes)
	else if(equalsIgnoreCase(svar, "repeating decimals") || svar == "repdeci") SET_BOOL_D(settings->printops.indicate_infinite_series)
	else if(equalsIgnoreCase(svar, "angle unit") || svar == "angle") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rad") || equalsIgnoreCase(svalue, "radians")) v = ANGLE_UNIT_RADIANS;
		else if(equalsIgnoreCase(svalue, "deg") || equalsIgnoreCase(svalue, "degrees")) v = ANGLE_UNIT_DEGREES;
		else if(equalsIgnoreCase(svalue, "gra") || equalsIgnoreCase(svalue, "gradians")) v = ANGLE_UNIT_GRADIANS;
		else if(equalsIgnoreCase(svalue, "none")) v = ANGLE_UNIT_NONE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 3) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			QAction *w = NULL;
			if(v == ANGLE_UNIT_DEGREES) w = findChild<QAction*>("action_degrees");
			else if(v == ANGLE_UNIT_RADIANS)w = findChild<QAction*>("action_radians");
			else if(v == ANGLE_UNIT_GRADIANS) w = findChild<QAction*>("action_gradians");
			if(w) {
				w->blockSignals(true);
				w->setChecked(true);
				w->blockSignals(false);
			}
			settings->evalops.parse_options.angle_unit = (AngleUnit) v;
			expressionFormatUpdated(true);
		}
	} else if(equalsIgnoreCase(svar, "caret as xor") || equalsIgnoreCase(svar, "xor^")) SET_BOOL_PT(settings->caret_as_xor)
	else if(equalsIgnoreCase(svar, "parsing mode") || svar == "parse" || svar == "syntax") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = PARSING_MODE_ADAPTIVE;
		else if(equalsIgnoreCase(svalue, "implicit first")) v = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
		else if(equalsIgnoreCase(svalue, "conventional")) v = PARSING_MODE_CONVENTIONAL;
		else if(equalsIgnoreCase(svalue, "chain")) v = PARSING_MODE_CHAIN;
		else if(equalsIgnoreCase(svalue, "rpn")) v = PARSING_MODE_RPN;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < PARSING_MODE_ADAPTIVE || v > PARSING_MODE_RPN) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.parse_options.parsing_mode = (ParsingMode) v;
			expressionFormatUpdated(true);
		}
	} else if(equalsIgnoreCase(svar, "update exchange rates") || svar == "upxrates") {
		int v = -2;
		if(equalsIgnoreCase(svalue, "never")) {
			settings->auto_update_exchange_rates = 0;
		} else if(equalsIgnoreCase(svalue, "ask")) {
			CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
		} else {
			v = s2i(svalue);
			if(empty_value) v = 7;
			if(v < 0) CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
			else settings->auto_update_exchange_rates = v;
		}
	} else if(equalsIgnoreCase(svar, "multiplication sign") || svar == "mulsign") {
		int v = -1;
		if(svalue == SIGN_MULTIDOT || svalue == ".") v = MULTIPLICATION_SIGN_DOT;
		else if(svalue == SIGN_MIDDLEDOT) v = MULTIPLICATION_SIGN_ALTDOT;
		else if(svalue == SIGN_MULTIPLICATION || svalue == "x") v = MULTIPLICATION_SIGN_X;
		else if(svalue == "*") v = MULTIPLICATION_SIGN_ASTERISK;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < MULTIPLICATION_SIGN_ASTERISK || v > MULTIPLICATION_SIGN_ALTDOT) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.multiplication_sign = (MultiplicationSign) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "division sign") || svar == "divsign") {
		int v = -1;
		if(svalue == SIGN_DIVISION_SLASH) v = DIVISION_SIGN_DIVISION_SLASH;
		else if(svalue == SIGN_DIVISION) v = DIVISION_SIGN_DIVISION;
		else if(svalue == "/") v = DIVISION_SIGN_SLASH;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.division_sign = (DivisionSign) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "approximation") || svar == "appr" || svar == "approx") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "exact")) v = APPROXIMATION_EXACT;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(equalsIgnoreCase(svalue, "dual")) v = APPROXIMATION_APPROXIMATE + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "try exact") || svalue == "try") v = APPROXIMATION_TRY_EXACT;
		else if(equalsIgnoreCase(svalue, "approximate") || svalue == "approx") v = APPROXIMATION_APPROXIMATE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v > APPROXIMATION_APPROXIMATE + 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			QAction *w = NULL;
			if(v < 0) w = findChild<QAction*>("action_autoappr");
			else if(v == APPROXIMATION_APPROXIMATE + 1) w = findChild<QAction*>("action_dualappr");
			else if(v == APPROXIMATION_EXACT) w = findChild<QAction*>("action_exact");
			else if(v == APPROXIMATION_TRY_EXACT) w = findChild<QAction*>("action_approximate");
			else if(v == APPROXIMATION_APPROXIMATE) w = findChild<QAction*>("action_approximate");
			if(w) {
				w->blockSignals(true);
				w->setChecked(true);
				w->blockSignals(false);
			}
			if(v < 0) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				settings->dual_approximation = -1;
			} else if(v == APPROXIMATION_APPROXIMATE + 1) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				settings->dual_approximation = 1;
			} else {
				settings->evalops.approximation = (ApproximationMode) v;
				settings->dual_approximation = 0;
			}
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "interval calculation") || svar == "ic" || equalsIgnoreCase(svar, "uncertainty propagation") || svar == "up") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "variance formula") || equalsIgnoreCase(svalue, "variance")) v = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		else if(equalsIgnoreCase(svalue, "interval arithmetic") || svalue == "iv") v = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < INTERVAL_CALCULATION_NONE || v > INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.interval_calculation = (IntervalCalculation) v;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "autoconversion") || svar == "conv") {
		int v = -1;
		MixedUnitsConversion muc = MIXED_UNITS_CONVERSION_DEFAULT;
		if(equalsIgnoreCase(svalue, "none")) {v = POST_CONVERSION_NONE;  muc = MIXED_UNITS_CONVERSION_NONE;}
		else if(equalsIgnoreCase(svalue, "best")) v = POST_CONVERSION_OPTIMAL_SI;
		else if(equalsIgnoreCase(svalue, "optimalsi") || svalue == "si") v = POST_CONVERSION_OPTIMAL_SI;
		else if(empty_value || equalsIgnoreCase(svalue, "optimal")) v = POST_CONVERSION_OPTIMAL;
		else if(equalsIgnoreCase(svalue, "base")) v = POST_CONVERSION_BASE;
		else if(equalsIgnoreCase(svalue, "mixed")) v = POST_CONVERSION_OPTIMAL + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v == 1) v = 3;
			else if(v == 3) v = 1;
		}
		if(v == POST_CONVERSION_OPTIMAL + 1) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_DEFAULT;
		} else if(v == 0) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_NONE;
		}
		if(v < 0 || v > POST_CONVERSION_OPTIMAL) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.auto_post_conversion = (AutoPostConversion) v;
			settings->evalops.mixed_units_conversion = muc;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "currency conversion") || svar == "curconv") SET_BOOL_E(settings->evalops.local_currency_conversion)
	else if(equalsIgnoreCase(svar, "algebra mode") || svar == "alg") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = STRUCTURING_NONE;
		else if(equalsIgnoreCase(svalue, "simplify") || equalsIgnoreCase(svalue, "expand")) v = STRUCTURING_SIMPLIFY;
		else if(equalsIgnoreCase(svalue, "factorize") || svalue == "factor") v = STRUCTURING_FACTORIZE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > STRUCTURING_FACTORIZE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.structuring = (StructuringMode) v;
			settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "exact")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			setOption("approx exact");
		}
	} else if(equalsIgnoreCase(svar, "ignore locale")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v != settings->ignore_locale) {
			if(v > 0) {
				settings->ignore_locale = true;
			} else {
				settings->ignore_locale = false;
			}
			CALCULATOR->error(false, "Please restart the program for the change to take effect.", NULL);
		}
	} else if(equalsIgnoreCase(svar, "save mode")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v > 0) {
			settings->save_mode_on_exit = true;
		} else {
			settings->save_mode_on_exit = false;
		}
	} else if(equalsIgnoreCase(svar, "save definitions") || svar == "save defs") {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v > 0) {
			settings->save_defs_on_exit = true;
		} else {
			settings->save_defs_on_exit = false;
		}
	} else if(equalsIgnoreCase(svar, "scientific notation") || svar == "exp mode" || svar == "exp") {
		int v = -1;
		bool valid = true;
		if(equalsIgnoreCase(svalue, "off")) v = EXP_NONE;
		else if(equalsIgnoreCase(svalue, "auto")) v = EXP_PRECISION;
		else if(equalsIgnoreCase(svalue, "pure")) v = EXP_PURE;
		else if(empty_value || equalsIgnoreCase(svalue, "scientific")) v = EXP_SCIENTIFIC;
		else if(equalsIgnoreCase(svalue, "engineering")) v = EXP_BASE_3;
		else if(svalue.find_first_not_of(SPACES NUMBERS MINUS) == std::string::npos) v = s2i(svalue);
		else valid = false;
		if(valid) {
			settings->printops.min_exp = v;
			resultFormatUpdated();
		} else {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		}
	} else if(equalsIgnoreCase(svar, "precision") || svar == "prec") {
		int v = 0;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		if(v < 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			QSpinBox *w = findChild<QSpinBox*>("spinbox_precision");
			if(w) {
				w->blockSignals(true);
				w->setValue(v);
				w->blockSignals(false);
			}
			CALCULATOR->setPrecision(v);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "interval display") || svar == "ivdisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = 0;
		else if(equalsIgnoreCase(svalue, "significant")) v = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS + 1;
		else if(equalsIgnoreCase(svalue, "interval")) v = INTERVAL_DISPLAY_INTERVAL + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "plusminus")) v = INTERVAL_DISPLAY_PLUSMINUS + 1;
		else if(equalsIgnoreCase(svalue, "midpoint")) v = INTERVAL_DISPLAY_MIDPOINT + 1;
		else if(equalsIgnoreCase(svalue, "upper")) v = INTERVAL_DISPLAY_UPPER + 1;
		else if(equalsIgnoreCase(svalue, "lower")) v = INTERVAL_DISPLAY_LOWER + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v == 0) {
			settings->adaptive_interval_display = true;
			settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
			resultFormatUpdated();
		} else {
			v--;
			if(v < INTERVAL_DISPLAY_SIGNIFICANT_DIGITS || v > INTERVAL_DISPLAY_UPPER) {
				CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
			} else {
				settings->adaptive_interval_display = false;
				settings->printops.interval_display = (IntervalDisplay) v;
				resultFormatUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "interval arithmetic") || svar == "ia" || svar == "interval") {
		bool b = CALCULATOR->usesIntervalArithmetic();
		SET_BOOL(b)
		if(b != CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->useIntervalArithmetic(b);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "variable units") || svar == "varunits") {
		bool b = CALCULATOR->variableUnitsEnabled();
		SET_BOOL(b)
		if(b != CALCULATOR->variableUnitsEnabled()) {
			CALCULATOR->setVariableUnitsEnabled(b);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "color")) {
		int v = -1;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->colorize_result = v;
		}
	} else if(equalsIgnoreCase(svar, "max decimals") || svar == "maxdeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		QSpinBox *w = findChild<QSpinBox*>("spinbox_maxdecimals");
		if(w) {
			w->blockSignals(true);
			w->setValue(v < 0 ? -1 : v);
			w->blockSignals(false);
		}
		settings->printops.use_max_decimals = (v >= 0);
		settings->printops.max_decimals = v;
		resultFormatUpdated();
	} else if(equalsIgnoreCase(svar, "min decimals") || svar == "mindeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		QSpinBox *w = findChild<QSpinBox*>("spinbox_mindecimals");
		if(w) {
			w->blockSignals(true);
			w->setValue(v < 0 ? 0 : v);
			w->blockSignals(false);
		}
		settings->printops.use_min_decimals = (v > 0);
		settings->printops.min_decimals = v;
		resultFormatUpdated();
	} else if(equalsIgnoreCase(svar, "fractions") || svar == "fr") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = FRACTION_DECIMAL;
		else if(equalsIgnoreCase(svalue, "exact")) v = FRACTION_DECIMAL_EXACT;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = FRACTION_FRACTIONAL;
		else if(equalsIgnoreCase(svalue, "combined") || equalsIgnoreCase(svalue, "mixed")) v = FRACTION_COMBINED;
		else if(equalsIgnoreCase(svalue, "long")) v = FRACTION_COMBINED + 1;
		else if(equalsIgnoreCase(svalue, "dual")) v = FRACTION_COMBINED + 2;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v > FRACTION_COMBINED + 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v < 0 || v > FRACTION_COMBINED + 1) {
			settings->printops.restrict_fraction_length = (v == FRACTION_FRACTIONAL || v == FRACTION_COMBINED);
			if(v < 0) settings->dual_fraction = -1;
			else if(v == FRACTION_COMBINED + 2) settings->dual_fraction = 1;
			else settings->dual_fraction = 0;
			if(v == FRACTION_COMBINED + 1) v = FRACTION_FRACTIONAL;
			else if(v < 0 || v == FRACTION_COMBINED + 2) v = FRACTION_DECIMAL;
			settings->printops.number_fraction_format = (NumberFractionFormat) v;
			resultFormatUpdated();
		}
	} else if(equalsIgnoreCase(svar, "complex form") || svar == "cplxform") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rectangular") || equalsIgnoreCase(svalue, "cartesian") || svalue == "rect") v = COMPLEX_NUMBER_FORM_RECTANGULAR;
		else if(equalsIgnoreCase(svalue, "exponential") || svalue == "exp") v = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		else if(equalsIgnoreCase(svalue, "polar")) v = COMPLEX_NUMBER_FORM_POLAR;
		else if(equalsIgnoreCase(svalue, "angle") || equalsIgnoreCase(svalue, "phasor")) v = COMPLEX_NUMBER_FORM_CIS + 1;
		else if(svar == "cis") v = COMPLEX_NUMBER_FORM_CIS;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 4) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->complex_angle_form = (v > 3);
			if(v == 4) v--;
			settings->evalops.complex_number_form = (ComplexNumberForm) v;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "read precision") || svar == "readprec") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DONT_READ_PRECISION;
		else if(equalsIgnoreCase(svalue, "always")) v = ALWAYS_READ_PRECISION;
		else if(empty_value || equalsIgnoreCase(svalue, "when decimals") || equalsIgnoreCase(svalue, "on")) v = READ_PRECISION_WHEN_DECIMALS;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.parse_options.read_precision = (ReadPrecisionMode) v;
			expressionFormatUpdated(true);
		}
	} else {
		if(i_underscore == std::string::npos) {
			if(index != std::string::npos) {
				if((index = svar.find_last_of(SPACES)) != std::string::npos) {
					svar = svar.substr(0, index);
					remove_blank_ends(svar);
					str = str.substr(index + 1);
					remove_blank_ends(str);
					svalue = str;
					gsub("_", " ", svar);
					goto set_option_place;
				}
			}
			if(!empty_value && !svalue.empty()) {
				svar += " ";
				svar += svalue;
				svalue = "1";
				empty_value = true;
				goto set_option_place;
			}
		}
		CALCULATOR->error(true, "Unrecognized option: %s.", svar.c_str(), NULL);
	}
}

void QalculateWindow::calculateExpression(bool force, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, std::string execute_str, std::string str, bool check_exrates) {

	//if(block_expression_execution || exit_in_progress) return;

	std::string saved_execute_str = execute_str;

	if(b_busy) return;

	expressionEdit->hideCompletion();

	b_busy++;

	bool do_factors = false, do_pfe = false, do_expand = false, do_ceu = execute_str.empty(), do_bases = false, do_calendars = false;
	if(do_stack && !settings->rpn_mode) do_stack = false;
	if(do_stack && do_mathoperation && f && stack_index == 0) do_stack = false;
	if(!do_stack) stack_index = 0;

	//if(!mbak_convert.isUndefined() && stack_index == 0) mbak_convert.setUndefined();

	if(execute_str.empty()) {
		to_fraction = false; to_prefix = 0; to_base = 0; to_bits = 0; to_nbase.clear(); to_caf = -1;
	}
	bool current_expr = false;
	if(str.empty() && !do_mathoperation) {
		if(do_stack) {
			/*GtkTreeIter iter;
			gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, stack_index);
			gchar *gstr;
			gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
			str = gstr;
			g_free(gstr);*/
		} else {
			current_expr = true;
			str = expressionEdit->toPlainText().toStdString();
			if(!force && (expressionEdit->expressionHasChanged() || str.find_first_not_of(SPACES) == std::string::npos)) {
				b_busy--;
				return;
			}
			expressionEdit->setExpressionHasChanged(false);
			if(!do_mathoperation && !str.empty() && !block_expression_history) expressionEdit->addToHistory();
			askDot(str);
		}
	}

	std::string to_str, str_conv;

	if(execute_str.empty()) {
		bool double_tag = false;
		to_str = CALCULATOR->parseComments(str, settings->evalops.parse_options, &double_tag);
		if(!to_str.empty()) {
			if(str.empty()) {
				if(!double_tag) {
					expressionEdit->clear();
					CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
					historyView->addMessages();
					b_busy--;
					return;
				}
				execute_str = CALCULATOR->f_message->referenceName();
				execute_str += "(";
				execute_str += to_str;
				execute_str += ")";
			} else {
				CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
			}
		}
		// qalc command
		bool b_command = false;
		if(str[0] == '/' && str.length() > 1) {
			size_t i = str.find_first_not_of(SPACES, 1);
			if(i != std::string::npos && str[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, str[i])) {
				b_command = true;
			}
		}
		if(b_command) {
			str.erase(0, 1);
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			std::string scom;
			if(ispace == std::string::npos) {
				scom = "";
			} else {
				scom = str.substr(1, ispace);
			}
			if(equalsIgnoreCase(scom, "convert") || equalsIgnoreCase(scom, "to")) {
				str = std::string("to") + str.substr(ispace, slen - ispace);
				b_command = false;
			} else if((str.length() > 2 && str[0] == '-' && str[1] == '>') || (str.length() > 3 && str[0] == '\xe2' && ((str[1] == '\x86' && str[2] == '\x92') || (str[1] == '\x9e' && (unsigned char) str[2] >= 148 && (unsigned char) str[3] <= 191)))) {
				b_command = false;
			} else if(str == "M+" || str == "M-" || str == "M−" || str == "MS" || str == "MC") {
				b_command = false;
			}
		}
		if(b_command) {
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			std::string scom;
			if(ispace == std::string::npos) {
				scom = "";
			} else {
				scom = str.substr(0, ispace);
			}
			b_busy--;
			if(equalsIgnoreCase(scom, "set")) {
				if(current_expr) setPreviousExpression();
				str = str.substr(ispace + 1, slen - (ispace + 1));
				setOption(str);
			} else if(equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(equalsIgnoreCase(str, "mode")) {
					settings->savePreferences();
				} else if(equalsIgnoreCase(str, "definitions")) {
					if(!CALCULATOR->saveDefinitions()) {
						QMessageBox::critical(this, tr("Error"), tr("Couldn't write definitions"), QMessageBox::Ok);
					}
				} else {
					std::string name = str, cat, title;
					if(str[0] == '\"') {
						size_t i = str.find('\"', 1);
						if(i != std::string::npos) {
							name = str.substr(1, i - 1);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
					} else {
						size_t i = str.find_first_of(SPACES, 1);
						if(i != std::string::npos) {
							name = str.substr(0, i);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
						bool catset = false;
						if(str.empty()) {
							cat = CALCULATOR->temporaryCategory();
						} else {
							if(str[0] == '\"') {
								size_t i = str.find('\"', 1);
								if(i != std::string::npos) {
									cat = str.substr(1, i - 1);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							} else {
								size_t i = str.find_first_of(SPACES, 1);
								if(i != std::string::npos) {
									cat = str.substr(0, i);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							}
							catset = true;
						}
						bool b = true;
						if(!CALCULATOR->variableNameIsValid(name)) {
							CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
							b = false;
						}
						Variable *v = NULL;
						if(b) v = CALCULATOR->getActiveVariable(name);
						if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
							CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
							b = false;
						}
						if(b) {
							if(v && v->isLocal() && v->isKnown()) {
								if(catset) v->setCategory(cat);
								if(!title.empty()) v->setTitle(title);
								((KnownVariable*) v)->set(*mstruct);
								if(v->countNames() == 0) {
									ExpressionName ename(name);
									ename.reference = true;
									v->setName(ename, 1);
								} else {
									v->setName(name, 1);
								}
							} else {
								CALCULATOR->addVariable(new KnownVariable(cat, name, *mstruct, title));
							}
							expressionEdit->updateCompletion();
						}
					}
				}
			} else if(equalsIgnoreCase(scom, "variable")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				std::string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != std::string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != std::string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->variableNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				Variable *v = NULL;
				if(b) v = CALCULATOR->getActiveVariable(name);
				if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
					CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(v && v->isLocal() && v->isKnown()) {
						((KnownVariable*) v)->set(expr);
						if(v->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							v->setName(ename, 1);
						} else {
							v->setName(name, 1);
						}
					} else {
						CALCULATOR->addVariable(new KnownVariable("", name, expr));
					}
					expressionEdit->updateCompletion();
				}
			} else if(equalsIgnoreCase(scom, "function")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				std::string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != std::string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != std::string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->functionNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				MathFunction *f = CALCULATOR->getActiveFunction(name);
				if(b && ((!f && CALCULATOR->functionNameTaken(name)) || (f && (!f->isLocal() || f->subtype() != SUBTYPE_USER_FUNCTION)))) {
					CALCULATOR->error(true, "A function with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(expr.find("\\") == std::string::npos) {
						gsub("x", "\\x", expr);
						gsub("y", "\\y", expr);
						gsub("z", "\\z", expr);
					}
					if(f && f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) {
						((UserFunction*) f)->setFormula(expr);
						if(f->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							f->setName(ename, 1);
						} else {
							f->setName(name, 1);
						}
					} else {
						CALCULATOR->addFunction(new UserFunction("", name, expr));
					}
					expressionEdit->updateCompletion();
					if(functionsDialog) functionsDialog->updateFunctions();
				}
			} else if(equalsIgnoreCase(scom, "delete")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				Variable *v = CALCULATOR->getActiveVariable(str);
				if(v && v->isLocal()) {
					v->destroy();
					expressionEdit->updateCompletion();
				} else {
					MathFunction *f = CALCULATOR->getActiveFunction(str);
					if(f && f->isLocal()) {
						f->destroy();
						expressionEdit->updateCompletion();
						if(functionsDialog) functionsDialog->updateFunctions();
					} else {
						CALCULATOR->error(true, "No user-defined variable or function with the specified name (%s) exist.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(scom, "base")) {
				if(current_expr) setPreviousExpression();
				setOption(str);
			} else if(equalsIgnoreCase(scom, "assume")) {
				if(current_expr) setPreviousExpression();
				std::string str2 = "assumptions ";
				setOption(str2 + str.substr(ispace + 1, slen - (ispace + 1)));
			} else if(equalsIgnoreCase(scom, "rpn")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				/*if(equalsIgnoreCase(str, "syntax")) {
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), FALSE);
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
				} else if(equalsIgnoreCase(str, "stack")) {
					if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
					}
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), TRUE);
				} else {
					int v = s2b(str);
					if(v < 0) {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					} else if(v) {
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_syntax")), TRUE);
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), TRUE);
					} else {
						if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
							gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_adaptive_parsing")), TRUE);
						}
						gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_builder_get_object(main_builder, "menu_item_rpn_mode")), FALSE);
					}
				}*/
			} else if(equalsIgnoreCase(str, "exrates")) {
				if(current_expr) setPreviousExpression();
				fetchExchangeRates();
			} else if(equalsIgnoreCase(str, "stack")) {
				//gtk_expander_set_expanded(GTK_EXPANDER(expander_stack), TRUE);
			} else if(equalsIgnoreCase(str, "swap")) {
				/*if(CALCULATOR->RPNStackSize() > 1) {
					gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_registerswap_clicked(NULL, NULL);
				}*/
			} else if(equalsIgnoreCase(scom, "swap")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					int index1 = 0, index2 = 0;
					str = str.substr(ispace + 1, slen - (ispace + 1));
					std::string str2 = "";
					remove_blank_ends(str);
					ispace = str.find_first_of(SPACES);
					if(ispace != std::string::npos) {
						str2 = str.substr(ispace + 1, str.length() - (ispace + 1));
						str = str.substr(0, ispace);
						remove_blank_ends(str2);
						remove_blank_ends(str);
					}
					index1 = s2i(str);
					if(str2.empty()) index2 = 1;
					else index2 = s2i(str2);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index2 < 0) index2 = (int) CALCULATOR->RPNStackSize() + 1 + index2;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize() || (!str2.empty() && (index2 <= 0 || index2 > (int) CALCULATOR->RPNStackSize()))) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else if(index2 != 1 && index1 != 1) {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					} else if(index1 != index2) {
						if(index1 == 1) index1 = index2;
						/*GtkTreeIter iter;
						if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index1 - 1)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
							on_button_registerswap_clicked(NULL, NULL);
							gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						}*/
					}
				}
			} else if(equalsIgnoreCase(scom, "move")) {
				CALCULATOR->error(true, "Unsupported command: %s.", scom.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					/*gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_registerdown_clicked(NULL, NULL);*/
				}
			} else if(equalsIgnoreCase(scom, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					/*if(equalsIgnoreCase(str, "up")) {
						gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						on_button_registerup_clicked(NULL, NULL);
					} else if(equalsIgnoreCase(str, "down")) {
						gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						on_button_registerdown_clicked(NULL, NULL);
					} else {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					}*/
				}
			} else if(equalsIgnoreCase(str, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					/*gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_copyregister_clicked(NULL, NULL);*/
				}
			} else if(equalsIgnoreCase(scom, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						/*GtkTreeIter iter;
						if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index1 - 1)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
							on_button_copyregister_clicked(NULL, NULL);
							gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
						}*/
					}
				}
			} else if(equalsIgnoreCase(str, "clear stack")) {
				//if(CALCULATOR->RPNStackSize() > 0) on_button_clearstack_clicked(NULL, NULL);
			} else if(equalsIgnoreCase(str, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					/*gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
					on_button_deleteregister_clicked(NULL, NULL);*/
				}
			} else if(equalsIgnoreCase(scom, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						/*GtkTreeIter iter;
						if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index1 - 1)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
							on_button_deleteregister_clicked(NULL, NULL);
						}*/
					}
				}
			} else if(equalsIgnoreCase(str, "factor")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_FACTORIZE);
			} else if(equalsIgnoreCase(str, "partial fraction")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
			} else if(equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_EXPAND);
			} else if(equalsIgnoreCase(str, "exact")) {
				if(current_expr) setPreviousExpression();
				setOption("approx exact");
			} else if(equalsIgnoreCase(str, "approximate") || str == "approx") {
				if(current_expr) setPreviousExpression();
				setOption("approx try exact");
			} else if(equalsIgnoreCase(str, "mode")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "help") || str == "?") {
				//show_help("index.html", gtk_builder_get_object(main_builder, "main_window"));
			} else if(equalsIgnoreCase(str, "list")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) {
				str = str.substr(ispace + 1);
				remove_blank_ends(str);
				/*char list_type = 0;
				GtkTreeIter iter;
				if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find")) {
					size_t i = str.find_first_of(SPACES);
					std::string str1, str2;
					if(i == std::string::npos) {
						str1 = str;
					} else {
						str1 = str.substr(0, i);
						str2 = str.substr(i + 1);
						remove_blank_ends(str2);
					}
					if(equalsIgnoreCase(str1, "currencies")) list_type = 'c';
					else if(equalsIgnoreCase(str1, "functions")) list_type = 'f';
					else if(equalsIgnoreCase(str1, "variables")) list_type = 'v';
					else if(equalsIgnoreCase(str1, "units")) list_type = 'u';
					else if(equalsIgnoreCase(str1, "prefixes")) list_type = 'p';
					if(list_type == 'c') {
						manage_units();
						std::string s_cat = CALCULATOR->u_euro->category();
						GtkTreeIter iter1;
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter1) && gtk_tree_model_iter_children(GTK_TREE_MODEL(tUnitCategories_store), &iter, &iter1)) {
							do {
								gchar *gstr;
								gtk_tree_model_get(GTK_TREE_MODEL(tUnitCategories_store), &iter, 0, &gstr, -1);
								if(s_cat == gstr) {
									gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
									g_free(gstr);
									break;
								}
								g_free(gstr);
							} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnitCategories_store), &iter));
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str2.c_str());
					} else if(list_type == 'f') {
						manage_functions();
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionCategories_store), &iter)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), str2.c_str());
					} else if(list_type == 'v') {
						manage_variables();
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str2.c_str());
					} else if(list_type == 'u') {
						manage_units();
						if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
							gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
						}
						gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str2.c_str());
					} else if(list_type == 'p') {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					}
				}
				if(list_type == 0) {
					ExpressionItem *item = CALCULATOR->getActiveExpressionItem(str);
					if(item) {
						if(item->type() == TYPE_UNIT) {
							manage_units();
							if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
								gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
							}
							gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str.c_str());
						} else if(item->type() == TYPE_FUNCTION) {
							manage_functions();
							if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionCategories_store), &iter)) {
								gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
							}
							gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), str.c_str());
						} else if(item->type() == TYPE_VARIABLE) {
							manage_variables();
							if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariableCategories_store), &iter)) {
								gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
							}
							gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variables_builder, "variables_entry_search")), str.c_str());
						}
					} else {
						CALCULATOR->error(true, "No function, variable, or unit with the specified name (%s) was found.", str.c_str(), NULL);
					}
				}*/
			} else if(equalsIgnoreCase(str, "quit") || equalsIgnoreCase(str, "exit")) {
				qApp->closeAllWindows();
			} else {
				CALCULATOR->error(true, "Unknown command: %s.", str.c_str(), NULL);
			}
			displayMessages(this);
			return;
		}
	}

	if(execute_str.empty()) {
		if(str == "MC") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMCClicked();
			return;
		} else if(str == "MS") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMSClicked();
			return;
		} else if(str == "M+") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMPlusClicked();
			return;
		} else if(str == "M-" || str == "M−") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMMinusClicked();
			return;
		}
	}

	ComplexNumberForm cnf_bak = settings->evalops.complex_number_form;
	bool b_units_saved = settings->evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = settings->evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = settings->evalops.mixed_units_conversion;

	bool had_to_expression = false;
	std::string from_str = str;
	bool last_is_space = !from_str.empty() && is_in(SPACES, from_str[from_str.length() - 1]);
	if(execute_str.empty() && CALCULATOR->separateToExpression(from_str, to_str, settings->evalops, true, !do_stack)) {
		remove_duplicate_blanks(to_str);
		had_to_expression = true;
		std::string str_left;
		std::string to_str1, to_str2;
		bool do_to = false;
		while(true) {
			if(!from_str.empty()) {
				if(last_is_space) to_str += " ";
				CALCULATOR->separateToExpression(to_str, str_left, settings->evalops, true, false);
				remove_blank_ends(to_str);
			}
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != std::string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, tr("hexadecimal").toStdString())) {
				to_base = BASE_HEXADECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, tr("octal").toStdString())) {
				to_base = BASE_OCTAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "dec") || equalsIgnoreCase(to_str, "decimal") || equalsIgnoreCase(to_str, tr("decimal").toStdString())) {
				to_base = BASE_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "duo") || equalsIgnoreCase(to_str, "duodecimal") || equalsIgnoreCase(to_str, tr("duodecimal").toStdString())) {
				to_base = BASE_DUODECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, tr("binary").toStdString())) {
				to_base = BASE_BINARY;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, tr("roman").toStdString())) {
				to_base = BASE_ROMAN_NUMERALS;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, tr("bijective").toStdString())) {
				to_base = BASE_BIJECTIVE_26;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, tr("sexagesimal").toStdString())) {
				to_base = BASE_SEXAGESIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "2")) {
				to_base = BASE_SEXAGESIMAL_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "3")) {
				to_base = BASE_SEXAGESIMAL_3;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "latitude") || equalsIgnoreCase(to_str, tr("latitude").toStdString())) {
				to_base = BASE_LATITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", tr("latitude"), "2")) {
				to_base = BASE_LATITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "longitude") || equalsIgnoreCase(to_str, tr("longitude").toStdString())) {
				to_base = BASE_LONGITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", tr("longitude"), "2")) {
				to_base = BASE_LONGITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
				to_base = BASE_FP32;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
				to_base = BASE_FP64;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
				to_base = BASE_FP16;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp80")) {
				to_base = BASE_FP80;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
				to_base = BASE_FP128;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, tr("time").toStdString())) {
				to_base = BASE_TIME;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "Unicode")) {
				to_base = BASE_UNICODE;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				settings->printops.time_zone = TIME_ZONE_UTC;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.custom_time_zone = 0;
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_BINARY;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_HEXADECIMAL;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
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
					CALCULATOR->error(true, tr("Time zone parsing failed.").toUtf8().data(),  NULL);
				}
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = itz;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.custom_time_zone = 0;
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str == "CET") {
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = 60;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.custom_time_zone = 0;
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, tr("bases").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					if(current_expr) setPreviousExpression();
					basesDock->show();
					return;
				}
				do_bases = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, tr("calendars").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					if(current_expr) setPreviousExpression();
					//on_popup_menu_item_calendarconversion_activate(NULL, NULL);
					return;
				}
				do_calendars = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "rectangular") || equalsIgnoreCase(to_str, "cartesian") || equalsIgnoreCase(to_str, tr("rectangular").toStdString()) || equalsIgnoreCase(to_str, tr("cartesian").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "exponential") || equalsIgnoreCase(to_str, tr("exponential").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "polar") || equalsIgnoreCase(to_str, tr("polar").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				to_caf = 0;
				do_to = true;
			} else if(to_str == "cis") {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "phasor") || equalsIgnoreCase(to_str, tr("phasor").toStdString()) || equalsIgnoreCase(to_str, "angle") || equalsIgnoreCase(to_str, tr("angle").toStdString())) {
				settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				to_caf = 1;
				do_to = true;
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
			} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, tr("optimal").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_OPTIMAL);
					if(current_expr) setPreviousExpression();
					return;
				}
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, tr("base").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_BASE);
					if(current_expr) setPreviousExpression();
					return;
				}
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_BASE;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, tr("mixed").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_NONE;
				settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				if(from_str.empty()) {
					b_busy--;
					if(!previous_expression.empty()) calculateExpression(force, do_mathoperation, op, f, do_stack, stack_index, previous_expression);
					if(current_expr) setPreviousExpression();
					settings->evalops.auto_post_conversion = save_auto_post_conversion;
					settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
					settings->evalops.parse_options.units_enabled = b_units_saved;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fraction") || equalsIgnoreCase(to_str, tr("fraction").toStdString())) {
				do_to = true;
				to_fraction = true;
			} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, tr("factors").toStdString()) || equalsIgnoreCase(to_str, "factor")) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_FACTORIZE);
					if(current_expr) setPreviousExpression();
					return;
				}
				do_factors = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, tr("partial fraction").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
					if(current_expr) setPreviousExpression();
					return;
				}
				do_pfe = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, tr("base").toStdString())) {
				base_from_string(to_str2, to_base, to_nbase);
				do_to = true;
			} else if(from_str.empty()) {
				b_busy--;
				executeCommand(COMMAND_CONVERT_STRING, true, CALCULATOR->unlocalizeExpression(to_str, settings->evalops.parse_options));
				if(current_expr) setPreviousExpression();
				return;
			} else {
				if(to_str[0] == '?') {
					to_prefix = 1;
				} else if(to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
					to_prefix = to_str[0];

				}
				do_to = true;
				if(!str_conv.empty()) str_conv += " to ";
				str_conv += to_str;
			}
			if(str_left.empty()) break;
			to_str = str_left;
		}
		if(do_to) {
			if(from_str.empty()) {
				b_busy--;
				setResult(NULL, true, false, false);
				if(current_expr) setPreviousExpression();
				return;
			} else {
				execute_str = from_str;
				if(!str_conv.empty()) {
					execute_str += " to ";
					execute_str += str_conv;
				}
			}
		}
	}
	if(execute_str.empty()) {
		size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
		if(i != std::string::npos) {
			to_str = str.substr(0, i);
			if(to_str == "factor" || equalsIgnoreCase(to_str, "factorize") || equalsIgnoreCase(to_str, tr("factorize").toStdString())) {
				execute_str = str.substr(i + 1);
				do_factors = true;
			} else if(equalsIgnoreCase(to_str, "expand") || equalsIgnoreCase(to_str, tr("expand").toStdString())) {
				execute_str = str.substr(i + 1);
				do_expand = true;
			}
		}
	}

	size_t stack_size = 0;

	/*if(do_ceu && str_conv.empty() && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion"))) && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		std::string ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), pa);
		remove_blank_ends(ceu_str);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {
			if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
				ceu_str = "?" + ceu_str;
			}
		}
		if(ceu_str.empty()) {
			parsed_tostruct->setUndefined();
		} else {
			if(ceu_str[0] == '?') {
				to_prefix = 1;
			} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
				to_prefix = ceu_str[0];
			}
			parsed_tostruct->set(ceu_str);
		}
	} else {*/
		parsed_tostruct->setUndefined();
	//}
	CALCULATOR->resetExchangeRatesUsed();
	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation && f) {
			CALCULATOR->getRPNRegister(stack_index + 1)->transform(f);
			parsed_mstruct->set(*CALCULATOR->getRPNRegister(stack_index + 1));
			CALCULATOR->calculateRPNRegister(stack_index + 1, 0, settings->evalops);
		} else {
			CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options), 0, settings->evalops, parsed_mstruct, parsed_tostruct);
		}
	} else if(settings->rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			if(mstruct) lastx = *mstruct;
			//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx")), TRUE);
			if(f) CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, settings->evalops, parsed_mstruct);
		} else {
			std::string str2 = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options);
			CALCULATOR->parseSigns(str2);
			remove_blank_ends(str2);
			MathStructure lastx_bak(lastx);
			if(mstruct) lastx = *mstruct;
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
					case '!': {CALCULATOR->calculateRPN(CALCULATOR->f_factorial, 0, settings->evalops, parsed_mstruct); break;}
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
					CALCULATOR->calculateRPN(CALCULATOR->f_factorial2, 0, settings->evalops, parsed_mstruct);
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
					CALCULATOR->RPNStackEnter(str2, 0, settings->evalops, parsed_mstruct, parsed_tostruct);
				}
				/*if(do_mathoperation) gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx")), TRUE);
				else lastx = lastx_bak;*/
			}
		}
	} else {
		original_expression = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options);
		CALCULATOR->calculate(mstruct, original_expression, 0, settings->evalops, parsed_mstruct, parsed_tostruct);
	}

	bool title_set = false, was_busy = false;

	QProgressDialog *dialog = NULL;

	do_progress:
	int i = 0;
	while(CALCULATOR->busy() && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(CALCULATOR->busy() && !was_busy) {
		if(updateWindowTitle(tr("Calculating…"))) title_set = true;
		dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(CALCULATOR->busy()) {
		qApp->processEvents();
		sleep_ms(100);
	}

	bool units_changed = false;
	if(!do_mathoperation && !str_conv.empty() && parsed_tostruct->containsType(STRUCT_UNIT, true) && !mstruct->containsType(STRUCT_UNIT) && !parsed_mstruct->containsType(STRUCT_UNIT, false, true, true) && !CALCULATOR->hasToExpression(str_conv, false, settings->evalops)) {
		MathStructure to_struct(*parsed_tostruct);
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
			CALCULATOR->calculate(mstruct, 0, settings->evalops, CALCULATOR->unlocalizeExpression(str_conv, settings->evalops.parse_options));
			str_conv = "";
			units_changed = true;
			goto do_progress;
		}
	}

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	if(settings->rpn_mode && stack_index == 0) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
	}

	if(do_stack && stack_index > 0) {
	} else if(settings->rpn_mode && do_mathoperation) {
		result_text = tr("RPN Operation").toStdString();
	} else {
		result_text = str;
	}
	settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
	if(settings->rpn_mode && stack_index == 0) {
		expressionEdit->clear();
		while(CALCULATOR->RPNStackSize() < stack_size) {
			//RPNRegisterRemoved(1);
			stack_size--;
		}
		if(CALCULATOR->RPNStackSize() > stack_size) {
			//RPNRegisterAdded("");
		}
	}

	if(settings->rpn_mode && do_mathoperation && parsed_tostruct && !parsed_tostruct->isUndefined() && parsed_tostruct->isSymbolic()) {
		mstruct->set(CALCULATOR->convert(*mstruct, parsed_tostruct->symbol(), settings->evalops));
	}

	// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
	if(!settings->rpn_mode && (!parsed_tostruct || parsed_tostruct->isUndefined()) && execute_str.empty() && !had_to_expression && (settings->evalops.approximation == APPROXIMATION_EXACT || settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) && parsed_mstruct && mstruct && ((parsed_mstruct->isMultiplication() && parsed_mstruct->size() == 2 && (*parsed_mstruct)[0].isNumber() && (*parsed_mstruct)[1].isUnit_exp() && parsed_mstruct->equals(*mstruct)) || (parsed_mstruct->isNegate() && (*parsed_mstruct)[0].isMultiplication() && (*parsed_mstruct)[0].size() == 2 && (*parsed_mstruct)[0][0].isNumber() && (*parsed_mstruct)[0][1].isUnit_exp() && mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1] == (*parsed_mstruct)[0][1] && (*mstruct)[0].isNumber() && (*parsed_mstruct)[0][0].number() == -(*mstruct)[0].number()) || (parsed_mstruct->isUnit_exp() && parsed_mstruct->equals(*mstruct)))) {
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
			if(settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) {
				if(munit->isUnit() && u->referenceName() == "oF") {
					u = CALCULATOR->getActiveUnit("oC");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, settings->evalops, true, false));
				} else {
					mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				}
			}
			if(settings->evalops.approximation == APPROXIMATION_EXACT && ((settings->evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL && settings->evalops.auto_post_conversion != POST_CONVERSION_NONE) || mstruct->equals(mbak))) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				if(settings->evalops.auto_post_conversion == POST_CONVERSION_BASE) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, settings->evalops));
				else mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = APPROXIMATION_EXACT;
			}
		}
	}

	if(!do_mathoperation && (askTC(*parsed_mstruct) || (check_exrates && checkExchangeRates()))) {
		calculateExpression(force, do_mathoperation, op, f, settings->rpn_mode, stack_index, saved_execute_str, str, false);
		settings->evalops.complex_number_form = cnf_bak;
		settings->evalops.auto_post_conversion = save_auto_post_conversion;
		settings->evalops.parse_options.units_enabled = b_units_saved;
		settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
		settings->printops.custom_time_zone = 0;
		settings->printops.time_zone = TIME_ZONE_LOCAL;
		b_busy--;
		return;
	}

	//update "ans" variables
	if(stack_index == 0) {
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

	mstruct_exact.setUndefined();
	
	if((!do_calendars || !mstruct->isDateTime()) && (settings->dual_approximation > 0 || settings->printops.base == BASE_DECIMAL) && !do_bases && !units_changed) {
		long int i_timeleft = 0;
		i_timeleft = mstruct->containsType(STRUCT_COMPARISON) ? 2000 : 1000;
		if(i_timeleft > 0) {
			calculate_dual_exact(mstruct_exact, mstruct, original_expression, parsed_mstruct, settings->evalops, settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_approximation > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), i_timeleft, -1);
		}
	}

	b_busy--;

	if(do_factors || do_pfe || do_expand) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			mstruct = save_mstruct;
		} else {
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS  : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
		}
	}

	if(stack_index == 0) {
		if(!mstruct_exact.isUndefined()) settings->history_answer.push_back(new MathStructure(mstruct_exact));
		settings->history_answer.push_back(new MathStructure(*mstruct));
	}

	if(!do_stack) previous_expression = execute_str.empty() ? str : execute_str;
	setResult(NULL, true, stack_index == 0, true, "", stack_index);
	
	if(do_bases) basesDock->show();
	//if(do_calendars) on_popup_menu_item_calendarconversion_activate(NULL, NULL);
	
	settings->evalops.complex_number_form = cnf_bak;
	settings->evalops.auto_post_conversion = save_auto_post_conversion;
	settings->evalops.parse_options.units_enabled = b_units_saved;
	settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
	settings->printops.custom_time_zone = 0;
	settings->printops.time_zone = TIME_ZONE_LOCAL;

	if(stack_index == 0) {
		/*if(!block_conversion_category_switch) {
			Unit *u = CALCULATOR->findMatchingUnit(*mstruct);
			if(u && !u->category().empty()) {
				std::string s_cat = u->category();
				if(s_cat.empty()) s_cat = tr("Uncategorized").toStdString();
				if(s_cat != selected_unit_category) {
					GtkTreeIter iter = convert_category_map[s_cat];
					GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
					gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tUnitSelectorCategories), path);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tUnitSelectorCategories), path, NULL, TRUE, 0.5, 0);
					gtk_tree_path_free(path);
					gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
				}
			}
			if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")))) {
				gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)));
			}
		}*/
		expressionEdit->blockCompletion();
		expressionEdit->blockParseStatus();
		if(settings->replace_expression == CLEAR_EXPRESSION) {
			expressionEdit->clear();
		} else if(settings->replace_expression == REPLACE_EXPRESSION_WITH_RESULT || settings->replace_expression == REPLACE_EXPRESSION_WITH_RESULT_IF_SHORTER) {
			if(settings->replace_expression == REPLACE_EXPRESSION_WITH_RESULT || (!exact_text.empty() && unicode_length(exact_text) < unicode_length(from_str))) {
				if(exact_text == "0") expressionEdit->clear();
				else if(exact_text.empty()) expressionEdit->setPlainText(QString::fromStdString(unhtmlize(result_text)));
				else expressionEdit->setPlainText(QString::fromStdString(exact_text));
			} else {
				if(!execute_str.empty()) {
					from_str = execute_str;
					CALCULATOR->separateToExpression(from_str, str, settings->evalops, true, true);
				}
				expressionEdit->setPlainText(QString::fromStdString(from_str));
			}
		}
		if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
		expressionEdit->selectAll();
		expressionEdit->blockCompletion(false);
		expressionEdit->blockParseStatus(false);
		expressionEdit->setExpressionHasChanged(false);
	}
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
		EvaluationOptions eo2 = settings->evalops;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->structure(STRUCTURING_FACTORIZE, eo2, true);
				}
				if(x2 && !((MathStructure*) x2)->integerFactorize()) {
					eo2.approximation = APPROXIMATION_EXACT;
					((MathStructure*) x2)->structure(STRUCTURING_FACTORIZE, eo2, true);
				}
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				((MathStructure*) x)->expandPartialFractions(eo2);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->expandPartialFractions(eo2);
				break;
			}
			case COMMAND_EXPAND: {
				((MathStructure*) x)->expand(eo2);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->expand(eo2);
				break;
			}
			case COMMAND_TRANSFORM: {
				std::string ceu_str;
				/*if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion"))) && gtk_expander_get_expanded(GTK_EXPANDER(expander_convert)) && !minimal_mode) {
					ParseOptions pa = eo2.parse_options; pa.base = 10;
					ceu_str = CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))), pa);
					remove_blank_ends(ceu_str);
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes"))) && !ceu_str.empty()) {
						if(ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
							ceu_str = "?" + ceu_str;
						}
					}
					if(!ceu_str.empty() && ceu_str[0] == '?') {
						to_prefix = 1;
					} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
						to_prefix = ceu_str[0];
					}
				}*/
				((MathStructure*) x)->set(CALCULATOR->calculate(*((MathStructure*) x), eo2, ceu_str));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->calculate(*((MathStructure*) x2), eo2, ceu_str));
				break;
			}
			case COMMAND_CONVERT_STRING: {
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_units_string, eo2));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convert(*((MathStructure*) x2), command_convert_units_string, eo2));
				break;
			}
			case COMMAND_CONVERT_UNIT: {
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_unit, eo2, false));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convert(*((MathStructure*) x2), command_convert_unit, eo2, false));
				break;
			}
			case COMMAND_CONVERT_OPTIMAL: {
				((MathStructure*) x)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x), eo2, true));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x2), eo2, true));
				break;
			}
			case COMMAND_CONVERT_BASE: {
				((MathStructure*) x)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x), eo2));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x2), eo2));
				break;
			}
			case COMMAND_CALCULATE: {
				eo2.calculate_functions = false;
				eo2.sync_units = false;
				((MathStructure*) x)->calculatesub(eo2, eo2, true);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->calculatesub(eo2, eo2, true);
				break;
			}
			case COMMAND_EVAL: {
			
				((MathStructure*) x)->eval(eo2);
				if(x2) ((MathStructure*) x2)->eval(eo2);
				break;
			}
		}
		b_busy--;
		CALCULATOR->stopControl();

	}
}

void QalculateWindow::executeCommand(int command_type, bool show_result, std::string ceu_str, Unit *u, int run) {

	if(run == 1) {
	
		if(expressionEdit->expressionHasChanged() && !settings->rpn_mode && command_type != COMMAND_TRANSFORM) {
			calculateExpression();
		}

		if(b_busy) return;

		/*if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING) {
			if(mbak_convert.isUndefined()) mbak_convert.set(*mstruct);
			else mstruct->set(mbak_convert);
		} else {
			if(!mbak_convert.isUndefined()) mbak_convert.setUndefined();
		}*/

		b_busy++;
		command_aborted = false;

		if(command_type >= COMMAND_CONVERT_UNIT) {
			CALCULATOR->resetExchangeRatesUsed();
			command_convert_units_string = ceu_str;
			command_convert_unit = u;
		}
		if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING || command_type == COMMAND_CONVERT_BASE || command_type == COMMAND_CONVERT_OPTIMAL) {
			to_prefix = 0;
		}
	}

	bool title_set = false, was_busy = false;
	QProgressDialog *dialog = NULL;

	int i = 0;

	MathStructure *mfactor = new MathStructure(*mstruct);
	MathStructure *mfactor2 = NULL;
	if(!mstruct_exact.isUndefined()) mfactor2 = new MathStructure(mstruct_exact);

	rerun_command:

	if((!command_thread->running && !command_thread->start()) || !command_thread->write(command_type) || !command_thread->write((void *) mfactor) || !command_thread->write((void *) mfactor2)) {
		command_thread->cancel();
		mfactor->unref();
		if(mfactor2) mfactor2->unref();
		b_busy--;
		return;
	}

	while(b_busy && command_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(!was_busy && b_busy && command_thread->running) {
		QString progress_str;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				progress_str = tr("Factorizing…");
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				progress_str = tr("Expanding partial fractions…");
				break;
			}
			case COMMAND_EXPAND: {
				progress_str = tr("Expanding…");
				break;
			}
			case COMMAND_EVAL: {}
			case COMMAND_TRANSFORM: {
				progress_str = tr("Calculating…");
				break;
			}
			default: {
				progress_str = tr("Converting…");
				break;
			}
		}
		if(updateWindowTitle(progress_str)) title_set = true;
		dialog = new QProgressDialog(progress_str, tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abortCommand()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(b_busy && command_thread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(!command_thread->running) command_aborted = true;

	if(!command_aborted && run == 1 && command_type >= COMMAND_CONVERT_UNIT && checkExchangeRates()) {
		b_busy++;
		mfactor->set(*mstruct);
		run = 2;
		goto rerun_command;
	}

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	if(command_type == COMMAND_CONVERT_STRING && !ceu_str.empty()) {
		if(ceu_str[0] == '?') {
			to_prefix = 1;
		} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
			to_prefix = ceu_str[0];
		}
	}

	if(!command_aborted) {
		if(mfactor2) {mstruct_exact.set(*mfactor2); mfactor2->unref();}
		mstruct->set(*mfactor);
		mfactor->unref();
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
		if(show_result) {
			if(!mstruct_exact.isUndefined()) settings->history_answer.push_back(new MathStructure(mstruct_exact));
			settings->history_answer.push_back(new MathStructure(*mstruct));
			setResult(NULL, true, false, true, command_type == COMMAND_TRANSFORM ? ceu_str : "");
		}
	}

}

void QalculateWindow::updateResultBases() {
	binEdit->setText(QString::fromStdString(result_bin));
	octEdit->setText(QString::fromStdString(result_oct));
	decEdit->setText(QString::fromStdString(result_dec));
	hexEdit->setText(QString::fromStdString(result_hex));
}

void set_result_bases(const MathStructure &m) {
	result_bin = ""; result_oct = "", result_dec = "", result_hex = "";
	if(max_bases.isZero()) {max_bases = 2; max_bases ^= 64; min_bases = 2; min_bases ^= 32; min_bases.negate();}
	if(!CALCULATOR->aborted() && ((m.isNumber() && m.number() < max_bases && m.number() > min_bases) || (m.isNegate() && m[0].isNumber() && m[0].number() < max_bases && m[0].number() > min_bases))) {
		Number nr;
		if(m.isNumber()) {
			nr = m.number();
		} else {
			nr = m[0].number();
			nr.negate();
		}
		nr.round(settings->printops.round_halfway_to_even);
		PrintOptions po = settings->printops;
		po.is_approximate = NULL;
		po.show_ending_zeroes = false;
		po.base_display = BASE_DISPLAY_NORMAL;
		po.min_exp = 0;
		po.base = 2;
		po.binary_bits = 64;
		result_bin = nr.print(po);
		if(result_bin.length() > 80 && result_bin.find("1") >= 80) result_bin.erase(0, 80);
		if(result_bin.length() >= 40) result_bin.replace(39, 1, "\n");
		po.base = 8;
		result_oct = nr.print(po);
		size_t i = result_oct.find_first_of(NUMBERS);
		if(i != std::string::npos && result_oct.length() > i + 1 && result_oct[i] == '0' && is_in(NUMBERS, result_oct[i + 1])) result_oct.erase(i, 1);
		po.base = 10;
		result_dec = nr.print(po);
		po.base = 16;
		result_hex = nr.print(po);
		gsub("0x", "", result_hex);
		size_t l = result_hex.length();
		size_t i_after_minus = 0;
		if(nr.isNegative()) {
			if(l > 1 && result_hex[0] == '-') i_after_minus = 1;
			else if(result_hex.find("−") == 0) i_after_minus = strlen("−");
		}
		for(int i = (int) l - 2; i > (int) i_after_minus; i -= 2) {
			result_hex.insert(i, 1, ' ');
		}
		if(result_hex.length() > i_after_minus + 1 && result_hex[i_after_minus + 1] == ' ') result_hex.insert(i_after_minus, 1, '0');
	}
}

void QalculateWindow::onExpressionChanged() {
	if(!expressionEdit->expressionHasChanged() || !basesDock->isVisible()) return;
	MathStructure m;
	EvaluationOptions eo = settings->evalops;
	eo.structuring = STRUCTURING_NONE;
	eo.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
	eo.auto_post_conversion = POST_CONVERSION_NONE;
	eo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	eo.expand = -2;
	CALCULATOR->beginTemporaryStopMessages();
	if(!CALCULATOR->calculate(&m, expressionEdit->toPlainText().toStdString(), 100, eo)) {
		result_bin = ""; result_oct = "", result_dec = "", result_hex = "";
	} else {
		set_result_bases(m);
	}
	CALCULATOR->endTemporaryStopMessages();
	updateResultBases();
}

void ViewThread::run() {

	while(true) {

		void *x = NULL;
		if(!read(&x) || !x) break;
		MathStructure *mresult = (MathStructure*) x;
		x = NULL;
		bool b_stack = false;
		if(!read(&b_stack)) break;
		if(!read(&x)) break;
		CALCULATOR->startControl();
		MathStructure *mparse = (MathStructure*) x;
		PrintOptions po;
		if(mparse) {
			if(!read(&po.is_approximate)) break;
			if(!read<bool>(&po.preserve_format)) break;
			po.show_ending_zeroes = settings->evalops.parse_options.read_precision != DONT_READ_PRECISION && CALCULATOR->usesIntervalArithmetic() && settings->evalops.parse_options.base > BASE_CUSTOM;
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
			parsed_text = mp.print(po, true, settings->colorize_result ? settings->color : 0, TAG_TYPE_HTML);
			if(po.base == BASE_CUSTOM) {
				CALCULATOR->setCustomOutputBase(nr_base);
			}
		}

		po = settings->printops;

		po.allow_non_usable = true;

		print_dual(*mresult, original_expression, mparse ? *mparse : *parsed_mstruct, mstruct_exact, result_text, alt_results, po, settings->evalops, settings->dual_fraction < 0 ? AUTOMATIC_FRACTION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_FRACTION_DUAL : AUTOMATIC_FRACTION_OFF), settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), settings->complex_angle_form, &exact_comparison, mparse != NULL, true, settings->colorize_result ? settings->color : 0, TAG_TYPE_HTML);

		if(!b_stack) {
			set_result_bases(*mresult);
		}

		b_busy--;
		CALCULATOR->stopControl();

	}
}

void QalculateWindow::setResult(Prefix *prefix, bool update_history, bool update_parse, bool force, std::string transformation, size_t stack_index, bool register_moved, bool supress_dialog) {

	if(block_result_update) return;

	if(expressionEdit->expressionHasChanged() && (!settings->rpn_mode || CALCULATOR->RPNStackSize() == 0)) {
		if(!force) return;
		calculateExpression();
		if(!prefix) return;
	}

	if(settings->rpn_mode && CALCULATOR->RPNStackSize() == 0) return;

	if(settings->history_answer.empty() && !register_moved && !update_parse && update_history) {
		return;
	}

	if(b_busy) return;

	std::string prev_result_text = result_text;
	bool prev_approximate = *settings->printops.is_approximate;
	result_text = "?";

	if(update_parse) {
		parsed_text = tr("aborted").toStdString();
	}

	if(!settings->rpn_mode) stack_index = 0;
	if(stack_index != 0) {
		update_history = true;
		update_parse = false;
		
	}
	if(register_moved) {
		update_history = true;
		update_parse = false;
	}

	if(update_parse && parsed_mstruct && parsed_mstruct->isFunction() && (parsed_mstruct->function() == CALCULATOR->f_error || parsed_mstruct->function() == CALCULATOR->f_warning || parsed_mstruct->function() == CALCULATOR->f_message)) {
		expressionEdit->clear();
		historyView->addMessages();
		return;
	}

	b_busy++;

	if(!view_thread->running && !view_thread->start()) {b_busy--; return;}

	bool b_rpn_operation = false;

	if(update_history) {
		if(update_parse || register_moved) {
			if(register_moved) {
				result_text = tr("RPN Register Moved").toStdString();
			} else {
				remove_blank_ends(result_text);
				gsub("\n", " ", result_text);
				if(result_text == tr("RPN Operation").toStdString()) {
					b_rpn_operation = true;
				} else {
					if(settings->adaptive_interval_display) {
						QString expression_str = expressionEdit->toPlainText();
						if((parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_uncertainty)) || expression_str.contains("+/-") || expression_str.contains("+/" SIGN_MINUS) || expression_str.contains("±")) settings->printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
						else if(parsed_mstruct && parsed_mstruct->containsFunction(CALCULATOR->f_interval)) settings->printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
						else settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
					}
				}
			}
		} else if(!transformation.empty()) {
		}
		result_text = "?";
	}

	if(update_parse) {
		parsed_text = "aborted";
	}

	bool parsed_approx = false;
	bool title_set = false, was_busy = false;

	Number save_nbase;
	bool custom_base_set = false;
	int save_base = settings->printops.base;
	bool caf_bak = settings->complex_angle_form;
	unsigned int save_bits = settings->printops.binary_bits;
	bool save_pre = settings->printops.use_unit_prefixes;
	bool save_cur = settings->printops.use_prefixes_for_currencies;
	bool save_allu = settings->printops.use_prefixes_for_all_units;
	bool save_all = settings->printops.use_all_prefixes;
	bool save_den = settings->printops.use_denominator_prefix;
	int save_bin = CALCULATOR->usesBinaryPrefixes();
	NumberFractionFormat save_format = settings->printops.number_fraction_format;
	bool save_restrict_fraction_length = settings->printops.restrict_fraction_length;
	bool do_to = false;

	if(stack_index == 0) {
		if(to_base != 0 || to_fraction || to_prefix != 0 || (to_caf >= 0 && to_caf != settings->complex_angle_form)) {
			if(to_base != 0 && (to_base != settings->printops.base || to_bits != settings->printops.binary_bits || (to_base == BASE_CUSTOM && to_nbase != CALCULATOR->customOutputBase()))) {
				settings->printops.base = to_base;
				settings->printops.binary_bits = to_bits;
				if(to_base == BASE_CUSTOM) {
					custom_base_set = true;
					save_nbase = CALCULATOR->customOutputBase();
					CALCULATOR->setCustomOutputBase(to_nbase);
				}
				do_to = true;
			}
			if(to_fraction && (settings->printops.restrict_fraction_length || settings->printops.number_fraction_format != FRACTION_COMBINED)) {
				settings->printops.restrict_fraction_length = false;
				settings->printops.number_fraction_format = FRACTION_COMBINED;
				do_to = true;
			}
			if(to_caf >= 0 && to_caf != settings->complex_angle_form) {
				settings->complex_angle_form = to_caf;
				do_to = true;
			}
			if(to_prefix != 0 && !prefix) {
				bool new_pre = settings->printops.use_unit_prefixes;
				bool new_cur = settings->printops.use_prefixes_for_currencies;
				bool new_allu = settings->printops.use_prefixes_for_all_units;
				bool new_all = settings->printops.use_all_prefixes;
				bool new_den = settings->printops.use_denominator_prefix;
				int new_bin = CALCULATOR->usesBinaryPrefixes();
				new_pre = true;
				if(to_prefix == 'b') {
					int i = has_information_unit(*mstruct);
					new_bin = (i > 0 ? 1 : 2);
					if(i == 1) {
						new_den = false;
					} else if(i > 1) {
						new_den = true;
					} else {
						new_cur = true;
						new_allu = true;
					}
				} else {
					new_cur = true;
					new_allu = true;
					if(to_prefix == 'a') new_all = true;
					else if(to_prefix == 'd') new_bin = 0;
				}
				if(settings->printops.use_unit_prefixes != new_pre || settings->printops.use_prefixes_for_currencies != new_cur || settings->printops.use_prefixes_for_all_units != new_allu || settings->printops.use_all_prefixes != new_all || settings->printops.use_denominator_prefix != new_den || CALCULATOR->usesBinaryPrefixes() != new_bin) {
					settings->printops.use_unit_prefixes = new_pre;
					settings->printops.use_all_prefixes = new_all;
					settings->printops.use_prefixes_for_currencies = new_cur;
					settings->printops.use_prefixes_for_all_units = new_allu;
					settings->printops.use_denominator_prefix = new_den;
					CALCULATOR->useBinaryPrefixes(new_bin);
					do_to = true;
				}
			}
		}
	}

	settings->printops.prefix = prefix;

	if(stack_index == 0) {
		if(!view_thread->write((void*) mstruct)) {b_busy--; view_thread->cancel(); return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!view_thread->write((void*) mreg)) {b_busy--; view_thread->cancel(); return;}
	}
	bool b_stack = stack_index != 0;
	if(!view_thread->write(b_stack)) {b_busy--; view_thread->cancel(); return;}
	if(update_parse) {
		if(!view_thread->write((void *) parsed_mstruct)) {b_busy--; view_thread->cancel(); return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!view_thread->write(parsed_approx_p)) {b_busy--; view_thread->cancel(); return;}
		if(!view_thread->write(!b_rpn_operation)) {b_busy--; view_thread->cancel(); return;}
	} else {
		if(settings->printops.base != BASE_DECIMAL && settings->dual_approximation <= 0) mstruct_exact.setUndefined();
		if(!view_thread->write((void *) NULL)) {b_busy--; view_thread->cancel(); return;}
	}

	QProgressDialog *dialog = NULL;

	int i = 0;
	while(b_busy && view_thread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(b_busy && view_thread->running) {
		if(updateWindowTitle(tr("Processing…"))) title_set = true;
		dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(b_busy && view_thread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	b_busy++;

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set && stack_index != 0) updateWindowTitle();
	}

	if(stack_index == 0) {
		if(basesDock->isVisible()) updateResultBases();
		if((settings->title_type == TITLE_APP || !updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true)) && title_set) updateWindowTitle();
	}
	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}
	/*if(current_inhistory_index < 0) {
		update_parse = true;
		current_inhistory_index = 0;
	}*/
	if(stack_index != 0) {
		//RPNRegisterChanged(result_text, stack_index);
		if(supress_dialog) CALCULATOR->clearMessages();
		else displayMessages(this);
	} else if(update_history) {
		if(update_parse) {}
	} else {
		if(supress_dialog) CALCULATOR->clearMessages();
		else displayMessages(this);
	}

	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}

	if(stack_index != 0) {
		//RPNRegisterChanged(result_text, stack_index);
	} else {
		if(mstruct->isAborted() || settings->printops.base != settings->evalops.parse_options.base || (settings->printops.base > 32 || settings->printops.base < 2)) {
			exact_text = "";
		} else if(!mstruct->isApproximate() && !(*settings->printops.is_approximate)) {
			exact_text = unhtmlize(result_text);
		} else if(!alt_results.empty()) {
			exact_text = unhtmlize(alt_results[0]);
		} else {
			exact_text = "";
		}
		alt_results.push_back(result_text);
		QString flag;
		if(mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1].isUnit() && (*mstruct)[1].unit()->isCurrency()) {
			flag = ":/data/flags/" + QString::fromStdString((*mstruct)[1].unit()->referenceName()) + ".png";
		}
		historyView->addResult(alt_results, update_parse ? parsed_text : "", (update_parse || !prev_approximate) && (exact_comparison || (!(*settings->printops.is_approximate) && !mstruct->isApproximate())), update_parse && !mstruct_exact.isUndefined(), flag);
	}

	if(do_to) {
		settings->complex_angle_form = caf_bak;
		settings->printops.base = save_base;
		settings->printops.binary_bits = save_bits;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_nbase);
		settings->printops.use_unit_prefixes = save_pre;
		settings->printops.use_all_prefixes = save_all;
		settings->printops.use_prefixes_for_currencies = save_cur;
		settings->printops.use_prefixes_for_all_units = save_allu;
		settings->printops.use_denominator_prefix = save_den;
		CALCULATOR->useBinaryPrefixes(save_bin);
		settings->printops.number_fraction_format = save_format;
		settings->printops.restrict_fraction_length = save_restrict_fraction_length;
	}
	settings->printops.prefix = NULL;

	b_busy--;
}

void QalculateWindow::changeEvent(QEvent *e) {
	if(e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) {
		QColor c = QApplication::palette().base().color();
		if(c.red() + c.green() + c.blue() < 255) settings->color = 2;
		else settings->color = 1;
		menuAction->setIcon(LOAD_ICON("menu"));
		toAction->setIcon(LOAD_ICON("convert"));
		storeAction->setIcon(LOAD_ICON("document-save"));
		functionsAction->setIcon(LOAD_ICON("function"));
		keypadAction->setIcon(LOAD_ICON("keypad"));
		basesAction->setIcon(LOAD_ICON("number-bases"));
		modeAction->setIcon(LOAD_ICON("configure"));
	} else if(e->type() == QEvent::FontChange || e->type() == QEvent::ApplicationFontChange) {
		QFontMetrics fm2(QApplication::font());
		binEdit->setMinimumWidth(fm2.boundingRect("0000 0000 0000 0000 0000 0000 0000 0000").width());
		binEdit->setFixedHeight(fm2.lineSpacing() * 2.1);
		if(!settings->use_custom_expression_font) {
			QFont font = QApplication::font();
			if(font.pixelSize() >= 0) font.setPixelSize(font.pixelSize() * 1.35);
			else font.setPointSize(font.pointSize() * 1.35);
			expressionEdit->setFont(font);
		}
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

void QalculateWindow::fetchExchangeRates() {
	CALCULATOR->clearMessages();
	fetchExchangeRates(15);
	CALCULATOR->loadExchangeRates();
	displayMessages(this);
	expressionCalculationUpdated();
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
		b_busy--;
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
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Temperature calculation mode"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("The expression is ambiguous.\nPlease select temperature calculation mode\n(the mode can later be changed in preferences).")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QRadioButton *w_abs = new QRadioButton(tr("Absolute"));
	group->addButton(w_abs);
	grid->addWidget(w_abs, 1, 0);
	grid->addWidget(new QLabel("<i>1 °C + 1 °C ≈ 274 K + 274 K ≈ 548 K\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 K\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>"), 1, 1);
	QRadioButton *w_relative = new QRadioButton(tr("Relative"));
	group->addButton(w_relative);
	grid->addWidget(w_relative, 2, 0);
	grid->addWidget(new QLabel("<i>1 °C + 1 °C ≈ 274 K + 274 K ≈ 548 K\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 K\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>"), 2, 1);
	QRadioButton *w_hybrid = new QRadioButton(tr("Hybrid"));
	group->addButton(w_hybrid);
	grid->addWidget(w_hybrid, 3, 0);
	grid->addWidget(new QLabel("<i>1 °C + 1 °C ≈ 2 °C\n1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K\n2 °C − 1 °C = 1 °C\n1 °C − 5 °F = 16 K\n1 °C + 1 K = 2 °C</i>"), 3, 1);
	switch(CALCULATOR->getTemperatureCalculationMode()) {
		case TEMPERATURE_CALCULATION_ABSOLUTE: {w_abs->setChecked(true); break;}
		case TEMPERATURE_CALCULATION_RELATIVE: {w_relative->setChecked(true); break;}
		default: {w_hybrid->setChecked(true); break;}
	}
	dialog->exec();
	TemperatureCalculationMode tc_mode = TEMPERATURE_CALCULATION_HYBRID;
	if(w_abs->isChecked()) tc_mode = TEMPERATURE_CALCULATION_ABSOLUTE;
	else if(w_relative->isChecked()) tc_mode = TEMPERATURE_CALCULATION_RELATIVE;
	dialog->deleteLater();
	settings->tc_set = true;
	if(tc_mode != CALCULATOR->getTemperatureCalculationMode()) {
		CALCULATOR->setTemperatureCalculationMode(tc_mode);
		return true;
	}
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
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Interpretation of dots"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("Please select interpretation of dots (\".\")\n(this can later be changed in preferences).")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QRadioButton *w_bothdeci = new QRadioButton(tr("Both dot and comma as decimal separators"));
	group->addButton(w_bothdeci);
	grid->addWidget(w_bothdeci, 1, 0);
	grid->addWidget(new QLabel("<i>(1.2 = 1,2)</i>"), 1, 1);
	QRadioButton *w_ignoredot = new QRadioButton(tr("Dot as thousands separator"));
	group->addButton(w_ignoredot);
	grid->addWidget(w_ignoredot, 2, 0);
	grid->addWidget(new QLabel("<i>(1.000.000 = 1000000)</i>"), 2, 1);
	QRadioButton *w_dotdeci = new QRadioButton(tr("Only dot as decimal separator"));
	group->addButton(w_dotdeci);
	grid->addWidget(w_dotdeci, 3, 0);
	grid->addWidget(new QLabel("<i>(1.2 + root(16, 4) = 3.2)</i>"), 3, 1);
	if(settings->evalops.parse_options.dot_as_separator) w_ignoredot->setChecked(true);
	else w_bothdeci->setChecked(true);
	dialog->exec();
	settings->dot_question_asked = true;
	bool das = settings->evalops.parse_options.dot_as_separator;
	if(w_dotdeci->isChecked()) {
		settings->evalops.parse_options.dot_as_separator = false;
		settings->evalops.parse_options.comma_as_separator = false;
		settings->decimal_comma = false;
		CALCULATOR->useDecimalPoint(false);
		das = !settings->evalops.parse_options.dot_as_separator;
	} else if(w_ignoredot->isChecked()) {
		settings->evalops.parse_options.dot_as_separator = true;
	} else {
		settings->evalops.parse_options.dot_as_separator = false;
	}
	dialog->deleteLater();
	return das != settings->evalops.parse_options.dot_as_separator;
}
void QalculateWindow::keyPressEvent(QKeyEvent *e) {
	if(e->matches(QKeySequence::Undo)) {
		expressionEdit->editUndo();
		return;
	}
	if(e->matches(QKeySequence::Redo)) {
		expressionEdit->editRedo();
		return;
	}
	QMainWindow::keyPressEvent(e);
}
void QalculateWindow::closeEvent(QCloseEvent *e) {
	settings->window_state = saveState();
	settings->window_geometry = saveGeometry();
	settings->splitter_state = ehSplitter->saveState();
	settings->savePreferences(settings->save_mode_on_exit);
	if(settings->save_defs_on_exit) CALCULATOR->saveDefinitions();
	CALCULATOR->abort();
	QMainWindow::closeEvent(e);
	qApp->closeAllWindows();
}

void QalculateWindow::onToActivated() {
	QTextCursor cur = expressionEdit->textCursor();
	QPoint pos = tb->mapToGlobal(tb->widgetForAction(toAction)->geometry().topRight());
	if(!expressionEdit->expressionHasChanged() && cur.hasSelection() && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length()) {
		if(expressionEdit->complete(mstruct, pos)) return;
	}
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	expressionEdit->moveCursor(QTextCursor::End);
	expressionEdit->insertPlainText("➞");
	expressionEdit->blockParseStatus(false);
	expressionEdit->displayParseStatus(true, false);
	expressionEdit->complete(NULL, pos);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onToConversionRequested(std::string str) {
	str.insert(0, "➞");
	if(str[str.length() - 1] == ' ') expressionEdit->insertPlainText(QString::fromStdString(str));
	else calculateExpression(true, false, OPERATION_ADD, NULL, false, 0, "", str);
}
void QalculateWindow::onStoreActivated() {
	KnownVariable *v = VariableEditDialog::newVariable(this, expressionEdit->expressionHasChanged() || settings->history_answer.empty() ? NULL : (mstruct_exact.isUndefined() ? mstruct : &mstruct_exact), expressionEdit->expressionHasChanged() ? expressionEdit->toPlainText() : (exact_text.empty() ? QString::fromStdString(result_text) : QString::fromStdString(exact_text)));
	if(v) expressionEdit->updateCompletion();
}
void QalculateWindow::onKeypadActivated(bool b) {
	keypadDock->setVisible(b);
}
void QalculateWindow::onKeypadVisibilityChanged(bool b) {
	keypadAction->setChecked(b);
}
void QalculateWindow::onBasesActivated(bool b) {
	if(b && !bases_shown) basesDock->setFloating(true);
	basesDock->setVisible(b);
}
void QalculateWindow::onBasesVisibilityChanged(bool b) {
	basesAction->setChecked(b);
	if(b && expressionEdit->expressionHasChanged()) onExpressionChanged();
	else if(b && !settings->history_answer.empty()) updateResultBases();
}
bool QalculateWindow::displayMessages(QWidget *parent) {
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
bool QalculateWindow::updateWindowTitle(const QString &str, bool is_result) {
	if(title_modified) return false;
	switch(settings->title_type) {
		case TITLE_RESULT: {
			if(str.isEmpty()) return false;
			if(!str.isEmpty()) setWindowTitle(str);
			break;
		}
		case TITLE_APP_RESULT: {
			if(!str.isEmpty()) setWindowTitle("Qalculate! (" + str + ")");
			break;
		}
		default: {
			if(is_result) return false;
			if(!str.isEmpty()) setWindowTitle("Qalculate! " + str);
			else setWindowTitle("Qalculate!");
		}
	}
	return true;
}

void QalculateWindow::gradiansActivated() {settings->evalops.parse_options.angle_unit = ANGLE_UNIT_GRADIANS; expressionFormatUpdated(true);}
void QalculateWindow::radiansActivated() {settings->evalops.parse_options.angle_unit = ANGLE_UNIT_RADIANS; expressionFormatUpdated(true);}
void QalculateWindow::degreesActivated() {settings->evalops.parse_options.angle_unit = ANGLE_UNIT_DEGREES; expressionFormatUpdated(true);}
void QalculateWindow::normalActivated() {
	settings->printops.sort_options.minus_last = true;
	settings->printops.min_exp = EXP_PRECISION;
	settings->printops.show_ending_zeroes = true;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = true;
	settings->printops.negative_exponents = false;
	resultFormatUpdated();
}
void QalculateWindow::scientificActivated() {
	settings->printops.sort_options.minus_last = false;
	settings->printops.min_exp = EXP_SCIENTIFIC;
	settings->printops.show_ending_zeroes = true;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = false;
	settings->printops.negative_exponents = true;
	resultFormatUpdated();
}
void QalculateWindow::simpleActivated() {
	settings->printops.sort_options.minus_last = true;
	settings->printops.min_exp = EXP_NONE;
	settings->printops.show_ending_zeroes = false;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = true;
	settings->printops.negative_exponents = false;
	resultFormatUpdated();
}
void QalculateWindow::onPrecisionChanged(int v) {
	CALCULATOR->setPrecision(v);
	expressionCalculationUpdated(500);
}
void QalculateWindow::onMinDecimalsChanged(int v) {
	settings->printops.use_min_decimals = (v > 0);
	settings->printops.min_decimals = v;
	resultFormatUpdated(500);
}
void QalculateWindow::onMaxDecimalsChanged(int v) {
	settings->printops.use_max_decimals = (v >= 0);
	settings->printops.max_decimals = v;
	resultFormatUpdated(500);
}
void QalculateWindow::approximationActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	if(v < 0) {
		settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
		if(v == -2) settings->dual_approximation = 1;
		else settings->dual_approximation = -1;
	} else {
		settings->evalops.approximation = (ApproximationMode) v;
	}
	expressionCalculationUpdated();
}
void QalculateWindow::outputBaseActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	to_base = 0;
	to_bits = 0;
	if(v == BASE_CUSTOM) {
		v = customOutputBaseEdit->value();
		if(v > 2 && v <= 36) {
			settings->printops.base = v;
		} else {
			settings->printops.base = BASE_CUSTOM;
			CALCULATOR->setCustomOutputBase(v);
		}
	} else {
		settings->printops.base = v;
	}
	resultFormatUpdated();
}
void QalculateWindow::onCustomOutputBaseChanged(int v) {
	customOutputBaseAction->setChecked(true);
	to_base = 0;
	to_bits = 0;
	if(v > 2 && v <= 36) {
		settings->printops.base = v;
	} else {
		settings->printops.base = BASE_CUSTOM;
		CALCULATOR->setCustomOutputBase(Number(v, 1));
	}
	resultFormatUpdated();
}
void QalculateWindow::inputBaseActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	if(v == BASE_CUSTOM) {
		v = customOutputBaseEdit->value();
		if(v > 2 && v <= 36) {
			settings->evalops.parse_options.base = v;
		} else {
			settings->evalops.parse_options.base = BASE_CUSTOM;
			CALCULATOR->setCustomInputBase(v);
		}
	} else {
		settings->evalops.parse_options.base = v;
	}
	expressionFormatUpdated(false);
}
void QalculateWindow::onCustomInputBaseChanged(int v) {
	customInputBaseAction->setChecked(true);
	if(v > 2 && v <= 36) {
		settings->evalops.parse_options.base = v;
	} else {
		settings->evalops.parse_options.base = BASE_CUSTOM;
		CALCULATOR->setCustomInputBase(Number(v, 1));
	}
	expressionFormatUpdated(false);
}
void QalculateWindow::assumptionsTypeActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
	for(int i = 0; i < 5; i++) {
		if(assumptionSignActions[i]->data().toInt() == CALCULATOR->defaultAssumptions()->sign()) {
			assumptionSignActions[i]->setChecked(true);
			break;
		}
	}
	expressionCalculationUpdated();
}
void QalculateWindow::assumptionsSignActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
	for(int i = 0; i < 4; i++) {
		if(assumptionTypeActions[i]->data().toInt() == CALCULATOR->defaultAssumptions()->type()) {
			assumptionTypeActions[i]->setChecked(true);
			break;
		}
	}
	expressionCalculationUpdated();
}
void QalculateWindow::onAlwaysOnTopChanged() {
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	else setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	show();
}
void QalculateWindow::onTitleTypeChanged() {
	title_modified = false;
	updateWindowTitle();
}
void QalculateWindow::onPreferencesClosed() {
	preferencesDialog->deleteLater();
	preferencesDialog = NULL;
}
void QalculateWindow::onResultFontChanged() {
	if(settings->use_custom_result_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_result_font)); historyView->setFont(font);}
	else historyView->setFont(QApplication::font());
}
void QalculateWindow::onExpressionFontChanged() {
	if(settings->use_custom_expression_font) {
		QFont font; font.fromString(QString::fromStdString(settings->custom_expression_font)); expressionEdit->setFont(font);
	} else {
		QFont font = QApplication::font();
		if(font.pixelSize() >= 0) font.setPixelSize(font.pixelSize() * 1.35);
		else font.setPointSize(font.pointSize() * 1.35);
		expressionEdit->setFont(font);
	}
}
void QalculateWindow::onKeypadFontChanged() {
	if(settings->use_custom_keypad_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_keypad_font)); keypad->setFont(font);}
	else keypad->setFont(QApplication::font());
}
void QalculateWindow::onAppFontChanged() {
	if(settings->use_custom_app_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_app_font)); QApplication::setFont(font);}
	else QApplication::setFont(saved_app_font);
	if(!settings->use_custom_expression_font) {
		QFont font = QApplication::font();
		if(font.pixelSize() >= 0) font.setPixelSize(font.pixelSize() * 1.35);
		else font.setPointSize(font.pointSize() * 1.35);
		expressionEdit->setFont(font);
	}
	if(!settings->use_custom_result_font) historyView->setFont(QApplication::font());
	if(!settings->use_custom_keypad_font) keypad->setFont(QApplication::font());
}
void QalculateWindow::editPreferences() {
	if(preferencesDialog) {
		preferencesDialog->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		preferencesDialog->show();
		preferencesDialog->raise();
		preferencesDialog->activateWindow();
		return;
	}
	preferencesDialog = new PreferencesDialog(this);
	connect(preferencesDialog, SIGNAL(resultFormatUpdated()), this, SLOT(resultFormatUpdated()));
	connect(preferencesDialog, SIGNAL(resultDisplayUpdated()), this, SLOT(resultDisplayUpdated()));
	connect(preferencesDialog, SIGNAL(expressionFormatUpdated(bool)), this, SLOT(expressionFormatUpdated(bool)));
	connect(preferencesDialog, SIGNAL(expressionCalculationUpdated(int)), this, SLOT(expressionCalculationUpdated(int)));
	connect(preferencesDialog, SIGNAL(alwaysOnTopChanged()), this, SLOT(onAlwaysOnTopChanged()));
	connect(preferencesDialog, SIGNAL(titleTypeChanged()), this, SLOT(onTitleTypeChanged()));
	connect(preferencesDialog, SIGNAL(resultFontChanged()), this, SLOT(onResultFontChanged()));
	connect(preferencesDialog, SIGNAL(expressionFontChanged()), this, SLOT(onExpressionFontChanged()));
	connect(preferencesDialog, SIGNAL(keypadFontChanged()), this, SLOT(onKeypadFontChanged()));
	connect(preferencesDialog, SIGNAL(appFontChanged()), this, SLOT(onAppFontChanged()));
	connect(preferencesDialog, SIGNAL(symbolsUpdated()), keypad, SLOT(updateSymbols()));
	connect(preferencesDialog, SIGNAL(dialogClosed()), this, SLOT(onPreferencesClosed()));
	preferencesDialog->show();
}
void QalculateWindow::applyFunction(MathFunction *f) {
	if(b_busy) return;
	/*if(settings->rpn_mode) {
		calculateRPN(f);
		return;
	}*/
	QString str = QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name);
	if(f->args() == 0) {
		str += "()";
	} else {
		str += "(";
		str += expressionEdit->toPlainText();
		str += ")";
	}
	expressionEdit->blockParseStatus();
	expressionEdit->blockUndo();
	expressionEdit->clear();
	expressionEdit->blockUndo(false);
	expressionEdit->setPlainText(str);
	expressionEdit->blockParseStatus(false);
	calculate();
}
void QalculateWindow::openFunctions() {
	if(functionsDialog) {
		functionsDialog->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		functionsDialog->show();
		functionsDialog->raise();
		functionsDialog->activateWindow();
		return;
	}
	functionsDialog = new FunctionsDialog();
	connect(functionsDialog, SIGNAL(itemsChanged()), expressionEdit, SLOT(updateCompletion()));
	connect(functionsDialog, SIGNAL(applyFunctionRequest(MathFunction*)), this, SLOT(applyFunction(MathFunction*)));
	connect(functionsDialog, SIGNAL(insertFunctionRequest(MathFunction*)), this, SLOT(onInsertFunctionRequested(MathFunction*)));
	connect(functionsDialog, SIGNAL(calculateFunctionRequest(MathFunction*)), this, SLOT(onCalculateFunctionRequested(MathFunction*)));
	functionsDialog->show();
}
void QalculateWindow::openFPConversion() {
	if(fpConversionDialog) {
		if(settings->always_on_top) fpConversionDialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		fpConversionDialog->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		fpConversionDialog->show();
		fpConversionDialog->raise();
		fpConversionDialog->activateWindow();
		return;
	}
	fpConversionDialog = new FPConversionDialog(this);
	if(settings->always_on_top) fpConversionDialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	QString str;
	int base;
	if(!expressionEdit->expressionHasChanged() && !settings->history_answer.empty()) {
		str = QString::fromStdString(unhtmlize(result_text));
		if(to_base != 0) base = to_base;
		else base = settings->printops.base;
	} else {
		str = expressionEdit->toPlainText();
		base = settings->evalops.parse_options.base;
	}
	switch(base) {
		case BASE_BINARY: {
			fpConversionDialog->setBin(str);
			break;
		}
		case BASE_HEXADECIMAL: {
			fpConversionDialog->setHex(str);
			break;
		}
		default: {
			fpConversionDialog->setValue(str);
			break;
		}
	}
	fpConversionDialog->show();
}

struct FunctionDialog {
	MathFunction *f;
	QDialog *dialog;
	QPushButton *b_cancel, *b_exec, *b_insert;
	QCheckBox *b_keepopen;
	QLabel *w_result;
	std::vector<QLabel*> label;
	std::vector<QWidget*> entry;
	std::vector<QRadioButton*> boolean_buttons;
	std::vector<int> boolean_index;
	bool add_to_menu, keep_open, rpn;
	int args;
};

MathSpinBox::MathSpinBox(QWidget *parent) : QSpinBox(parent) {
	setLineEdit(new MathLineEdit(this));
	connect(lineEdit(), SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));
}
MathSpinBox::~MathSpinBox() {}

int MathSpinBox::valueFromText(const QString &text) const {
	if(settings->evalops.parse_options.base != BASE_DECIMAL) return QSpinBox::valueFromText(text);
	bool b = false;
	int v = text.toInt(&b);
	if(b) return v;
	MathStructure value;
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo = settings->evalops;
	eo.parse_options.base = 10;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(text.toStdString(), eo.parse_options), 100, eo);
	CALCULATOR->endTemporaryStopMessages();
	return value.number().intValue();
}
QValidator::State MathSpinBox::validate(QString &text, int &pos) const {
	if(settings->evalops.parse_options.base != BASE_DECIMAL) return QSpinBox::validate(text, pos);
	std::string str = text.trimmed().toStdString();
	if(str.empty()) return QValidator::Intermediate;
	if(str.find_first_not_of(NUMBERS) == std::string::npos) return QValidator::Acceptable;
	if(last_is_operator(str)) return QValidator::Intermediate;
	MathStructure value;
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo = settings->evalops;
	eo.parse_options.base = 10;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(text.toStdString(), eo.parse_options), 100, eo);
	CALCULATOR->endTemporaryStopMessages();
	if(value.isNumber()) return QValidator::Acceptable;
	return QValidator::Intermediate;
}
QLineEdit *MathSpinBox::entry() const {
	return lineEdit();
}
void MathSpinBox::keyPressEvent(QKeyEvent *event) {
	QSpinBox::keyPressEvent(event);
	if(event->key() == Qt::Key_Return) event->accept();
}

class MathDateTimeEdit : public QDateTimeEdit {

	public:

		MathDateTimeEdit(QWidget *parent = NULL) : QDateTimeEdit(parent) {}
		virtual ~MathDateTimeEdit() {}

		QLineEdit *entry() const {return lineEdit();}

	protected:

		void keyPressEvent(QKeyEvent *event) override {
			QDateTimeEdit::keyPressEvent(event);
			if(event->key() == Qt::Key_Return) event->accept();
		}

};

void QalculateWindow::onCalculateFunctionRequested(MathFunction *f) {
	insertFunction(f, functionsDialog);
}
void QalculateWindow::onInsertFunctionRequested(MathFunction *f) {
	expressionEdit->blockCompletion();
	expressionEdit->blockParseStatus();
	if(!expressionEdit->expressionHasChanged()) {
		expressionEdit->blockUndo(true);
		expressionEdit->clear();
		expressionEdit->blockUndo(false);
	}
	QTextCursor cur = expressionEdit->textCursor();
	expressionEdit->wrapSelection(QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name), true, true);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::insertFunction(MathFunction *f, QWidget *parent) {
	if(!f) return;

	//if function takes no arguments, do not display dialog and insert function directly
	if(f->args() == 0) {
		expressionEdit->insertPlainText(QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name) + "()");
		//function_inserted(f);
		return;
	}

	FunctionDialog *fd = new FunctionDialog;

	int args = 0;
	bool has_vector = false;
	if(f->args() > 0) {
		args = f->args();
	} else if(f->minargs() > 0) {
		args = f->minargs() + 1;
		has_vector = true;
	} else {
		args = 1;
		has_vector = true;
	}
	fd->args = args;
	fd->rpn = settings->rpn_mode && expressionEdit->document()->isEmpty() && CALCULATOR->RPNStackSize() >= (f->minargs() <= 0 ? 1 : (size_t) f->minargs());
	fd->add_to_menu = true;
	fd->f = f;

	std::string f_title = f->title(true);
	fd->dialog = new QDialog(parent ? parent : this);
	if(settings->always_on_top) fd->dialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	fd->dialog->setWindowTitle(QString::fromStdString(f_title));

	QVBoxLayout *box = new QVBoxLayout(fd->dialog);

	box->addSpacing(box->spacing());
	QLabel *titleLabel = new QLabel("<big>" + QString::fromStdString(f_title) + "</big>");
	box->addWidget(titleLabel);
	box->addSpacing(box->spacing());

	QGridLayout *table = new QGridLayout();
	box->addLayout(table);

	box->addSpacing(box->spacing());
	if(!f->description().empty() || !f->example(true).empty()) {
		QTextBrowser *descr = new QTextBrowser();
		box->addWidget(descr);
		QString str = QString::fromStdString(f->description());
		if(!f->example(true).empty()) {
			if(!str.isEmpty()) str += "\n\n";
			str += tr("Example:");
			str += " ";
			str += QString::fromStdString(f->example(false));
		}
		str.replace(">=", SIGN_GREATER_OR_EQUAL);
		str.replace("<=", SIGN_LESS_OR_EQUAL);
		str.replace("!=", SIGN_NOT_EQUAL);
		descr->setPlainText(str);
		QFontMetrics fm(descr->font());
		descr->setFixedHeight(fm.lineSpacing() * 5 + descr->frameWidth() * 2 + descr->contentsMargins().top() + descr->contentsMargins().bottom());
	}

	fd->w_result = new QLabel();
	fd->w_result->setTextInteractionFlags(Qt::TextSelectableByMouse);
	fd->w_result->setWordWrap(true);
	fd->w_result->setAlignment(Qt::AlignRight);
	QFont font(fd->w_result->font());
	font.setWeight(QFont::Bold);
	QFontMetrics fm(font);
	fd->w_result->setFixedHeight(fm.lineSpacing() * 2 + fd->w_result->frameWidth() * 2 + fd->w_result->contentsMargins().top() + fd->w_result->contentsMargins().bottom());
	fd->w_result->setFixedWidth(fm.averageCharWidth() * 40);
	box->addWidget(fd->w_result, Qt::AlignRight);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, fd->dialog);
	box->addWidget(buttonBox);
	fd->b_cancel = buttonBox->button(QDialogButtonBox::Close);
	fd->b_exec = buttonBox->addButton(settings->rpn_mode ? tr("Enter", "RPN Enter") : tr("Calculate"), QDialogButtonBox::ApplyRole);
	fd->b_insert = buttonBox->addButton(settings->rpn_mode ? tr("Apply to stack") : tr("Insert"), QDialogButtonBox::AcceptRole);
	if(settings->rpn_mode && CALCULATOR->RPNStackSize() < (f->minargs() <= 0 ? 1 : (size_t) f->minargs())) fd->b_insert->setEnabled(false);
	fd->b_keepopen = new QCheckBox(tr("Keep open"));
	fd->keep_open = settings->keep_function_dialog_open;
	fd->b_keepopen->setChecked(fd->keep_open);
	buttonBox->addButton(fd->b_keepopen, QDialogButtonBox::ActionRole);
	box->addWidget(buttonBox);

	box->setSizeConstraint(QLayout::SetFixedSize);

	fd->label.resize(args, NULL);
	fd->entry.resize(args, NULL);
	fd->boolean_index.resize(args, 0);

	int bindex = 0;
	QString argstr, typestr, defstr;
	QString freetype = QString::fromStdString(Argument().printlong());
	Argument *arg;
	int r = 0;
	for(int i = 0; i < args; i++) {
		arg = f->getArgumentDefinition(i + 1);
		if(!arg || arg->name().empty()) {
			if(args == 1) {
				argstr = tr("Value");
			} else {
				argstr = tr("Argument");
				argstr += " ";
				argstr += QString::number(i + 1);
			}
		} else {
			argstr = QString::fromStdString(arg->name());
		}
		typestr = "";
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		defstr = QString::fromStdString(CALCULATOR->localizeExpression(f->getDefaultValue(i + 1), pa));
		if(arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && defstr.length() >= 2 && defstr[0] == '\"' && defstr[defstr.length() - 1] == '\"') {
			defstr = defstr.mid(1, defstr.length() - 2);
		}
		fd->label[i] = new QLabel(argstr);
		fd->label[i]->setAlignment(Qt::AlignRight);
		QWidget *entry = NULL;
		if(arg) {
			switch(arg->type()) {
				case ARGUMENT_TYPE_INTEGER: {
					IntegerArgument *iarg = (IntegerArgument*) arg;
					int min = INT_MIN, max = INT_MAX;
					if(iarg->min()) {
						min = iarg->min()->intValue();
					}
					if(iarg->max()) {
						max = iarg->max()->intValue();
					}
					MathSpinBox *spin = new MathSpinBox();
					entry = spin;
					fd->entry[i] = spin->entry();
					spin->setRange(min, max);
					spin->setAlignment(Qt::AlignRight);
					if(!defstr.isEmpty()) {
						spin->setValue(defstr.toInt());
					} else if(!arg->zeroForbidden() && min <= 0 && max >= 0) {
						spin->setValue(0);
					} else {
						if(max < 0) {
							spin->setValue(max);
						} else if(min <= 1) {
							spin->setValue(1);
						} else {
							spin->setValue(min);
						}
					}
					spin->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
					connect(spin, SIGNAL(textChanged(const QString&)), this, SLOT(onInsertFunctionChanged()));
					connect(spin, SIGNAL(valueChanged(int)), this, SLOT(onInsertFunctionChanged()));
					connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
					break;
				}
				case ARGUMENT_TYPE_DATE: {
					MathDateTimeEdit *dateEdit = new MathDateTimeEdit();
					entry = dateEdit;
					if(defstr == "now") {
						dateEdit->setDateTime(QDateTime::currentDateTime());
						dateEdit->setDisplayFormat("yyyy-MM-ddTHH:mm:ss");
					} else {
						dateEdit->setDate(QDate::currentDate());
						dateEdit->setDisplayFormat("yyyy-MM-dd");
						dateEdit->setCalendarPopup(true);
					}
					dateEdit->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
					fd->entry[i] = dateEdit->entry();
					dateEdit->setAlignment(Qt::AlignRight);
					connect(dateEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(onInsertFunctionChanged()));
					connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
					break;
				}
				case ARGUMENT_TYPE_BOOLEAN: {
					fd->boolean_index[i] = bindex;
					bindex += 2;
					fd->entry[i] = new QWidget();
					QHBoxLayout *hbox = new QHBoxLayout(fd->entry[i]);
					QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
					QRadioButton *w = new QRadioButton(tr("True")); group->addButton(w); hbox->addWidget(w);
					w->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
					fd->boolean_buttons.push_back(w);
					w = new QRadioButton(tr("False")); group->addButton(w); hbox->addWidget(w);
					w->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
					fd->boolean_buttons.push_back(w); w->setChecked(true);
					hbox->addStretch(1);
					connect(fd->boolean_buttons[fd->boolean_buttons.size() - 2], SIGNAL(toggled(bool)), this, SLOT(onInsertFunctionChanged()));
					connect(fd->boolean_buttons[fd->boolean_buttons.size() - 1], SIGNAL(toggled(bool)), this, SLOT(onInsertFunctionChanged()));
					break;
				}
				case ARGUMENT_TYPE_DATA_PROPERTY: {
					if(f->subtype() == SUBTYPE_DATA_SET) {
						QComboBox *w = new QComboBox();
						fd->entry[i] = w;
						DataPropertyIter it;
						DataSet *ds = (DataSet*) f;
						DataProperty *dp = ds->getFirstProperty(&it);
						/*if(fd->rpn && (size_t) i < CALCULATOR->RPNStackSize()) {
							GtkTreeIter iter;
							if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, i)) {
								gchar *gstr;
								gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
								defstr = gstr;
								g_free(gstr);
							}
						}*/
						int active_index = -1;
						for(int i = 0; dp; i++) {
							if(!dp->isHidden()) {
								w->addItem(QString::fromStdString(dp->title()), QVariant::fromValue((void*) dp));
								if(active_index < 0 && defstr.toStdString() == dp->getName()) {
									active_index = i;
								}
							}
							dp = ds->getNextProperty(&it);
						}
						w->addItem(tr("Info"), QVariant::fromValue((void*) NULL));
						if(active_index < 0) active_index = i + 1;
						w->setCurrentIndex(active_index);
						connect(w, SIGNAL(currentTextChanged(const QString&)), this, SLOT(onInsertFunctionChanged()));
						break;
					}
				}
				default: {
					typestr = QString::fromStdString(arg->printlong());
					if(typestr == freetype) typestr = "";
					if(i == 1 && f == CALCULATOR->f_ascii && arg->type() == ARGUMENT_TYPE_TEXT) {
						QComboBox *combo = new QComboBox();
						combo->setEditable(true);
						combo->addItem("UTF-8");
						combo->addItem("UTF-16");
						combo->addItem("UTF-32");
						fd->entry[i] = combo->lineEdit();
						entry = combo;
					} else if(i == 3 && f == CALCULATOR->f_date && arg->type() == ARGUMENT_TYPE_TEXT) {
						QComboBox *combo = new QComboBox();
						combo->setEditable(true);
						combo->addItem("chinese");
						combo->addItem("coptic");
						combo->addItem("egyptian");
						combo->addItem("ethiopian");
						combo->addItem("gregorian");
						combo->addItem("hebrew");
						combo->addItem("indian");
						combo->addItem("islamic");
						combo->addItem("julian");
						combo->addItem("milankovic");
						combo->addItem("persian");
						entry = combo;
						fd->entry[i] = combo->lineEdit();
					} else {
						fd->entry[i] = new MathLineEdit();
					}
					if(i >= f->minargs() && !has_vector) {
						((QLineEdit*) fd->entry[i])->setPlaceholderText(tr("optional", "optional parameter"));
					}
					((QLineEdit*) fd->entry[i])->setAlignment(Qt::AlignRight);
					connect(fd->entry[i], SIGNAL(textEdited(const QString&)), this, SLOT(onInsertFunctionChanged()));
					connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
				}
			}
		} else {
			fd->entry[i] = new MathLineEdit();
			if(i >= f->minargs() && !has_vector) {
				((QLineEdit*) fd->entry[i])->setPlaceholderText(tr("optional", "optional parameter"));
			}
			((QLineEdit*) fd->entry[i])->setAlignment(Qt::AlignRight);
			connect(fd->entry[i], SIGNAL(textEdited(const QString&)), this, SLOT(onInsertFunctionChanged()));
			connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
		}
		if(arg && arg->type() == ARGUMENT_TYPE_FILE) {
			QAction *action = ((QLineEdit*) fd->entry[i])->addAction(LOAD_ICON("document-open"), QLineEdit::TrailingPosition);
			action->setProperty("QALCULATE ENTRY", QVariant::fromValue((void*) fd->entry[i]));
			connect(action, SIGNAL(triggered()), this, SLOT(onEntrySelectFile()));
		/*} else if(arg && (arg->type() == ARGUMENT_TYPE_VECTOR || arg->type() == ARGUMENT_TYPE_MATRIX)) {
			QPushButton *w = new QPushButton(typestr);
			if(arg->type() == ARGUMENT_TYPE_VECTOR) g_signal_connect(G_OBJECT(fd->type_label[i]), "clicked", G_CALLBACK(on_type_label_vector_clicked), (gpointer) fd->entry[i]);
			else g_signal_connect(G_OBJECT(fd->type_label[i]), "clicked", G_CALLBACK(on_type_label_matrix_clicked), (gpointer) fd->entry[i]);*/
		}
		/*if(fd->rpn && (size_t) i < CALCULATOR->RPNStackSize()) {
			GtkTreeIter iter;
			if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, i)) {
				gchar *gstr;
				gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
				if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
					if(g_strcmp0(gstr, "1") == 0) {
						g_signal_handlers_block_matched((gpointer) fd->boolean_buttons[fd->boolean_buttons.size() - 2], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fd->boolean_buttons[fd->boolean_buttons.size() - 2]), TRUE);
						g_signal_handlers_unblock_matched((gpointer) fd->boolean_buttons[fd->boolean_buttons.size() - 2], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
					}
				} else if(fd->properties_store && arg && arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) {
				} else {
					g_signal_handlers_block_matched((gpointer) fd->entry[i], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
					if(i == 0 && args == 1 && (has_vector || arg->type() == ARGUMENT_TYPE_VECTOR)) {
						string rpn_vector = gstr;
						while(gtk_tree_model_iter_next(GTK_TREE_MODEL(stackstore), &iter)) {
							g_free(gstr);
							gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
							rpn_vector += CALCULATOR->getComma();
							rpn_vector += " ";
							rpn_vector += gstr;
						}
						gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), rpn_vector.c_str());
					} else {
						gtk_entry_set_text(GTK_ENTRY(fd->entry[i]), gstr);
						if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
							gtk_spin_button_update(GTK_SPIN_BUTTON(fd->entry[i]));
						}
					}
					g_signal_handlers_unblock_matched((gpointer) fd->entry[i], G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_insert_function_changed, NULL);
				}
				g_free(gstr);
			}
		} else */if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(defstr == "1") {
				fd->boolean_buttons[fd->boolean_buttons.size() - 2]->blockSignals(true);
				fd->boolean_buttons[fd->boolean_buttons.size() - 2]->setChecked(true);
				fd->boolean_buttons[fd->boolean_buttons.size() - 2]->blockSignals(false);
			}
		} else if(!arg || arg->type() != ARGUMENT_TYPE_DATA_PROPERTY || f->subtype() != SUBTYPE_DATA_SET) {
			fd->entry[i]->blockSignals(true);
			if(!defstr.isEmpty() && (!arg || (arg->type() != ARGUMENT_TYPE_DATE && arg->type() != ARGUMENT_TYPE_INTEGER))&& (i < f->minargs() || has_vector || (defstr != "undefined" && defstr != "\"\""))) {
				((QLineEdit*) fd->entry[i])->setText(defstr);
			}
			if(i == 0) {
				std::string seltext, str2;
				if(expressionEdit->textCursor().hasSelection()) seltext = expressionEdit->textCursor().selectedText().toStdString();
				else seltext = expressionEdit->toPlainText().toStdString();
				CALCULATOR->separateToExpression(seltext, str2, settings->evalops, true);
				remove_blank_ends(seltext);
				if(!seltext.empty()) {
					if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
						MathStructure m;
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(seltext, settings->evalops.parse_options), 200, settings->evalops);
						if(CALCULATOR->endTemporaryStopMessages() && m.isInteger()) {
							bool overflow = false;
							int v = m.number().intValue(&overflow);
							QSpinBox *spin = (QSpinBox*) entry;
							if(!overflow && v >= spin->minimum() && v <= spin->maximum()) spin->setValue(v);
						}
					} else if(arg && arg->type() == ARGUMENT_TYPE_DATE) {
						MathStructure m;
						CALCULATOR->beginTemporaryStopMessages();
						CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(seltext, settings->evalops.parse_options), 200, settings->evalops);
						if(CALCULATOR->endTemporaryStopMessages() && m.isDateTime()) {
							QDateTime d;
							d.setDate(QDate(m.datetime()->year(), m.datetime()->month(), m.datetime()->day()));
							Number nr_sec = m.datetime()->second();
							Number nr_msec(nr_sec); nr_msec.frac(); nr_msec *= 1000; nr_msec.round();
							nr_sec.trunc();
							d.setTime(QTime(m.datetime()->hour(), m.datetime()->minute(), nr_sec.intValue(), nr_msec.intValue()));
							((QDateTimeEdit*) entry)->setDateTime(d);
						}
					} else {
						((QLineEdit*) fd->entry[i])->setText(QString::fromStdString(seltext));
					}
				}
			}
			fd->entry[i]->blockSignals(false);
		}
		table->addWidget(fd->label[i], r, 0, 1, 1);
		if(entry) table->addWidget(entry, r, 1, 1, 1);
		else table->addWidget(fd->entry[i], r, 1, 1, 1);
		r++;
		fd->entry[i]->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
		if(i == 0) fd->entry[i]->setFocus();
		if(!typestr.isEmpty()) {
			typestr.replace(">=", SIGN_GREATER_OR_EQUAL);
			typestr.replace("<=", SIGN_LESS_OR_EQUAL);
			typestr.replace("!=", SIGN_NOT_EQUAL);
			QLabel *w = new QLabel("<i>" + typestr.toHtmlEscaped() + "</i>");
			w->setAlignment(Qt::AlignRight | Qt::AlignTop);
			table->addWidget(w, r, 1, 1, 1);
			r++;
		}
	}
	table->setColumnStretch(1, 1);

	fd->b_exec->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_cancel->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_insert->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_keepopen->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_exec->setDefault(false);
	fd->b_cancel->setDefault(false);
	fd->b_insert->setDefault(false);
	fd->dialog->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	connect(fd->b_exec, SIGNAL(clicked()), this, SLOT(onInsertFunctionExec()));
	if(fd->rpn) connect(fd->b_insert, SIGNAL(clicked()), this, SLOT(onInsertFunctionRPN()));
	else connect(fd->b_insert, SIGNAL(clicked()), this, SLOT(onInsertFunctionInsert()));
	connect(fd->b_cancel, SIGNAL(clicked()), fd->dialog, SLOT(reject()));
	connect(fd->b_keepopen, SIGNAL(toggled(bool)), this, SLOT(onInsertFunctionKeepOpen(bool)));
	connect(fd->dialog, SIGNAL(rejected()), this, SLOT(onInsertFunctionClosed()));

	fd->dialog->show();

}
void QalculateWindow::onEntrySelectFile() {
	QLineEdit *w = (QLineEdit*) sender()->property("QALCULATE ENTRY").value<void*>();
	FunctionDialog *fd = (FunctionDialog*) w->property("QALCULATE FD").value<void*>();
	QString str = QFileDialog::getOpenFileName(fd->dialog, QString(), w->text());
	if(!str.isEmpty()) w->setText(str);
}
void QalculateWindow::insertFunctionDo(FunctionDialog *fd) {
	MathFunction *f = fd->f;
	std::string str = f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).name + "(", str2;
	int argcount = fd->args;
	if(f->maxargs() > 0 && f->minargs() < f->maxargs() && argcount > f->minargs()) {
		while(true) {
			ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
			std::string defstr = CALCULATOR->localizeExpression(f->getDefaultValue(argcount), pa);
			remove_blank_ends(defstr);
			if(f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_BOOLEAN) {
				if(fd->boolean_buttons[fd->boolean_index[argcount - 1]]->isChecked()) str2 = "1";
				else str2 = "0";
			} else if(settings->evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_INTEGER) {
				Number nr(((QLineEdit*) fd->entry[argcount - 1])->text().toStdString());
				str2 = print_with_evalops(nr);
			} else if(f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_DATA_PROPERTY && f->subtype() == SUBTYPE_DATA_SET) {
				DataProperty *dp = (DataProperty*) ((QComboBox*) fd->entry[argcount - 1])->currentData().value<void*>();
				if(dp) {
					str2 = dp->getName();
				} else {
					str2 = "info";
				}
			} else {
				str2 = ((QLineEdit*) fd->entry[argcount - 1])->text().toStdString();
				remove_blank_ends(str2);
			}
			if(!str2.empty() && f->getArgumentDefinition(argcount) && (f->getArgumentDefinition(argcount)->suggestsQuotes() || (f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_TEXT && str2.find(CALCULATOR->getComma()) == std::string::npos))) {
				if(str2.length() < 1 || (str2[0] != '\"' && str[0] != '\'')) {
					str2.insert(0, "\"");
					str2 += "\"";
				}
			}
			if(str2.empty() || str2 == defstr) argcount--;
			else break;
			if(argcount == 0 || argcount == f->minargs()) break;
		}
	}

	int i_vector = f->maxargs() > 0 ? f->maxargs() : argcount;
	for(int i = 0; i < argcount; i++) {
		if(f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(fd->boolean_buttons[fd->boolean_index[i]]->isChecked()) str2 = "1";
			else str2 = "0";
		} else if((i != (f->maxargs() > 0 ? f->maxargs() : argcount) - 1 || i_vector == i - 1) && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_VECTOR) {
			i_vector = i;
			str2 = ((QLineEdit*) fd->entry[i])->text().toStdString();
			remove_blank_ends(str2);
			if(str2.find_first_of(PARENTHESISS VECTOR_WRAPS) == std::string::npos && str2.find_first_of(CALCULATOR->getComma() == COMMA ? COMMAS : CALCULATOR->getComma()) != std::string::npos) {
				str2.insert(0, 1, '[');
				str2 += ']';
			}
		} else if(settings->evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_INTEGER) {
			Number nr(((QLineEdit*) fd->entry[i])->text().toStdString());
			str2 = print_with_evalops(nr);
		} else if(f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_DATA_PROPERTY && f->subtype() == SUBTYPE_DATA_SET) {
			DataProperty *dp = (DataProperty*) ((QComboBox*) fd->entry[i])->currentData().value<void*>();
			if(dp) {
				str2 = dp->getName();
			} else {
				str2 = "info";
			}
		} else {
			str2 = ((QLineEdit*) fd->entry[i])->text().toStdString();
			remove_blank_ends(str2);
		}
		if((i < f->minargs() || !str2.empty()) && f->getArgumentDefinition(i + 1) && (f->getArgumentDefinition(i + 1)->suggestsQuotes() || (f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_TEXT && str2.find(CALCULATOR->getComma()) == std::string::npos))) {
			if(str2.length() < 1 || (str2[0] != '\"' && str[0] != '\'')) {
				str2.insert(0, "\"");
				str2 += "\"";
			}
		}
		if(i > 0) {
			str += CALCULATOR->getComma();
			str += " ";
		}
		str += str2;
	}
	str += ")";
	expressionEdit->blockParseStatus(true);
	expressionEdit->blockCompletion(true);
	expressionEdit->insertPlainText(QString::fromStdString(str));
	expressionEdit->blockCompletion(false);
	expressionEdit->blockParseStatus(false);
	//if(fd->add_to_menu) function_inserted(f);
}

void QalculateWindow::onInsertFunctionChanged() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	fd->w_result->clear();
}
void QalculateWindow::onInsertFunctionEntryActivated() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	for(int i = 0; i < fd->args; i++) {
		if(fd->entry[i] == sender()) {
			if(i == fd->args - 1) {
				if(fd->rpn) onInsertFunctionRPN();
				else if(fd->keep_open || settings->rpn_mode) onInsertFunctionExec();
				else onInsertFunctionInsert();
			} else {
				if(fd->f->getArgumentDefinition(i + 2) && fd->f->getArgumentDefinition(i + 2)->type() == ARGUMENT_TYPE_BOOLEAN) {
					fd->boolean_buttons[fd->boolean_index[i + 1]]->setFocus();
				} else {
					fd->entry[i + 1]->setFocus();
				}
			}
			break;
		}
	}
}
void QalculateWindow::onInsertFunctionExec() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	expressionEdit->blockUndo(true);
	expressionEdit->clear();
	expressionEdit->blockUndo(false);
	if(!fd->keep_open) fd->dialog->hide();
	insertFunctionDo(fd);
	calculateExpression();
	if(fd->keep_open) {
		QString str;
		bool b_approx = *settings->printops.is_approximate || (mstruct && mstruct->isApproximate());
		str = "<span font-weight=\"bold\">";
		if(!b_approx) str += "= ";
		else str += SIGN_ALMOST_EQUAL " ";
		str += QString::fromStdString(result_text);
		str += "</span>";
		fd->w_result->setText(str);
		fd->entry[0]->setFocus();
		expressionEdit->selectAll();
	} else {
		fd->dialog->deleteLater();
		delete fd;
	}
}
void QalculateWindow::onInsertFunctionRPN() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	if(!fd->keep_open) fd->dialog->hide();
	//calculateRPN(f);
	//if(fd->add_to_menu) function_inserted(f);
	if(fd->keep_open) {
		fd->entry[0]->setFocus();
		expressionEdit->selectAll();
	} else {
		fd->dialog->deleteLater();
		delete fd;
	}
}
void QalculateWindow::onInsertFunctionInsert() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	if(!fd->keep_open) fd->dialog->hide();
	insertFunctionDo(fd);
	if(fd->keep_open) {
		fd->entry[0]->setFocus();
		expressionEdit->selectAll();
	} else {
		fd->dialog->deleteLater();
		delete fd;
	}
}
void QalculateWindow::onInsertFunctionKeepOpen(bool b) {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	fd->keep_open = b;
	settings->keep_function_dialog_open = b;
}
void QalculateWindow::onInsertFunctionClosed() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	fd->dialog->deleteLater();
	delete fd;
}
void QalculateWindow::executeFromFile(const QString &file) {
	QFile qfile(file);
	if(!qfile.open(QIODevice::ReadOnly)) {
		qDebug() << tr("Failed to open %1.\n%1").arg(file).arg(qfile.errorString());
		return;
	}
	char buffer[10000];
	std::string str, scom;
	size_t ispace;
	bool rpn_save = settings->rpn_mode;
	settings->rpn_mode = false;
	previous_expression = "";
	expressionEdit->blockUndo(true);
	expressionEdit->blockCompletion(true);
	expressionEdit->blockParseStatus(true);
	block_expression_history = true;
	while(qfile.readLine(buffer, 10000)) {
		str = buffer;
		remove_blank_ends(str);
		ispace = str.find_first_of(SPACES);
		if(ispace == std::string::npos) scom = "";
		else scom = str.substr(0, ispace);
		if(equalsIgnoreCase(str, "exrates") || equalsIgnoreCase(str, "stack") || equalsIgnoreCase(str, "swap") || equalsIgnoreCase(str, "rotate") || equalsIgnoreCase(str, "copy") || equalsIgnoreCase(str, "clear stack") || equalsIgnoreCase(str, "exact") || equalsIgnoreCase(str, "approximate") || equalsIgnoreCase(str, "approx") || equalsIgnoreCase(str, "factor") || equalsIgnoreCase(str, "partial fraction") || equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand") || equalsIgnoreCase(str, "mode") || equalsIgnoreCase(str, "help") || equalsIgnoreCase(str, "?") || equalsIgnoreCase(str, "list") || equalsIgnoreCase(str, "exit") || equalsIgnoreCase(str, "quit") || equalsIgnoreCase(scom, "variable") || equalsIgnoreCase(scom, "function") || equalsIgnoreCase(scom, "set") || equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store") || equalsIgnoreCase(scom, "swap") || equalsIgnoreCase(scom, "delete") || equalsIgnoreCase(scom, "assume") || equalsIgnoreCase(scom, "base") || equalsIgnoreCase(scom, "rpn") || equalsIgnoreCase(scom, "move") || equalsIgnoreCase(scom, "rotate") || equalsIgnoreCase(scom, "copy") || equalsIgnoreCase(scom, "pop") || equalsIgnoreCase(scom, "convert") || (equalsIgnoreCase(scom, "to") && scom != "to") || equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) str.insert(0, 1, '/');
		if(!str.empty()) calculateExpression(true, false, OPERATION_ADD, NULL, false, 0, "", str, false);
	}
	expressionEdit->clear();
	expressionEdit->setExpressionHasChanged(true);
	if(parsed_mstruct) parsed_mstruct->clear();
	if(parsed_tostruct) parsed_tostruct->setUndefined();
	expressionEdit->blockUndo(false);
	expressionEdit->blockCompletion(false);
	expressionEdit->blockParseStatus(false);
	block_expression_history = false;
	settings->rpn_mode = rpn_save;
	previous_expression = "";
	if(mstruct) {
		if(settings->rpn_mode) {
			mstruct->unref();
			mstruct = CALCULATOR->getRPNRegister(1);
			if(!mstruct) mstruct = new MathStructure();
			else mstruct->ref();
		} else {
			mstruct->clear();
		}
	}
	qfile.close();
}

