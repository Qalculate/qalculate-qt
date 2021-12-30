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
#include <QFontDialog>
#include <QStyleFactory>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "preferencesdialog.h"

#define BOX(t, x, f) box = new QCheckBox(t, this); l->addWidget(box); box->setChecked(x); connect(box, SIGNAL(toggled(bool)), this, SLOT(f));
#define BOX_G(t, x, f) box = new QCheckBox(t, this); l2->addWidget(box, r, 0, 1, 2); box->setChecked(x); connect(box, SIGNAL(toggled(bool)), this, SLOT(f)); r++;
#define BOX_G1(t, x, f) box = new QCheckBox(t, this); l2->addWidget(box, r, 0); box->setChecked(x); connect(box, SIGNAL(toggled(bool)), this, SLOT(f));

QString font_string(std::string str) {
	size_t i = str.find(",");
	if(i != std::string::npos && i < str.length() - 1) {
		if(str[i + 1] != ' ') str.insert(i + 1, 1, ' ');
		i = str.find(",", i + 1);
		if(i != std::string::npos) str = str.substr(0, i);
	}
	return QString::fromStdString(str);
}

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle(tr("Preferences"));
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QTabWidget *tabs = new QTabWidget(this);
	tabs->setUsesScrollButtons(false);
	topbox->addWidget(tabs);
	QWidget *w1 = new QWidget(this);
	QWidget *w2 = new QWidget(this);
	QWidget *w3 = new QWidget(this);
	QWidget *w4 = new QWidget(this);
	tabs->addTab(w1, tr("Look && Feel"));
	tabs->addTab(w2, tr("Numbers && Operators"));
	tabs->addTab(w3, tr("Units && Currencies"));
	tabs->addTab(w4, tr("Parsing && Calculation"));
	QCheckBox *box; QVBoxLayout *l; QGridLayout *l2; QComboBox *combo;
	int r = 0;
	l2 = new QGridLayout(w1); l2->setSizeConstraint(QLayout::SetFixedSize);
	BOX_G(tr("Ignore system language (requires restart)"), settings->ignore_locale, ignoreLocaleToggled(bool));
	BOX_G(tr("Allow multiple instances"), settings->allow_multiple_instances > 0, multipleInstancesToggled(bool));
	BOX_G(tr("Clear history on exit"), settings->clear_history_on_exit, clearHistoryToggled(bool));
	BOX_G(tr("Keep above other windows"), settings->always_on_top, keepAboveToggled(bool));
	l2->addWidget(new QLabel(tr("Window title:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Application name"), TITLE_APP);
	combo->addItem(tr("Result"), TITLE_RESULT);
	combo->addItem(tr("Application name + result"), TITLE_APP_RESULT);
	combo->setCurrentIndex(combo->findData(settings->title_type));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(titleChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	l2->addWidget(new QLabel(tr("Style:"), this), r, 0);
	combo = new QComboBox(this);
	QStringList list = QStyleFactory::keys();
	combo->addItem(tr("Default (requires restart)", "Default style"));
	combo->addItems(list);
	combo->setCurrentIndex(settings->style < 0 ? 0 : settings->style + 1);
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));
	l2->addWidget(combo, r, 1); r++; styleCombo = combo;
	BOX_G(tr("Dark mode"), settings->palette == 1, darkModeToggled(bool));
	BOX_G(tr("Colorize result"), settings->colorize_result, colorizeToggled(bool));
	BOX_G1(tr("Custom result font:"), settings->use_custom_result_font, resultFontToggled(bool)); 
	QPushButton *button = new QPushButton(font_string(settings->custom_result_font), this); l2->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(resultFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX_G1(tr("Custom expression font:"), settings->use_custom_expression_font, expressionFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_expression_font), this); l2->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(expressionFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX_G1(tr("Custom keypad font:"), settings->use_custom_keypad_font, keypadFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_keypad_font), this); l2->addWidget(button, r, 1); button->setEnabled(box->isChecked());; r++;
	connect(button, SIGNAL(clicked()), this, SLOT(keypadFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	BOX_G1(tr("Custom application font:"), settings->use_custom_app_font, appFontToggled(bool));
	button = new QPushButton(font_string(settings->custom_app_font), this); l2->addWidget(button, r, 1); button->setEnabled(box->isChecked()); r++;
	connect(button, SIGNAL(clicked()), this, SLOT(appFontClicked())); connect(box, SIGNAL(toggled(bool)), button, SLOT(setEnabled(bool)));
	l2->setRowStretch(r, 1);
	r = 0;
	l2 = new QGridLayout(w4); l2->setSizeConstraint(QLayout::SetFixedSize);
	box = new QCheckBox(tr("Display expression status"), this); l2->addWidget(box, r, 0); box->setChecked(settings->display_expression_status); connect(box, SIGNAL(toggled(bool)), this, SLOT(expressionStatusToggled(bool)));
	statusBox = box;
	QHBoxLayout *hbox = new QHBoxLayout();
	l2->addLayout(hbox, r, 1); r++;
	hbox->addWidget(new QLabel(tr("Delay:")));
	statusDelayWidget = new QSpinBox(this);
	statusDelayWidget->setRange(0, 10000);
	statusDelayWidget->setSingleStep(250);
	//: milliseconds
	statusDelayWidget->setSuffix(" " + tr("ms"));
	statusDelayWidget->setValue(settings->expression_status_delay); 
	connect(statusDelayWidget, SIGNAL(valueChanged(int)), this, SLOT(statusDelayChanged(int)));
	hbox->addWidget(statusDelayWidget);
	hbox->addStretch(1);
	l2->addWidget(new QLabel(tr("Expression after calculation:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Keep expression"), KEEP_EXPRESSION);
	combo->addItem(tr("Clear expression"), CLEAR_EXPRESSION);
	combo->addItem(tr("Replace with result"), REPLACE_EXPRESSION_WITH_RESULT);
	combo->addItem(tr("Replace with result if shorter"), REPLACE_EXPRESSION_WITH_RESULT_IF_SHORTER);
	combo->setCurrentIndex(combo->findData(settings->replace_expression));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(replaceExpressionChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	BOX_G(tr("Use keyboard keys for RPN"), settings->rpn_keys, rpnKeysToggled(bool));
	l2->addWidget(new QLabel(tr("Parsing mode:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Adaptive"), PARSING_MODE_ADAPTIVE);
	combo->addItem(tr("Conventional"), PARSING_MODE_CONVENTIONAL);
	combo->addItem(tr("Implicit multiplication first"), PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST);
	combo->addItem(tr("Chain"), PARSING_MODE_CHAIN);
	combo->addItem(tr("RPN"), PARSING_MODE_RPN);
	combo->setCurrentIndex(combo->findData(settings->evalops.parse_options.parsing_mode));
	parseCombo = combo;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(parsingModeChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	BOX_G(tr("Read precision"), settings->evalops.parse_options.read_precision != DONT_READ_PRECISION, readPrecisionToggled(bool));
	BOX_G(tr("Limit implicit multiplication"), settings->evalops.parse_options.limit_implicit_multiplication, limitImplicitToggled(bool));
	l2->addWidget(new QLabel(tr("Interval calculation:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Variance formula"), INTERVAL_CALCULATION_VARIANCE_FORMULA);
	combo->addItem(tr("Interval arithmetic"), INTERVAL_CALCULATION_INTERVAL_ARITHMETIC);
	combo->setCurrentIndex(combo->findData(settings->evalops.interval_calculation));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(intervalCalculationChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	BOX_G(tr("Factorize result"), settings->evalops.structuring == STRUCTURING_FACTORIZE, factorizeToggled(bool));
	l2->setRowStretch(r, 1);
	l = new QVBoxLayout(w2); l->setSizeConstraint(QLayout::SetFixedSize);
	BOX(tr("Binary two's complement representation"), settings->printops.twos_complement, binTwosToggled(bool));
	BOX(tr("Hexadecimal two's complement representation"), settings->printops.hexadecimal_twos_complement, hexTwosToggled(bool));
	BOX(tr("Use lower case letters in non-decimal numbers"), settings->printops.lower_case_numbers, lowerCaseToggled(bool));
	BOX(tr("Use dot as multiplication sign"), settings->printops.multiplication_sign != MULTIPLICATION_SIGN_X, multiplicationDotToggled(bool));
	BOX(tr("Use Unicode division slash in output"), settings->printops.division_sign == DIVISION_SIGN_DIVISION_SLASH, divisionSlashToggled(bool));
	BOX(tr("Spell out logical operators"), settings->printops.spell_out_logical_operators, spellOutToggled(bool));
	BOX(tr("Use E-notation instead of 10^n"), settings->printops.lower_case_e, eToggled(bool));
	BOX(tr("Use 'j' as imaginary unit"), CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0, imaginaryJToggled(bool));
	BOX(tr("Use comma as decimal separator"), CALCULATOR->getDecimalPoint() == COMMA, decimalCommaToggled(bool));
	decimalCommaBox = box;
	BOX(tr("Ignore comma in numbers"), settings->evalops.parse_options.comma_as_separator, ignoreCommaToggled(bool)); ignoreCommaBox = box;
	BOX(tr("Ignore dots in numbers"), settings->evalops.parse_options.dot_as_separator, ignoreDotToggled(bool)); ignoreDotBox = box;
	BOX(tr("Indicate repeating decimals"), settings->printops.indicate_infinite_series, repeatingDecimalsToggled(bool));
	if(CALCULATOR->getDecimalPoint() == COMMA) ignoreCommaBox->hide();
	if(CALCULATOR->getDecimalPoint() == DOT) ignoreDotBox->hide();
	l2 = new QGridLayout();
	r = 0;
	l2->addWidget(new QLabel(tr("Digit grouping:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("None"), DIGIT_GROUPING_NONE);
	combo->addItem(tr("Standard"), DIGIT_GROUPING_STANDARD);
	combo->addItem(tr("Local"), DIGIT_GROUPING_LOCALE);
	combo->setCurrentIndex(combo->findData(settings->printops.digit_grouping));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(groupingChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	l2->addWidget(new QLabel(tr("Interval display:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Adaptive"), -1);
	combo->addItem(tr("Significant digits"), INTERVAL_DISPLAY_SIGNIFICANT_DIGITS);
	combo->addItem(tr("Interval"), INTERVAL_DISPLAY_INTERVAL);
	combo->addItem(tr("Plus/minus"), INTERVAL_DISPLAY_PLUSMINUS);
	combo->addItem(tr("Midpoint"), INTERVAL_DISPLAY_MIDPOINT);
	combo->addItem(tr("Lower"), INTERVAL_DISPLAY_LOWER);
	combo->addItem(tr("Upper"), INTERVAL_DISPLAY_UPPER);
	if(settings->adaptive_interval_display) combo->setCurrentIndex(0);
	else combo->setCurrentIndex(combo->findData(settings->printops.interval_display));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(intervalDisplayChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	l2->addWidget(new QLabel(tr("Rounding:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Round halfway upwards"), 0);
	combo->addItem(tr("Round halfway to even"), 1);
	combo->addItem(tr("Truncate"), 2);
	combo->setCurrentIndex(combo->findData(settings->rounding_mode));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(roundingChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	l2->addWidget(new QLabel(tr("Complex number form:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Rectangular"), COMPLEX_NUMBER_FORM_RECTANGULAR);
	combo->addItem(tr("Exponential"), COMPLEX_NUMBER_FORM_EXPONENTIAL);
	combo->addItem(tr("Polar"), COMPLEX_NUMBER_FORM_POLAR);
	combo->addItem(tr("Angle/phasor"), COMPLEX_NUMBER_FORM_CIS);
	combo->setCurrentIndex(combo->findData(settings->evalops.complex_number_form));
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(complexFormChanged(int)));
	l2->addWidget(combo, r, 1); r++;
	l->addLayout(l2);
	l->addStretch(1);
	l2 = new QGridLayout(w3); l2->setSizeConstraint(QLayout::SetFixedSize);
	r = 0;
	BOX_G(tr("Abbreviate names"), settings->printops.abbreviate_names, abbreviateNamesToggled(bool));
	BOX_G(tr("Use binary prefixes for information units"), CALCULATOR->usesBinaryPrefixes() > 0, binaryPrefixesToggled(bool));
	l2->addWidget(new QLabel(tr("Automatic unit conversion:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("No conversion"), POST_CONVERSION_NONE);
	combo->addItem(tr("Base units"), POST_CONVERSION_BASE);
	combo->addItem(tr("Optimal units"), POST_CONVERSION_OPTIMAL);
	combo->addItem(tr("Optimal SI units"), POST_CONVERSION_OPTIMAL_SI);
	combo->setCurrentIndex(combo->findData(settings->evalops.auto_post_conversion));
	l2->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(conversionChanged(int)));
	BOX_G(tr("Convert to mixed units"), settings->evalops.mixed_units_conversion != MIXED_UNITS_CONVERSION_NONE, mixedUnitsToggled(bool));
	l2->addWidget(new QLabel(tr("Automatic unit prefixes:"), this), r, 0);
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
	l2->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(prefixesChanged(int)));
	BOX_G(tr("Enable all SI-prefixes"), settings->printops.use_all_prefixes, allPrefixesToggled(bool));
	BOX_G(tr("Enable denominator prefixes"), settings->printops.use_denominator_prefix, denominatorPrefixToggled(bool));
	BOX_G(tr("Enable units in physical constants"), CALCULATOR->variableUnitsEnabled(), variableUnitsToggled(bool));
	l2->addWidget(new QLabel(tr("Temperature calculation:"), this), r, 0);
	combo = new QComboBox(this);
	combo->addItem(tr("Absolute"), TEMPERATURE_CALCULATION_ABSOLUTE);
	combo->addItem(tr("Relative"), TEMPERATURE_CALCULATION_RELATIVE);
	combo->addItem(tr("Hybrid"), TEMPERATURE_CALCULATION_HYBRID);
	combo->setCurrentIndex(combo->findData(CALCULATOR->getTemperatureCalculationMode()));
	tcCombo = combo;
	l2->addWidget(combo, r, 1); r++;
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(temperatureCalculationChanged(int)));
	box = new QCheckBox(tr("Exchange rates updates:"), this); box->setChecked(settings->auto_update_exchange_rates > 0); connect(box, SIGNAL(toggled(bool)), this, SLOT(exratesToggled(bool))); l2->addWidget(box, r, 0);
	int days = settings->auto_update_exchange_rates <= 0 ? 7 : settings->auto_update_exchange_rates;
	QSpinBox *spin = new QSpinBox(this); spin->setRange(1, 100); spin->setValue(days); spin->setEnabled(settings->auto_update_exchange_rates > 0); connect(spin, SIGNAL(valueChanged(int)), this, SLOT(exratesChanged(int))); l2->addWidget(spin, r, 1); exratesSpin = spin; r++;
	QString str = tr("%n day(s)", "", days);
	int index = str.indexOf(QString::number(days));
	if(index == 0) {
		str = str.mid(index + QString::number(days).length());
		if(str == " day(s)") str = (days == 1 ? " day" : " days");
		exratesSpin->setSuffix(str);
	} else {
		exratesSpin->setPrefix(str.right(index));
	}
	l2->setRowStretch(r, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::ignoreLocaleToggled(bool b) {
	settings->ignore_locale = b;
}
void PreferencesDialog::multipleInstancesToggled(bool b) {
	settings->allow_multiple_instances = b;
}
void PreferencesDialog::clearHistoryToggled(bool b) {
	settings->clear_history_on_exit = b;
}
void PreferencesDialog::keepAboveToggled(bool b) {
	settings->always_on_top = b;
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	else setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	show();
	emit alwaysOnTopChanged();
}
void PreferencesDialog::expressionStatusToggled(bool b) {
	settings->display_expression_status = b;
	statusDelayWidget->setEnabled(b);
	if(!b) QToolTip::hideText();
}
void PreferencesDialog::statusDelayChanged(int v) {
	settings->expression_status_delay = v;
}
void PreferencesDialog::binTwosToggled(bool b) {
	settings->printops.twos_complement = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::hexTwosToggled(bool b) {
	settings->printops.hexadecimal_twos_complement = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::lowerCaseToggled(bool b) {
	settings->printops.lower_case_numbers = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::multiplicationDotToggled(bool b) {
	if(b) settings->printops.multiplication_sign = MULTIPLICATION_SIGN_ALTDOT;
	else settings->printops.multiplication_sign = MULTIPLICATION_SIGN_X;
	emit resultDisplayUpdated();
	emit symbolsUpdated();
}
void PreferencesDialog::divisionSlashToggled(bool b) {
	if(b) settings->printops.division_sign = DIVISION_SIGN_DIVISION_SLASH;
	else settings->printops.division_sign = DIVISION_SIGN_SLASH;
	emit resultDisplayUpdated();
	emit symbolsUpdated();
}
void PreferencesDialog::spellOutToggled(bool b) {
	settings->printops.spell_out_logical_operators = b;
	emit resultDisplayUpdated();
}
void PreferencesDialog::eToggled(bool b) {
	settings->printops.lower_case_e = b;
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
	emit resultDisplayUpdated();
}
void PreferencesDialog::parsingModeChanged(int i) {
	settings->evalops.parse_options.parsing_mode = (ParsingMode) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	settings->implicit_question_asked = (settings->evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || settings->evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST);
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
void PreferencesDialog::binaryPrefixesToggled(bool b) {
	CALCULATOR->useBinaryPrefixes(b ? 1 : 0);
	emit resultFormatUpdated();
}
void PreferencesDialog::abbreviateNamesToggled(bool b) {
	settings->printops.abbreviate_names = b;
	emit resultFormatUpdated();
}
void PreferencesDialog::readPrecisionToggled(bool b) {
	if(b) settings->evalops.parse_options.read_precision = READ_PRECISION_WHEN_DECIMALS;
	else settings->evalops.parse_options.read_precision = DONT_READ_PRECISION;
	emit expressionFormatUpdated(true);
}
void PreferencesDialog::rpnKeysToggled(bool b) {
	settings->rpn_keys = b;
}
void PreferencesDialog::intervalCalculationChanged(int i) {
	settings->evalops.interval_calculation = (IntervalCalculation) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::complexFormChanged(int i) {
	settings->evalops.complex_number_form = (ComplexNumberForm) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	settings->complex_angle_form = (settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS);
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::roundingChanged(int i) {
	settings->rounding_mode = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	settings->printops.custom_time_zone = (settings->rounding_mode == 2 ? -21586 : 0);
	settings->printops.round_halfway_to_even = (settings->rounding_mode == 1);
	emit resultFormatUpdated();
}
void PreferencesDialog::repeatingDecimalsToggled(bool b) {
	settings->printops.indicate_infinite_series = b;
	emit resultFormatUpdated();
}
void PreferencesDialog::mixedUnitsToggled(bool b) {
	if(b) settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_DEFAULT;
	else settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
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
void PreferencesDialog::groupingChanged(int i) {
	settings->printops.digit_grouping = (DigitGrouping) qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit resultFormatUpdated();
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
void PreferencesDialog::limitImplicitToggled(bool b) {
	settings->evalops.parse_options.limit_implicit_multiplication = b;
	settings->printops.limit_implicit_multiplication = b;
	emit expressionFormatUpdated(true);
}
void PreferencesDialog::titleChanged(int i) {
	settings->title_type = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
	emit titleTypeChanged();
}
void PreferencesDialog::resultFontClicked() {
	bool ok = true;
	QFont font; font.fromString(QString::fromStdString(settings->custom_result_font));
	font = QFontDialog::getFont(&ok, font);
	if(ok) {
		settings->save_custom_result_font = true;
		settings->use_custom_result_font = true;
		settings->custom_result_font = font.toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_result_font));
		emit resultFontChanged();
	}
}
void PreferencesDialog::resultFontToggled(bool b) {
	settings->use_custom_result_font = b;
	emit resultFontChanged();
}
void PreferencesDialog::expressionFontClicked() {
	bool ok = true;
	QFont font; font.fromString(QString::fromStdString(settings->custom_expression_font));
	font = QFontDialog::getFont(&ok, font);
	if(ok) {
		settings->save_custom_expression_font = true;
		settings->use_custom_expression_font = true;
		settings->custom_expression_font = font.toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_expression_font));
		emit expressionFontChanged();
	}
}
void PreferencesDialog::expressionFontToggled(bool b) {
	settings->use_custom_expression_font = b;
	emit expressionFontChanged();
}
void PreferencesDialog::keypadFontClicked() {
	bool ok = true;
	QFont font; font.fromString(QString::fromStdString(settings->custom_keypad_font));
	font = QFontDialog::getFont(&ok, font);
	if(ok) {
		settings->save_custom_keypad_font = true;
		settings->use_custom_keypad_font = true;
		settings->custom_keypad_font = font.toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_keypad_font));
		emit keypadFontChanged();
	}
}
void PreferencesDialog::keypadFontToggled(bool b) {
	settings->use_custom_keypad_font = b;
	emit keypadFontChanged();
}
void PreferencesDialog::appFontClicked() {
	bool ok = true;
	QFont font; font.fromString(QString::fromStdString(settings->custom_app_font));
	font = QFontDialog::getFont(&ok, font);
	if(ok) {
		settings->save_custom_app_font = true;
		settings->use_custom_app_font = true;
		settings->custom_app_font = font.toString().toStdString();
		qobject_cast<QPushButton*>(sender())->setText(font_string(settings->custom_app_font));
		emit appFontChanged();
	}
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
		if(styleCombo->currentIndex() == 0 || styleCombo->currentText().compare("windowsvista", Qt::CaseInsensitive) == 0) {
			for(int i = 1; i < styleCombo->count(); i++) {
				if(styleCombo->itemText(i).compare("Fusion", Qt::CaseInsensitive) == 0) {
					int prev_style = settings->style;
					styleCombo->setCurrentIndex(i);
					settings->light_style = prev_style;
					break;
				}
			}
		}
	} else if(light_style != settings->style && styleCombo->currentText().compare("Fusion", Qt::CaseInsensitive) == 0) {
		styleCombo->setCurrentIndex(settings->light_style < 0 ? 0 : settings->light_style + 1);
	}
#endif
	settings->updatePalette();
}
void PreferencesDialog::styleChanged(int i) {
	settings->style = i - 1;
	settings->light_style = settings->style;
	settings->updateStyle();
}
void PreferencesDialog::factorizeToggled(bool b) {
	if(b) settings->evalops.structuring = STRUCTURING_FACTORIZE;
	else settings->evalops.structuring = STRUCTURING_EXPAND;
	emit expressionCalculationUpdated(0);
}
void PreferencesDialog::closeEvent(QCloseEvent *e) {
	QDialog::closeEvent(e);
	emit dialogClosed();
}
void PreferencesDialog::replaceExpressionChanged(int i) {
	settings->replace_expression = qobject_cast<QComboBox*>(sender())->itemData(i).toInt();
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
	statusBox->blockSignals(true);
	statusDelayWidget->setValue(settings->expression_status_delay);
	statusBox->setChecked(settings->display_expression_status);
	statusDelayWidget->setEnabled(settings->display_expression_status);
	statusDelayWidget->blockSignals(false);
	statusBox->blockSignals(false);
}
void PreferencesDialog::updateTemperatureCalculation() {
	tcCombo->blockSignals(true);
	tcCombo->setCurrentIndex(tcCombo->findData(CALCULATOR->getTemperatureCalculationMode()));
	tcCombo->blockSignals(false);
}
