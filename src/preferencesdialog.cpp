/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QToolTip>
#include <QComboBox>
#include <QLabel>
#include <QScrollArea>
#include <QFontDialog>
#include <QStyleFactory>
#include <QMessageBox>
#include <QApplication>
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
#	include <QStyleHints>
#endif
#include <QDebug>

#include "qalculateqtsettings.h"
#include "preferencesdialog.h"

#define BOX(t, x, f) box = new QCheckBox(t, this); l->addWidget(box, r, 0, 1, 2); box->setChecked(x); connect(box, SIGNAL(toggled(bool)), this, SLOT(f)); r++;
#define BOX1(t, x, f) box = new QCheckBox(t, this); l->addWidget(box, r, 0); box->setChecked(x); connect(box, SIGNAL(toggled(bool)), this, SLOT(f));

QString font_string(std::string str) {
	size_t i = str.find(",");
	if(i != std::string::npos && i < str.length() - 1) {
		if(str[i + 1] != ' ') str.insert(i + 1, 1, ' ');
		i = str.find(",", i + 1);
		if(i != std::string::npos) str = str.substr(0, i);
	}
	return QString::fromStdString(str);
}

class PreferencesPageScroll : public QScrollArea {
	protected:
		QWidget *child_widget;
		bool use_child_hint;
	public:
		PreferencesPageScroll(QWidget *parent, QWidget *child, bool b) : QScrollArea(parent), child_widget(child), use_child_hint(b) {
			setWidget(child);
		}
		QSize sizeHint() const {
			if(use_child_hint) {
				QSize s = child_widget->sizeHint();
				s.setHeight(s.height() + 1);
				return s;
			}
			return QScrollArea::sizeHint();
		}
};

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle(tr("Preferences"));
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QTabWidget *tabs = new QTabWidget(this);
	topbox->addWidget(tabs);
	QWidget *w1 = new QWidget(this);
	QWidget *w2 = new QWidget(this);
	QWidget *w3 = new QWidget(this);
	QWidget *w4 = new QWidget(this);
	QWidget *w5 = new QWidget(this);
	QAbstractButton *box; QGridLayout *l; QComboBox *combo;
	int r = 0;
	l = new QGridLayout(w1);
	BOX(tr("Ignore system language (requires restart)"), settings->ignore_locale, ignoreLocaleToggled(bool));
	ignoreLocaleBox = box;
	QLabel *label = new QLabel(tr("Language:"), this);
	l->addWidget(label, r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Default"));
	combo->addItem("Català");
	combo->addItem("Deutsch");
	combo->addItem("English");
	combo->addItem("Español");
	combo->addItem("Français");
	combo->addItem("ქართული ენა");
	combo->addItem("Nederlands");
	combo->addItem("Português");
	combo->addItem("Português do Brasil");
	combo->addItem("Русский");
	combo->addItem("Slovenščina");
	combo->addItem("Svenska");
	combo->addItem("简体中文");
	combo->addItem("繁體中文");
	QString lang = settings->custom_lang.left(2);
	if(lang == "ca") combo->setCurrentIndex(1);
	else if(lang == "de") combo->setCurrentIndex(2);
	else if(lang == "en") combo->setCurrentIndex(3);
	else if(lang == "es") combo->setCurrentIndex(4);
	else if(lang == "fr") combo->setCurrentIndex(5);
	else if(lang == "ka") combo->setCurrentIndex(6);
	else if(lang == "nl") combo->setCurrentIndex(7);
	else if(settings->custom_lang == "pt_PT") combo->setCurrentIndex(8);
	else if(lang == "pt") combo->setCurrentIndex(9);
	else if(lang == "ru") combo->setCurrentIndex(10);
	else if(lang == "sl") combo->setCurrentIndex(11);
	else if(lang == "sv") combo->setCurrentIndex(12);
	else if(settings->custom_lang == "zh_CN") combo->setCurrentIndex(13);
	else if(settings->custom_lang == "zh_TW") combo->setCurrentIndex(14);
	else combo->setCurrentIndex(0);
	combo->setEnabled(!settings->ignore_locale);
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(langChanged(int)));
	l->addWidget(combo, r, 1); r++; langCombo = combo;
	BOX(tr("Allow multiple instances"), settings->allow_multiple_instances > 0, multipleInstancesToggled(bool));
	BOX(tr("Notify when a new version is available"), settings->check_version, checkVersionToggled(bool));
	BOX(tr("Clear history on exit"), settings->clear_history_on_exit, clearHistoryToggled(bool));
	l->addWidget(new QLabel(tr("Max history lines saved:"), this), r, 0); QSpinBox *spin = new QSpinBox(this); spin->setRange(0, 100000); spin->setValue(settings->max_history_lines); connect(spin, SIGNAL(valueChanged(int)), this, SLOT(maxHistoryLinesChanged(int))); l->addWidget(spin, r, 1); r++;
	BOX(tr("Close application with Escape key"), settings->close_with_esc, closeWithEscToggled(bool));
	BOX(tr("Use keyboard keys for RPN"), settings->rpn_keys && settings->evalops.parse_options.parsing_mode != PARSING_MODE_RPN, rpnKeysToggled(bool)); rpnKeysBox = box; rpnKeysBox->setEnabled(settings->evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
	BOX(tr("Use caret for bitwise XOR"), settings->caret_as_xor, caretAsXorToggled(bool));
	BOX(tr("Keep above other windows"), settings->always_on_top, keepAboveToggled(bool));
	QCheckBox *box2 = new QCheckBox(tr("Preserve history height"), this); l->addWidget(box2, r, 0, 1, 2); connect(box2, SIGNAL(stateChanged(int)), this, SLOT(preserveHeightChanged(int))); r++;
	preserveHeightBox = box2;
	if(settings->preserve_history_height < 0) {
		box2->setTristate(true);
		box2->setCheckState(Qt::PartiallyChecked);
	} else {
		box2->setChecked(settings->preserve_history_height > 0);
	}
	box2->setToolTip(tr("Do not change the height of history list when keypad or number bases are show or hidden."));
	BOX(tr("Place expression field below history (experimental)"), settings->expression_pos == 1, expressionPositionToggled(bool));
	BOX(tr("Show status bar"), settings->show_statusbar || settings->status_in_statusbar, showStatusBarToggled(bool));
	l->addWidget(new QLabel(tr("Window title:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Application name"), TITLE_APP);
	combo->addItem(tr("Result"), TITLE_RESULT);
	combo->addItem(tr("Application name + result"), TITLE_APP_RESULT);
	combo->addItem(tr("Workspace"), TITLE_WORKSPACE);
	combo->addItem(tr("Application name + workspace"), TITLE_APP_WORKSPACE);
	combo->setCurrentIndex(combo->findData(settings->title_type));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(titleChanged(int)));
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Tooltips:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Show all"), 1);
	combo->addItem(tr("Hide in keypad"), 2);
	combo->addItem(tr("Hide all"), 0);
	combo->setCurrentIndex(combo->findData(settings->enable_tooltips));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(tooltipsChanged(int)));
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Style:"), this), r, 0);
	combo = new QComboBox(this);
	QStringList list = QStyleFactory::keys();
	combo->addItem(tr("Default (requires restart)", "Default style"));
	combo->addItems(list);
	if(settings->style.isEmpty()) {
		combo->setCurrentIndex(0);
	} else {
		int i = list.indexOf(settings->style);
		combo->setCurrentIndex(i < 0 ? 0 : i + 1);
	}
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
	l->addWidget(combo, r, 1); r++; styleCombo = combo;
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	BOX(tr("Dark mode"), settings->palette == 1 || (settings->palette == -1 && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark), darkModeToggled(bool));
#else
	BOX(tr("Dark mode"), settings->palette == 1, darkModeToggled(bool));
#endif
	BOX(tr("Disable cursor blinking"), settings->disable_cursor_blinking, disableCursorBlinkingToggled(bool));
	BOX(tr("Colorize result"), settings->colorize_result, colorizeToggled(bool));
	BOX(tr("Format result"), settings->format_result, formatToggled(bool));
	l->setRowStretch(r, 1);
	r = 0;
	l = new QGridLayout(w4);
	l->addWidget(new QLabel(tr("Expression status:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Off"), 0);
	combo->addItem(tr("In history list"), 1);
	combo->addItem(tr("In expression field"), 2);
	combo->addItem(tr("In status bar"), 3);
	if(!settings->display_expression_status) combo->setCurrentIndex(0);
	else if(settings->status_in_history) combo->setCurrentIndex(1);
	else if(settings->status_in_statusbar) combo->setCurrentIndex(3);
	else combo->setCurrentIndex(2);
	statusCombo = combo;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(statusModeChanged(int)));
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Status tooltip delay:")), r, 0);
	statusDelayWidget = new QSpinBox(this);
	statusDelayWidget->setRange(0, 10000);
	statusDelayWidget->setSingleStep(250);
	//: milliseconds
	statusDelayWidget->setSuffix(" " + tr("ms"));
	statusDelayWidget->setValue(settings->expression_status_delay); 
	statusDelayWidget->setEnabled(settings->display_expression_status);
	connect(statusDelayWidget, SIGNAL(valueChanged(int)), this, SLOT(statusDelayChanged(int)));
	l->addWidget(statusDelayWidget, r, 1); r++;
	l->addWidget(new QLabel(tr("Calculate as you type delay:")), r, 0);
	calculateDelayWidget = new QSpinBox(this);
	calculateDelayWidget->setRange(0, 10000);
	calculateDelayWidget->setSingleStep(250);
	//: milliseconds
	calculateDelayWidget->setSuffix(" " + tr("ms"));
	calculateDelayWidget->setValue(settings->auto_calculate_delay);
	calculateDelayWidget->setEnabled(settings->status_in_history);
	connect(calculateDelayWidget, SIGNAL(valueChanged(int)), this, SLOT(calculateDelayChanged(int)));
	l->addWidget(calculateDelayWidget, r, 1); r++;
	BOX(tr("Automatically calculate selection"), settings->autocalc_selection, autocalcSelectionToggled(bool));
	l->addWidget(new QLabel(tr("Expression in history:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Parsed"), 0);
	combo->addItem(tr("Entered"), 1);
	combo->addItem(tr("Entered + parsed"), 2);
	combo->setCurrentIndex(combo->findData(settings->history_expression_type));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(historyExpressionChanged(int)));
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Expression after calculation:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Keep expression"), KEEP_EXPRESSION);
	combo->addItem(tr("Clear expression"), CLEAR_EXPRESSION);
	combo->addItem(tr("Replace with result"), REPLACE_EXPRESSION_WITH_RESULT);
	combo->addItem(tr("Replace with result if shorter"), REPLACE_EXPRESSION_WITH_RESULT_IF_SHORTER);
	combo->setCurrentIndex(combo->findData(settings->replace_expression));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(replaceExpressionChanged(int)));
	l->addWidget(combo, r, 1); r++;
	BOX(tr("Automatically copy result"), settings->autocopy_result, autocopyResultToggled(bool));
	l->addWidget(new QLabel(tr("Parsing mode:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Adaptive"), PARSING_MODE_ADAPTIVE);
	combo->addItem(tr("Conventional"), PARSING_MODE_CONVENTIONAL);
	combo->addItem(tr("Implicit multiplication first"), PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST);
	combo->addItem(tr("Chain"), PARSING_MODE_CHAIN);
	combo->addItem(tr("RPN"), PARSING_MODE_RPN);
	combo->setCurrentIndex(combo->findData(settings->evalops.parse_options.parsing_mode));
	parseCombo = combo;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(parsingModeChanged(int)));
	l->addWidget(combo, r, 1); r++;
	BOX(tr("Simplified percentage calculation"), settings->simplified_percentage, simplifiedPercentageToggled(bool));
	BOX(tr("Read precision"), settings->evalops.parse_options.read_precision != DONT_READ_PRECISION, readPrecisionToggled(bool));
	BOX(tr("Allow concise uncertainty input"), CALCULATOR->conciseUncertaintyInputEnabled(), conciseUncertaintyInputToggled(bool)); conciseUncertaintyInputBox = box;
	BOX(tr("Limit implicit multiplication"), settings->evalops.parse_options.limit_implicit_multiplication, limitImplicitToggled(bool));
	BOX(tr("Interpret unrecognized symbols as variables"), settings->evalops.parse_options.unknowns_enabled, unknownsToggled(bool));
	l->addWidget(new QLabel(tr("Interval calculation:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Variance formula"), INTERVAL_CALCULATION_VARIANCE_FORMULA);
	combo->addItem(tr("Interval arithmetic"), INTERVAL_CALCULATION_INTERVAL_ARITHMETIC);
	combo->setCurrentIndex(combo->findData(settings->evalops.interval_calculation));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(intervalCalculationChanged(int)));
	l->addWidget(combo, r, 1); r++;
	intervalCalculationCombo = combo;
	BOX(tr("Factorize result"), settings->evalops.structuring == STRUCTURING_FACTORIZE, factorizeToggled(bool));
	l->setRowStretch(r, 1);
	l = new QGridLayout(w2);
	r = 0;
	QGridLayout *l2 = new QGridLayout();
	l2->addWidget(new QLabel(tr("Two's complement output:"), this), 0, 0);
	l2->addWidget(new QLabel(tr("Two's complement input:"), this), 1, 0);
	box = new QCheckBox(tr("Binary"), this); l2->addWidget(box, 0, 1); box->setChecked(settings->printops.twos_complement); connect(box, SIGNAL(toggled(bool)), this, SLOT(binTwosToggled(bool)));
	box = new QCheckBox(tr("Hexadecimal"), this); l2->addWidget(box, 0, 2); box->setChecked(settings->printops.hexadecimal_twos_complement); connect(box, SIGNAL(toggled(bool)), this, SLOT(hexTwosToggled(bool)));
	box = new QCheckBox(tr("Binary"), this); l2->addWidget(box, 1, 1); box->setChecked(settings->evalops.parse_options.twos_complement); connect(box, SIGNAL(toggled(bool)), this, SLOT(binTwosInputToggled(bool)));
	box = new QCheckBox(tr("Hexadecimal"), this); l2->addWidget(box, 1, 2); box->setChecked(settings->evalops.parse_options.hexadecimal_twos_complement); connect(box, SIGNAL(toggled(bool)), this, SLOT(hexTwosInputToggled(bool)));
	l2->addWidget(new QLabel(tr("Binary bits:"), this), 2, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Automatic"), 0);
	combo->addItem("8", 8);
	combo->addItem("16", 16);
	combo->addItem("32", 32);
	combo->addItem("64", 64);
	combo->addItem("128", 128);
	combo->addItem("256", 256);
	combo->addItem("512", 512);
	combo->addItem("1024", 1024);
	combo->setCurrentIndex(combo->findData(settings->printops.binary_bits));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(bitsChanged(int)));
	l2->addWidget(combo, 2, 1, 1, 2);
	l->addLayout(l2, r, 0, 1, 2); r++;
	BOX(tr("Use lower case letters in non-decimal numbers"), settings->printops.lower_case_numbers, lowerCaseToggled(bool));
	BOX(tr("Use special duodecimal symbols"), settings->printops.duodecimal_symbols, duodecimalSymbolsToggled(bool));
	l->addWidget(new QLabel(tr("Multiplication sign:"), this), r, 0);
	combo = new QComboBox(this);
	//: Unicode character name
	combo->addItem(QString(" × (%1)").arg(tr("multiplication sign")), MULTIPLICATION_SIGN_X);
	//: Unicode character name
	combo->addItem(QString(" ⋅ (%1)").arg(tr("dot operator")), MULTIPLICATION_SIGN_DOT);
	//: Unicode character name
	combo->addItem(QString(" · (%1)").arg(tr("middle dot")), MULTIPLICATION_SIGN_ALTDOT);
	//: Unicode character name
	combo->addItem(QString(" * (%1)").arg(tr("asterisk")), MULTIPLICATION_SIGN_ASTERISK);
	combo->setCurrentIndex(combo->findData(settings->printops.multiplication_sign));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(multiplicationSignChanged(int)));
	l->addWidget(combo, r, 1); r++;
	BOX(tr("Use Unicode division slash in output"), settings->printops.division_sign == DIVISION_SIGN_DIVISION_SLASH, divisionSlashToggled(bool));
	BOX(tr("Spell out logical operators"), settings->printops.spell_out_logical_operators, spellOutToggled(bool));
	BOX(tr("Use E-notation instead of 10^n"), settings->printops.exp_display != EXP_POWER_OF_10, eToggled(bool));
	BOX(tr("Use 'j' as imaginary unit"), CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0, imaginaryJToggled(bool));
	BOX(tr("Use comma as decimal separator"), CALCULATOR->getDecimalPoint() == COMMA, decimalCommaToggled(bool));
	decimalCommaBox = box;
	BOX(tr("Ignore comma in numbers"), settings->evalops.parse_options.comma_as_separator, ignoreCommaToggled(bool)); ignoreCommaBox = box;
	BOX(tr("Ignore dots in numbers"), settings->evalops.parse_options.dot_as_separator, ignoreDotToggled(bool)); ignoreDotBox = box;
	if(CALCULATOR->getDecimalPoint() == COMMA) ignoreCommaBox->hide();
	if(CALCULATOR->getDecimalPoint() == DOT) ignoreDotBox->hide();
	BOX(tr("Copy unformatted ASCII by default"), settings->copy_ascii, copyAsciiToggled(bool));
	BOX(tr("Place percent button in number pad"), settings->show_percent_in_numpad, showPercentInNumpadToggled(bool));
	l->addWidget(new QLabel(tr("Indicate repeating decimals:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Off"), REPEATING_DECIMALS_OFF);
	combo->addItem(tr("Ellipsis"), REPEATING_DECIMALS_ELLIPSIS);
	combo->addItem(tr("Overline"), REPEATING_DECIMALS_OVERLINE);
	combo->setCurrentIndex(combo->findData(settings->printops.indicate_infinite_series));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(repeatingDecimalsChanged(int)));
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Digit grouping:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("None"), DIGIT_GROUPING_NONE);
	combo->addItem(tr("Standard"), DIGIT_GROUPING_STANDARD);
	combo->addItem(tr("Local"), DIGIT_GROUPING_LOCALE);
	combo->setCurrentIndex(combo->findData(settings->printops.digit_grouping));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(groupingChanged(int)));
	l->addWidget(combo, r, 1); r++;
	BOX(tr("Automatically group digits in input"), settings->automatic_digit_grouping, automaticDigitGroupingToggled(bool));
	l->addWidget(new QLabel(tr("Interval display:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Adaptive"), -1);
	combo->addItem(tr("Significant digits"), INTERVAL_DISPLAY_SIGNIFICANT_DIGITS);
	combo->addItem(tr("Interval"), INTERVAL_DISPLAY_INTERVAL);
	combo->addItem(tr("Plus/minus"), INTERVAL_DISPLAY_PLUSMINUS);
	combo->addItem(tr("Relative"), INTERVAL_DISPLAY_RELATIVE);
	combo->addItem(tr("Concise"), INTERVAL_DISPLAY_CONCISE);
	combo->addItem(tr("Midpoint"), INTERVAL_DISPLAY_MIDPOINT);
	combo->addItem(tr("Lower"), INTERVAL_DISPLAY_LOWER);
	combo->addItem(tr("Upper"), INTERVAL_DISPLAY_UPPER);
	if(settings->adaptive_interval_display) combo->setCurrentIndex(0);
	else combo->setCurrentIndex(combo->findData(settings->printops.interval_display));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(intervalDisplayChanged(int)));
	intervalDisplayCombo = combo;
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Rounding:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Round halfway numbers away from zero"), ROUNDING_HALF_AWAY_FROM_ZERO);
	combo->addItem(tr("Round halfway numbers to even"), ROUNDING_HALF_TO_EVEN);
	combo->addItem(tr("Round halfway numbers to odd"), ROUNDING_HALF_TO_ODD);
	combo->addItem(tr("Round halfway numbers toward zero"), ROUNDING_HALF_TOWARD_ZERO);
	combo->addItem(tr("Round halfway numbers to random"), ROUNDING_HALF_RANDOM);
	combo->addItem(tr("Round halfway numbers up"), ROUNDING_HALF_UP);
	combo->addItem(tr("Round halfway numbers down"), ROUNDING_HALF_DOWN);
	combo->addItem(tr("Round toward zero"), ROUNDING_TOWARD_ZERO);
	combo->addItem(tr("Round away from zero"), ROUNDING_AWAY_FROM_ZERO);
	combo->addItem(tr("Round up"), ROUNDING_UP);
	combo->addItem(tr("Round down"), ROUNDING_DOWN);
	combo->setCurrentIndex(combo->findData(settings->printops.rounding));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(roundingChanged(int)));
	l->addWidget(combo, r, 1); r++;
	l->addWidget(new QLabel(tr("Complex number form:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Rectangular"), COMPLEX_NUMBER_FORM_RECTANGULAR);
	combo->addItem(tr("Exponential"), COMPLEX_NUMBER_FORM_EXPONENTIAL);
	combo->addItem(tr("Polar"), COMPLEX_NUMBER_FORM_POLAR);
	combo->addItem(tr("Angle/phasor"), COMPLEX_NUMBER_FORM_CIS);
	combo->setCurrentIndex(combo->findData(settings->evalops.complex_number_form));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(complexFormChanged(int)));
	complexFormCombo = combo;
	l->addWidget(combo, r, 1); r++;
	l->setRowStretch(r, 1);
	l = new QGridLayout(w3);
	r = 0;
	BOX(tr("Enable units"), settings->evalops.parse_options.units_enabled, unitsToggled(bool));
	BOX(tr("Abbreviate names"), settings->printops.abbreviate_names, abbreviateNamesToggled(bool));
	BOX(tr("Use binary prefixes for information units"), CALCULATOR->usesBinaryPrefixes() > 0, binaryPrefixesToggled(bool));
	l->addWidget(new QLabel(tr("Automatic unit conversion:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("No conversion"), POST_CONVERSION_NONE);
	combo->addItem(tr("Base units"), POST_CONVERSION_BASE);
	combo->addItem(tr("Optimal units"), POST_CONVERSION_OPTIMAL);
	combo->addItem(tr("Optimal SI units"), POST_CONVERSION_OPTIMAL_SI);
	combo->setCurrentIndex(combo->findData(settings->evalops.auto_post_conversion));
	l->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(conversionChanged(int)));
	BOX(tr("Convert to mixed units"), settings->evalops.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE, mixedUnitsToggled(bool));
	l->addWidget(new QLabel(tr("Automatic unit prefixes:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Default"));
	combo->addItem(tr("No prefixes"));
	combo->addItem(tr("Prefixes for some units"));
	combo->addItem(tr("Prefixes also for currencies"));
	combo->addItem(tr("Prefixes for all units"));
	if(settings->prefixes_default) combo->setCurrentIndex(0);
	else if(!settings->printops.use_unit_prefixes) combo->setCurrentIndex(1);
	else if(settings->printops.use_prefixes_for_all_units) combo->setCurrentIndex(4);
	else if(settings->printops.use_prefixes_for_currencies) combo->setCurrentIndex(3);
	else combo->setCurrentIndex(2);
	l->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(prefixesChanged(int)));
	BOX(tr("Enable all SI-prefixes"), settings->printops.use_all_prefixes, allPrefixesToggled(bool));
	BOX(tr("Enable denominator prefixes"), settings->printops.use_denominator_prefix, denominatorPrefixToggled(bool));
	BOX(tr("Enable units in physical constants"), CALCULATOR->variableUnitsEnabled(), variableUnitsToggled(bool)); variableUnitsBox = box;
	BOX(tr("Copy unformatted ASCII without units"), settings->copy_ascii_without_units, copyAsciiWithoutUnitsToggled(bool));
	l->addWidget(new QLabel(tr("Temperature calculation:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Absolute"), TEMPERATURE_CALCULATION_ABSOLUTE);
	combo->addItem(tr("Relative"), TEMPERATURE_CALCULATION_RELATIVE);
	combo->addItem(tr("Hybrid"), TEMPERATURE_CALCULATION_HYBRID);
	combo->setCurrentIndex(combo->findData(CALCULATOR->getTemperatureCalculationMode()));
	tcCombo = combo;
	l->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(temperatureCalculationChanged(int)));
	box = new QCheckBox(tr("Exchange rates updates:"), this); box->setChecked(settings->auto_update_exchange_rates > 0); connect(box, SIGNAL(toggled(bool)), this, SLOT(exratesToggled(bool))); l->addWidget(box, r, 0);
	int days = settings->auto_update_exchange_rates <= 0 ? 7 : settings->auto_update_exchange_rates;
	spin = new QSpinBox(this); spin->setRange(1, 100); spin->setValue(days); spin->setEnabled(settings->auto_update_exchange_rates > 0); connect(spin, SIGNAL(valueChanged(int)), this, SLOT(exratesChanged(int))); l->addWidget(spin, r, 1); exratesSpin = spin; r++;
	QString str = tr("%n day(s)", "", days);
	int index = str.indexOf(QString::number(days));
	if(index == 0) {
		str = str.mid(index + QString::number(days).length());
		if(str == " day(s)") str = (days == 1 ? " day" : " days");
		exratesSpin->setSuffix(str);
	} else {
		exratesSpin->setPrefix(str.right(index));
	}
	l->addWidget(new QLabel(tr("Default currency:"), this), r, 0);
	combo = new QComboBox(this);
	QStringList curs;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->isCurrency() && CALCULATOR->units[i]->referenceName().length() == 3) {
			curs << QString::fromStdString(CALCULATOR->units[i]->title()) + " (" + QString::fromStdString(CALCULATOR->units[i]->referenceName()) + ")";
		}
	}
	curs.sort();
	combo->addItems(curs);
	if(CALCULATOR->getLocalCurrency()) {
		QString scur;
		scur = QString::fromStdString(CALCULATOR->getLocalCurrency()->referenceName());
		for(int i = 0; i < curs.size(); i++) {
			if(curs[i].indexOf(scur) == curs[i].length() - 4) {
				combo->setCurrentIndex(i);
				break;
			}
		}
	}
	combo->setMinimumContentsLength(20);
	combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	l->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentTextChanged(const QString&)), this, SLOT(defaultCurrencyChanged(const QString&)));
	l->setRowStretch(r, 1);
	l = new QGridLayout(w5);
	r = 0;
	BOX1(tr("Custom history font:"), settings->use_custom_result_font, resultFontToggled(bool));
	QPushButton *button = new QPushButton(font_string(settings->custom_result_font), this); l->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(resultFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX1(tr("Custom expression font:"), settings->use_custom_expression_font, expressionFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_expression_font), this); l->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(expressionFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX1(tr("Custom status bar font:"), settings->use_custom_status_font, statusFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_status_font), this); l->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(statusFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX1(tr("Custom keypad font:"), settings->use_custom_keypad_font, keypadFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_keypad_font), this); l->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(keypadFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX1(tr("Custom application font:"), settings->use_custom_app_font, appFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_app_font), this); l->addWidget(button, r, 1); button->setEnabled(box->isChecked()); r++;
	connect(button, SIGNAL(clicked()), this, SLOT(appFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	l->setRowStretch(r, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	QSize size = get_screen_geometry(this).size();
	QSize sh = w1->sizeHint().expandedTo(w2->sizeHint().expandedTo(w3->sizeHint().expandedTo(w4->sizeHint().expandedTo(w5->sizeHint()))));
	if(size.width() >= sh.width() + 100) tabs->setUsesScrollButtons(false);
	bool use_child_hint = size.height() >= sh.height() + 100;
	QScrollArea *s1 = new PreferencesPageScroll(this, w1, use_child_hint);
	QScrollArea *s2 = new PreferencesPageScroll(this, w2, use_child_hint);
	QScrollArea *s3 = new PreferencesPageScroll(this, w3, use_child_hint);
	QScrollArea *s4 = new PreferencesPageScroll(this, w4, use_child_hint);
	QScrollArea *s5 = new PreferencesPageScroll(this, w5, use_child_hint);
	tabs->addTab(s1, tr("Look && Feel"));
	tabs->addTab(s5, tr("Fonts"));
	tabs->addTab(s2, tr("Numbers && Operators"));
	tabs->addTab(s3, tr("Units"));
	tabs->addTab(s4, tr("Calculation"));
}
PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::ignoreLocaleToggled(bool b) {
	settings->ignore_locale = b;
	if(settings->ignore_locale) {
		langCombo->blockSignals(true);
		langCombo->setCurrentIndex(0);
		langCombo->blockSignals(false);
	}
	langCombo->setEnabled(!settings->ignore_locale);
}
void PreferencesDialog::langChanged(int i) {
	switch(i) {
		case 0: {settings->custom_lang = ""; break;}
		case 1: {settings->custom_lang = "ca_ES"; break;}
		case 2: {settings->custom_lang = "de_DE"; break;}
		case 3: {settings->custom_lang = "en_US"; break;}
		case 4: {settings->custom_lang = "es_ES"; break;}
		case 5: {settings->custom_lang = "fr_FR"; break;}
		case 6: {settings->custom_lang = "ka_GE"; break;}
		case 7: {settings->custom_lang = "nl_NL"; break;}
		case 8: {settings->custom_lang = "pt_PT"; break;}
		case 9: {settings->custom_lang = "pt_BR"; break;}
		case 10: {settings->custom_lang = "ru_RU"; break;}
		case 11: {settings->custom_lang = "sl_SL"; break;}
		case 12: {settings->custom_lang = "sv_SE"; break;}
		case 13: {settings->custom_lang = "zh_CN"; break;}
		case 14: {settings->custom_lang = "zh_TW"; break;}
	}
	if(!settings->custom_lang.isEmpty()) {
		ignoreLocaleBox->setChecked(false);
		settings->ignore_locale = false;
	}
	QMessageBox::information(this, tr("Restart required"), tr("Please restart the program for the language change to take effect."), QMessageBox::Close);
}
void PreferencesDialog::multipleInstancesToggled(bool b) {
	settings->allow_multiple_instances = b;
}
void PreferencesDialog::checkVersionToggled(bool b) {
	settings->check_version = b;
}
void PreferencesDialog::clearHistoryToggled(bool b) {
	settings->clear_history_on_exit = b;
}
void PreferencesDialog::maxHistoryLinesChanged(int i) {
	settings->max_history_lines = i;
}
void PreferencesDialog::preserveHeightChanged(int state) {
	preserveHeightBox->setTristate(false);
	settings->preserve_history_height = (state == Qt::Checked);
	settings->keypad_appended = false;
	settings->bases_appended = false;
}
void PreferencesDialog::expressionPositionToggled(bool b) {
	settings->expression_pos = (b ? 1 : 0);
	emit expressionPositionChanged();
}
void PreferencesDialog::showStatusBarToggled(bool b) {
	settings->show_statusbar = b;
	if(!b) {
		settings->status_in_statusbar = false;
		settings->status_in_history = true;
		emit statusModeChanged();
	}
	emit showStatusBarChanged();
}
void PreferencesDialog::keepAboveToggled(bool b) {
	settings->always_on_top = b;
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	else setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	show();
	emit alwaysOnTopChanged();
}
void PreferencesDialog::tooltipsChanged(int i) {
	settings->enable_tooltips = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit enableTooltipsChanged();
}
void PreferencesDialog::statusModeChanged(int i) {
	settings->display_expression_status = (i > 0);
	settings->status_in_history = (i == 1);
	settings->status_in_statusbar = (i == 3);
	statusDelayWidget->setEnabled(settings->display_expression_status);
	calculateDelayWidget->setEnabled(settings->status_in_history);
	emit statusModeChanged();
}
void PreferencesDialog::statusDelayChanged(int v) {
	settings->expression_status_delay = v;
}
void PreferencesDialog::calculateDelayChanged(int v) {
	settings->auto_calculate_delay = v;
}
void PreferencesDialog::autocalcSelectionToggled(bool b) {
	settings->autocalc_selection = b;
}
void PreferencesDialog::binTwosToggled(bool b) {
	settings->printops.twos_complement = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::hexTwosToggled(bool b) {
	settings->printops.hexadecimal_twos_complement = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::binTwosInputToggled(bool b) {
	settings->evalops.parse_options.twos_complement = b;
	if(b != settings->default_signed) settings->default_signed = -1;
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::hexTwosInputToggled(bool b) {
	settings->evalops.parse_options.hexadecimal_twos_complement = b;
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::bitsChanged(int i) {
	settings->printops.binary_bits = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	settings->evalops.parse_options.binary_bits = settings->printops.binary_bits;
	settings->default_bits = -1;
	emit binaryBitsChanged();
	if(settings->evalops.parse_options.hexadecimal_twos_complement || settings->evalops.parse_options.twos_complement) {
		emit expressionFormatUpdated(false);
	} else {
		emit resultDisplayUpdated();
	}
}
void PreferencesDialog::lowerCaseToggled(bool b) {
	settings->printops.lower_case_numbers = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::duodecimalSymbolsToggled(bool b) {
	settings->printops.duodecimal_symbols = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::multiplicationSignChanged(int i) {
	settings->printops.multiplication_sign = (MultiplicationSign) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit resultDisplayUpdated();
	emit symbolsUpdated();
}
void PreferencesDialog::divisionSlashToggled(bool b) {
	if(b) {
		settings->printops.division_sign = DIVISION_SIGN_DIVISION_SLASH;
	} else {
		settings->printops.division_sign = DIVISION_SIGN_SLASH;
	}
	emit resultDisplayUpdated();
	emit symbolsUpdated();
}
void PreferencesDialog::spellOutToggled(bool b) {
	settings->printops.spell_out_logical_operators = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::eToggled(bool b) {
	if(b) {
		settings->printops.exp_display = EXP_LOWERCASE_E;
	} else {
		settings->printops.exp_display = EXP_POWER_OF_10;
	}
	emit resultDisplayUpdated();
}
void PreferencesDialog::imaginaryJToggled(bool b) {
	Variable *v_i = CALCULATOR->getVariableById(VARIABLE_ID_I);
	if(!v_i || (b == (CALCULATOR->v_i->hasName("j") > 0))) return;
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
	emit expressionFormatUpdated(false);
	emit symbolsUpdated();
}
void PreferencesDialog::decimalCommaToggled(bool b) {
	settings->decimal_comma = b;
	if(b) {
		CALCULATOR->useDecimalComma();
		ignoreCommaBox->hide();
		ignoreDotBox->show();
	} else {
		CALCULATOR->useDecimalPoint(settings->evalops.parse_options.comma_as_separator);
		ignoreCommaBox->show();
		ignoreDotBox->hide();
	}
	settings->dot_question_asked = true;
	emit expressionFormatUpdated(false);
	emit resultDisplayUpdated();
	emit symbolsUpdated();
}
void PreferencesDialog::ignoreDotToggled(bool b) {
	settings->evalops.parse_options.dot_as_separator = b;
	settings->dot_question_asked = true;
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::ignoreCommaToggled(bool b) {
	settings->evalops.parse_options.comma_as_separator = b;
	CALCULATOR->useDecimalPoint(settings->evalops.parse_options.comma_as_separator);
	settings->dot_question_asked = true;
	emit expressionFormatUpdated(false);
	emit symbolsUpdated();
}
void PreferencesDialog::colorizeToggled(bool b) {
	settings->colorize_result = b;
	emit historyExpressionTypeChanged();
}
void PreferencesDialog::formatToggled(bool b) {
	settings->format_result = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::parsingModeChanged(int i) {
	settings->evalops.parse_options.parsing_mode = (ParsingMode) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || settings->evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) settings->implicit_question_asked = true;
	rpnKeysBox->setEnabled(settings->evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
	rpnKeysBox->blockSignals(true);
	rpnKeysBox->setChecked(settings->rpn_keys && settings->evalops.parse_options.parsing_mode != PARSING_MODE_RPN);
	rpnKeysBox->blockSignals(false);
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::temperatureCalculationChanged(int i) {
	CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) qobject_cast<QComboBox*>(sender())->itemData(i).toInt());
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::exratesToggled(bool b) {
	exratesSpin->setEnabled(b);
	settings->auto_update_exchange_rates = b ? exratesSpin->value() : 0;
}
void PreferencesDialog::exratesChanged(int i) {
	settings->auto_update_exchange_rates = i;
	QString str = tr("%n day(s)", "", i);
	int index = str.indexOf(QString::number(i));
	if(index == 0) {
		str = str.mid(index + QString::number(i).length());
		if(str == " day(s)") str = (i == 1 ? " day" : " days");
		exratesSpin->setSuffix(str);
	} else {
		exratesSpin->setPrefix(str.right(index));
	}
}
void PreferencesDialog::unitsToggled(bool b) {
	settings->evalops.parse_options.units_enabled = b;
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::binaryPrefixesToggled(bool b) {
	CALCULATOR->useBinaryPrefixes(b ? 1 : 0);
	emit resultFormatUpdated();
}
void PreferencesDialog::abbreviateNamesToggled(bool b) {
	settings->printops.abbreviate_names = b;
	emit resultFormatUpdated();
}
void PreferencesDialog::simplifiedPercentageToggled(bool b) {
	settings->simplified_percentage = b;
	emit expressionFormatUpdated(true);
}
void PreferencesDialog::readPrecisionToggled(bool b) {
	if(b) {
		settings->evalops.parse_options.read_precision = READ_PRECISION_WHEN_DECIMALS;
	} else {
		settings->evalops.parse_options.read_precision = DONT_READ_PRECISION;
	}
	emit expressionFormatUpdated(true);
}
void PreferencesDialog::rpnKeysToggled(bool b) {
	settings->rpn_keys = b;
}
void PreferencesDialog::intervalCalculationChanged(int i) {
	settings->evalops.interval_calculation = (IntervalCalculation) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::updateIntervalCalculation() {
	intervalCalculationCombo->blockSignals(true);
	intervalCalculationCombo->setCurrentIndex(intervalCalculationCombo->findData(settings->evalops.interval_calculation));
	intervalCalculationCombo->blockSignals(false);
}
void PreferencesDialog::complexFormChanged(int i) {
	settings->evalops.complex_number_form = (ComplexNumberForm) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	settings->complex_angle_form = (settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS);
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::updateComplexForm() {
	complexFormCombo->blockSignals(true);
	complexFormCombo->setCurrentIndex(complexFormCombo->findData(settings->evalops.complex_number_form));
	complexFormCombo->blockSignals(false);
}
void PreferencesDialog::roundingChanged(int i) {
	settings->printops.rounding = (RoundingMode) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit resultFormatUpdated();
}
void PreferencesDialog::repeatingDecimalsChanged(int i) {
	settings->printops.indicate_infinite_series = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit resultDisplayUpdated();
}
void PreferencesDialog::copyAsciiToggled(bool b) {
	settings->copy_ascii = b;
}
void PreferencesDialog::showPercentInNumpadToggled(bool b) {
	settings->show_percent_in_numpad = b;
	emit buttonLocationChanged();
}
void PreferencesDialog::copyAsciiWithoutUnitsToggled(bool b) {
	settings->copy_ascii_without_units = b;
}
void PreferencesDialog::caretAsXorToggled(bool b) {
	settings->caret_as_xor = b;
}
void PreferencesDialog::automaticDigitGroupingToggled(bool b) {
	settings->automatic_digit_grouping = b;
	emit automaticDigitGroupingChanged();
}
void PreferencesDialog::closeWithEscToggled(bool b) {
	settings->close_with_esc = b;
}
void PreferencesDialog::disableCursorBlinkingToggled(bool b) {
	settings->disable_cursor_blinking = b;
	if(b) qApp->setCursorFlashTime(0);
	else qApp->setCursorFlashTime(1000);
}
void PreferencesDialog::mixedUnitsToggled(bool b) {
	if(b) {
		settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	} else {
		settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
	}
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::conversionChanged(int i) {
	settings->evalops.auto_post_conversion = (AutoPostConversion) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::prefixesChanged(int i) {
	settings->prefixes_default = (i == 0);
	settings->printops.use_unit_prefixes = (i == 0 && (settings->printops.min_exp == EXP_PRECISION || settings->printops.min_exp == EXP_NONE)) || i >= 2;
	settings->printops.use_prefixes_for_all_units = (i == 4);
	settings->printops.use_prefixes_for_currencies = (i >= 3);
	emit resultFormatUpdated();
}
void PreferencesDialog::allPrefixesToggled(bool b) {
	settings->printops.use_all_prefixes = b;
	emit resultFormatUpdated();
}
void PreferencesDialog::denominatorPrefixToggled(bool b) {
	settings->printops.use_denominator_prefix = b;
	emit resultFormatUpdated();
}
void PreferencesDialog::variableUnitsToggled(bool b) {
	CALCULATOR->setVariableUnitsEnabled(b);
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::updateVariableUnits() {
	variableUnitsBox->blockSignals(true);
	variableUnitsBox->setChecked(CALCULATOR->variableUnitsEnabled());
	variableUnitsBox->blockSignals(false);
}
void PreferencesDialog::groupingChanged(int i) {
	settings->printops.digit_grouping = (DigitGrouping) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	if(settings->printops.digit_grouping == DIGIT_GROUPING_LOCALE && (!settings->evalops.parse_options.comma_as_separator || CALCULATOR->getDecimalPoint() == COMMA) && CALCULATOR->local_digit_group_separator == COMMA) {
		if(CALCULATOR->getDecimalPoint() == COMMA) {
			settings->evalops.parse_options.comma_as_separator = true;
			settings->evalops.parse_options.dot_as_separator = false;
			ignoreCommaBox->blockSignals(true);
			ignoreCommaBox->setChecked(true);
			ignoreCommaBox->blockSignals(false);
			ignoreDotBox->blockSignals(true);
			ignoreDotBox->setChecked(false);
			ignoreDotBox->blockSignals(false);
			decimalCommaBox->toggle();
		} else {
			ignoreCommaBox->toggle();
		}
	} else if(settings->printops.digit_grouping == DIGIT_GROUPING_LOCALE && (!settings->evalops.parse_options.dot_as_separator || CALCULATOR->getDecimalPoint() == DOT) && CALCULATOR->local_digit_group_separator == DOT) {
		if(CALCULATOR->getDecimalPoint() == DOT) {
			settings->evalops.parse_options.dot_as_separator = true;
			settings->evalops.parse_options.comma_as_separator = false;
			ignoreDotBox->blockSignals(true);
			ignoreDotBox->setChecked(true);
			ignoreDotBox->blockSignals(false);
			ignoreCommaBox->blockSignals(true);
			ignoreCommaBox->setChecked(false);
			ignoreCommaBox->blockSignals(false);
			decimalCommaBox->toggle();
		} else {
			ignoreDotBox->toggle();
		}
	} else {
		emit resultFormatUpdated();
	}
}
void PreferencesDialog::intervalDisplayChanged(int i) {
	if(i == 0) {
		settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		settings->adaptive_interval_display = true;
	} else {
		settings->printops.interval_display = (IntervalDisplay) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
		settings->adaptive_interval_display = false;
	}
	emit resultFormatUpdated();
}
void PreferencesDialog::updateIntervalDisplay() {
	intervalDisplayCombo->blockSignals(true);
	if(settings->adaptive_interval_display) intervalDisplayCombo->setCurrentIndex(0);
	else intervalDisplayCombo->setCurrentIndex(intervalDisplayCombo->findData(settings->printops.interval_display));
	intervalDisplayCombo->blockSignals(false);
}
void PreferencesDialog::conciseUncertaintyInputToggled(bool b) {
	CALCULATOR->setConciseUncertaintyInputEnabled(b);
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::updateConciseUncertaintyInput() {
	conciseUncertaintyInputBox->blockSignals(true);
	conciseUncertaintyInputBox->setChecked(CALCULATOR->conciseUncertaintyInputEnabled());
	conciseUncertaintyInputBox->blockSignals(false);
}
void PreferencesDialog::limitImplicitToggled(bool b) {
	settings->evalops.parse_options.limit_implicit_multiplication = b;
	settings->printops.limit_implicit_multiplication = b;
	emit expressionFormatUpdated(true);
}
void PreferencesDialog::unknownsToggled(bool b) {
	settings->evalops.parse_options.unknowns_enabled = b;
	emit expressionFormatUpdated(false);
}
void PreferencesDialog::titleChanged(int i) {
	settings->title_type = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit titleTypeChanged();
}
void PreferencesDialog::resultFontClicked() {
	QFont font; font.fromString(QString::fromStdString(settings->custom_result_font));
	QFontDialog *dialog = new QFontDialog(font, this);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	if(dialog->exec() == QDialog::Accepted) {
		settings->save_custom_result_font = true;
		settings->use_custom_result_font = true;
		settings->custom_result_font = dialog->selectedFont().toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_result_font));
		emit resultFontChanged();
	}
	dialog->deleteLater();
}
void PreferencesDialog::resultFontToggled(bool b) {
	settings->use_custom_result_font = b;
	emit resultFontChanged();
}
void PreferencesDialog::expressionFontClicked() {
	QFont font; font.fromString(QString::fromStdString(settings->custom_expression_font));
	QFontDialog *dialog = new QFontDialog(font, this);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	if(dialog->exec() == QDialog::Accepted) {
		settings->save_custom_expression_font = true;
		settings->use_custom_expression_font = true;
		settings->custom_expression_font = dialog->selectedFont().toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_expression_font));
		emit expressionFontChanged();
	}
	dialog->deleteLater();
}
void PreferencesDialog::expressionFontToggled(bool b) {
	settings->use_custom_expression_font = b;
	emit expressionFontChanged();
}
void PreferencesDialog::statusFontClicked() {
	QFont font; font.fromString(QString::fromStdString(settings->custom_status_font));
	QFontDialog *dialog = new QFontDialog(font, this);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	if(dialog->exec() == QDialog::Accepted) {
		settings->save_custom_status_font = true;
		settings->use_custom_status_font = true;
		settings->custom_status_font = dialog->selectedFont().toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_status_font));
		emit statusFontChanged();
	}
	dialog->deleteLater();
}
void PreferencesDialog::statusFontToggled(bool b) {
	settings->use_custom_status_font = b;
	emit statusFontChanged();
}
void PreferencesDialog::keypadFontClicked() {
	QFont font; font.fromString(QString::fromStdString(settings->custom_keypad_font));
	QFontDialog *dialog = new QFontDialog(font, this);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	if(dialog->exec() == QDialog::Accepted) {
		settings->save_custom_keypad_font = true;
		settings->use_custom_keypad_font = true;
		settings->custom_keypad_font = dialog->selectedFont().toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_keypad_font));
		emit keypadFontChanged();
	}
	dialog->deleteLater();
}
void PreferencesDialog::keypadFontToggled(bool b) {
	settings->use_custom_keypad_font = b;
	emit keypadFontChanged();
}
void PreferencesDialog::appFontClicked() {
	QFont font; font.fromString(QString::fromStdString(settings->custom_app_font));
	QFontDialog *dialog = new QFontDialog(font, this);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	if(dialog->exec() == QDialog::Accepted) {
		settings->save_custom_app_font = true;
		settings->use_custom_app_font = true;
		settings->custom_app_font = dialog->selectedFont().toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_app_font));
		emit appFontChanged();
	}
	dialog->deleteLater();
}
void PreferencesDialog::appFontToggled(bool b) {
	settings->use_custom_app_font = b;
	emit appFontChanged();
}
void PreferencesDialog::darkModeToggled(bool b) {
	if(b) settings->palette = 1;
	else settings->palette = -1;
#ifdef _WIN32
	if(b) {
		if(styleCombo->currentText().compare("windowsvista", Qt::CaseInsensitive) == 0) {
			for(int i = 1; i < styleCombo->count(); i++) {
				if(styleCombo->itemText(i).compare("Fusion", Qt::CaseInsensitive) == 0) {
					styleCombo->setCurrentIndex(i);
					break;
				}
			}
		}
	}
#endif
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	settings->updatePalette(QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark);
#else
	settings->updatePalette();
#endif
}
void PreferencesDialog::styleChanged(int i) {
	settings->style = (i == 0 ? "" : styleCombo->currentText());
	settings->updateStyle();
}
void PreferencesDialog::factorizeToggled(bool b) {
	if(b) {
		settings->evalops.structuring = STRUCTURING_FACTORIZE;
	} else {
		settings->evalops.structuring = STRUCTURING_EXPAND;
	}
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::closeEvent(QCloseEvent *e) {
	QDialog::closeEvent(e);
	emit dialogClosed();
}
void PreferencesDialog::replaceExpressionChanged(int i) {
	settings->replace_expression = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
}
void PreferencesDialog::autocopyResultToggled(bool b) {
	settings->autocopy_result = b;
}
void PreferencesDialog::historyExpressionChanged(int i) {
	settings->history_expression_type = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit historyExpressionTypeChanged();
}
void PreferencesDialog::defaultCurrencyChanged(const QString &str) {
	settings->default_currency = str.mid(str.length() - 4, 3).toStdString();
	Unit *u = CALCULATOR->getActiveUnit(settings->default_currency);
	if(u) CALCULATOR->setLocalCurrency(u);
}

void PreferencesDialog::updateDot() {
	decimalCommaBox->blockSignals(true);
	ignoreCommaBox->blockSignals(true);
	ignoreDotBox->blockSignals(true);
	decimalCommaBox->setChecked(CALCULATOR->getDecimalPoint() == COMMA);
	ignoreCommaBox->setChecked(settings->evalops.parse_options.comma_as_separator);
	ignoreDotBox->setChecked(settings->evalops.parse_options.dot_as_separator);
	decimalCommaBox->blockSignals(false);
	ignoreCommaBox->blockSignals(false);
	ignoreDotBox->blockSignals(false);
	if(CALCULATOR->getDecimalPoint() == COMMA) ignoreCommaBox->hide();
	if(CALCULATOR->getDecimalPoint() == DOT) ignoreDotBox->hide();
}
void PreferencesDialog::updateParsingMode() {
	parseCombo->blockSignals(true);
	parseCombo->setCurrentIndex(parseCombo->findData(settings->evalops.parse_options.parsing_mode));
	parseCombo->blockSignals(false);
}
void PreferencesDialog::updateExpressionStatus() {
	statusDelayWidget->blockSignals(true);
	calculateDelayWidget->blockSignals(true);
	statusCombo->blockSignals(true);
	if(!settings->display_expression_status) statusCombo->setCurrentIndex(0);
	else if(settings->status_in_history) statusCombo->setCurrentIndex(1);
	else if(settings->status_in_statusbar) statusCombo->setCurrentIndex(3);
	else statusCombo->setCurrentIndex(2);
	statusDelayWidget->setValue(settings->expression_status_delay);
	statusDelayWidget->setEnabled(settings->display_expression_status);
	calculateDelayWidget->setValue(settings->auto_calculate_delay);
	calculateDelayWidget->setEnabled(settings->status_in_history);
	statusDelayWidget->blockSignals(false);
	calculateDelayWidget->blockSignals(false);
	statusCombo->blockSignals(false);
}
void PreferencesDialog::updateTemperatureCalculation() {
	tcCombo->blockSignals(true);
	tcCombo->setCurrentIndex(tcCombo->findData(CALCULATOR->getTemperatureCalculationMode()));
	tcCombo->blockSignals(false);
}
