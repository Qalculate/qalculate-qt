/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFontMetrics>
#include <QLineEdit>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "fpconversiondialog.h"

FPConversionDialog::FPConversionDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle(tr("Floating Point Conversion (IEEE 754)"));
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Format"), this), 0, 0, Qt::AlignRight);
	formatCombo = new QComboBox(this); grid->addWidget(formatCombo, 0, 1);
	formatCombo->addItem(tr("16-bit (half precision)"));
	formatCombo->addItem(tr("32-bit (single precision)"));
	formatCombo->addItem(tr("64-bit (double precision)"));
	formatCombo->addItem(tr("80-bit (x86 extended format)"));
	formatCombo->addItem(tr("128-bit (quadruple precision)"));
	formatCombo->addItem(tr("Microchip 24-bit"));
	formatCombo->addItem(tr("Microchip 32-bit"));
	formatCombo->setCurrentIndex(1);
	grid->addWidget(new QLabel(tr("Value"), this), 1, 0, Qt::AlignRight);
	valueEdit = new MathLineEdit(this); valueEdit->setAlignment(Qt::AlignRight); grid->addWidget(valueEdit, 1, 1);
	QLabel *label = new QLabel(tr("Binary representation"), this);
	label->setMinimumHeight(valueEdit->sizeHint().height());
	grid->addWidget(label, 2, 0, Qt::AlignRight | Qt::AlignTop);
	binEdit = new QTextEdit(this); grid->addWidget(binEdit, 2, 1);
	QFontMetrics fm(binEdit->font());
	QString str; str.fill('0', 68);
	binEdit->setMinimumWidth(fm.boundingRect(str).width());
	binEdit->setFixedHeight(fm.lineSpacing() * 3 + (binEdit->document()->documentMargin() + binEdit->frameWidth()) * 2 + binEdit->contentsMargins().top() + binEdit->contentsMargins().bottom());
	binEdit->setPlainText("0");
	QTextCursor cursor = binEdit->textCursor();
	QTextBlockFormat textBlockFormat = cursor.blockFormat();
	textBlockFormat.setAlignment(Qt::AlignRight);
	cursor.mergeBlockFormat(textBlockFormat);
	binEdit->setTextCursor(cursor);
	binEdit->setPlainText("");
	grid->addWidget(new QLabel(tr("Hexadecimal representation"), this), 3, 0, Qt::AlignRight);
	hexEdit = new QLineEdit(this); hexEdit->setAlignment(Qt::AlignRight); grid->addWidget(hexEdit, 3, 1);
	grid->addWidget(new QLabel(tr("Floating point value"), this), 4, 0, Qt::AlignRight);
	hexExp2Edit = new QLineEdit(this); hexExp2Edit->setAlignment(Qt::AlignRight); grid->addWidget(hexExp2Edit, 4, 1); hexExp2Edit->setReadOnly(true);
	exp2Edit = new QLineEdit(this); exp2Edit->setAlignment(Qt::AlignRight); grid->addWidget(exp2Edit, 5, 1); exp2Edit->setReadOnly(true);
	decEdit = new QLineEdit(this); decEdit->setAlignment(Qt::AlignRight); grid->addWidget(decEdit, 6, 1); decEdit->setReadOnly(true);
	grid->addWidget(new QLabel(tr("Conversion error"), this), 7, 0, Qt::AlignRight);
	errorEdit = new QLineEdit(this); errorEdit->setAlignment(Qt::AlignRight); grid->addWidget(errorEdit, 7, 1); errorEdit->setReadOnly(true);
	valueEdit->setFocus();
	grid->setColumnStretch(1, 1);
	box->addLayout(grid);
	box->addStretch(1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(formatCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged()));
	connect(binEdit, SIGNAL(textChanged()), this, SLOT(binChanged()));
	connect(valueEdit, SIGNAL(textEdited(const QString&)), this, SLOT(valueChanged()));
	connect(hexEdit, SIGNAL(textEdited(const QString&)), this, SLOT(hexChanged()));
	box->setSizeConstraint(QLayout::SetFixedSize);
}
FPConversionDialog::~FPConversionDialog() {}
unsigned int FPConversionDialog::getBits() {
	switch(formatCombo->currentIndex()) {
		case 0: return 16;
		case 1: return 32;
		case 2: return 64;
		case 3: return 80;
		case 4: return 128;
		case 5: return 24;
		case 6: return 32;
	}
	return 32;
}
unsigned int FPConversionDialog::getExponentBits() {
	int i = formatCombo->currentIndex();
	if(i == 5) return 8;
	return standard_expbits(getBits());
}
unsigned int FPConversionDialog::getSignPosition() {
	int i = formatCombo->currentIndex();
	if(i == 5 || i == 6) return 8;
	return 0;
}
void FPConversionDialog::formatChanged() {
	updateFields(10);
}
void FPConversionDialog::clear() {
	updateFields(0);
}
void FPConversionDialog::updateFields(int base, const MathStructure *v) {
	std::string sbin;
	Number decnum;
	bool use_decnum = false;
	unsigned int bits = getBits();
	unsigned int expbits = getExponentBits();
	unsigned int sgnpos = getSignPosition();
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	po.use_unicode_signs = settings->printops.use_unicode_signs;
	po.exp_display = settings->printops.exp_display;
	po.lower_case_numbers = settings->printops.lower_case_numbers;
	po.rounding = settings->printops.rounding;
	po.base_display = BASE_DISPLAY_NONE;
	po.abbreviate_names = settings->printops.abbreviate_names;
	po.digit_grouping = settings->printops.digit_grouping;
	po.multiplication_sign = settings->printops.multiplication_sign;
	po.division_sign = settings->printops.division_sign;
	po.short_multiplication = settings->printops.short_multiplication;
	po.excessive_parenthesis = settings->printops.excessive_parenthesis;
	po.can_display_unicode_string_function = &can_display_unicode_string_function;
	po.can_display_unicode_string_arg = (void*) valueEdit;
	po.spell_out_logical_operators = settings->printops.spell_out_logical_operators;
	po.binary_bits = bits;
	po.show_ending_zeroes = false;
	po.min_exp = 0;
	if(base == 10 || v) {
		MathStructure value;
		if(v) {
			valueEdit->setText(QString::fromStdString(v->number().print(po)));
			base = 10;
			value.set(*v);
		} else {
			std::string str = valueEdit->text().toStdString();
			remove_blank_ends(str);
			if(str.empty()) {
				value.setAborted();
			} else {
				if(last_is_operator(str, true)) return;
				EvaluationOptions eo;
				eo.parse_options = settings->evalops.parse_options;
				eo.parse_options.read_precision = DONT_READ_PRECISION;
				if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
				if(!settings->simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
				eo.parse_options.base = 10;
				CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 1500, eo);
			}
		}
		if(value.isNumber()) {
			sbin = to_float(value.number(), bits, expbits, sgnpos);
			decnum = value.number();
			use_decnum = true;
		} else if(value.isUndefined()) {
			sbin = to_float(nr_one_i, bits, expbits, sgnpos);
		} else {
			sbin = "";
		}
		CALCULATOR->clearMessages();
	} else if(base == 2) {
		std::string str = binEdit->toPlainText().toStdString();
		remove_blanks(str);
		if(!str.empty()) {
			if(str.find_first_not_of("01") == std::string::npos && str.length() <= bits) {
				sbin = str;
			} else {
				sbin = "";
			}
			CALCULATOR->clearMessages();
		}
	} else if(base == 16) {
		std::string str = hexEdit->text().toStdString();
		remove_blanks(str);
		if(!str.empty()) {
			ParseOptions pa;
			pa.base = BASE_HEXADECIMAL;
			Number nr(str, pa);
			PrintOptions po;
			po.base = BASE_BINARY;
			po.binary_bits = bits;
			po.max_decimals = 0;
			po.use_max_decimals = true;
			po.base_display = BASE_DISPLAY_NONE;
			sbin = nr.print(po);
			if(sbin.length() < bits) sbin.insert(0, bits - sbin.length(), '0');
			if(sbin.length() > bits) {
				sbin = "";
			}
			CALCULATOR->clearMessages();
		}
	}
	valueEdit->blockSignals(true);
	binEdit->blockSignals(true);
	hexEdit->blockSignals(true);
	if(sbin.empty()) {
		if(base != 10) valueEdit->clear();
		if(base != 16) hexEdit->clear();
		if(base != 2) binEdit->clear();
		hexExp2Edit->clear();
		exp2Edit->clear();
		decEdit->clear();
		errorEdit->clear();
	} else {
		int prec_bak = CALCULATOR->getPrecision();
		CALCULATOR->setPrecision(100);
		ParseOptions pa;
		pa.base = BASE_BINARY;
		Number nr(sbin, pa);
		if(base != 16) {po.base = 16; hexEdit->setText(QString::fromStdString(nr.print(po)));}
		if(base != 2) {
			std::string str = sbin;
			if(bits > 32) {
				for(size_t i = expbits + 5; i < str.length() - 1; i += 4) {
					if((bits == 80 && str.length() - i == 32) || (bits == 128 && (str.length() - i == 56))) str.insert(i, "\n");
					else str.insert(i, " ");
					i++;
				}
			}
			str.insert(expbits + 1, bits > 32 ? "\n" : " ");
			str.insert(1, " ");
			binEdit->setPlainText(QString::fromStdString(str));
			QTextCursor cursor = binEdit->textCursor();
			cursor.select(QTextCursor::Document);
			QTextBlockFormat textBlockFormat = cursor.blockFormat();
			textBlockFormat.setAlignment(Qt::AlignRight);
			cursor.mergeBlockFormat(textBlockFormat);
			cursor.setPosition(0);
			binEdit->setTextCursor(cursor);
		}
		if(settings->printops.min_exp == -1 || settings->printops.min_exp == 0) po.min_exp = 8;
		else po.min_exp = settings->printops.min_exp;
		po.base = 10;
		po.max_decimals = 50;
		po.use_max_decimals = true;
		Number value;
		int ret = from_float(value, sbin, bits, expbits, sgnpos);
		if(ret <= 0) {
			decEdit->setText(ret < 0 ? "NaN" : "");
			hexExp2Edit->setText(ret < 0 ? "NaN" : "");
			exp2Edit->setText(ret < 0 ? "NaN" : "");
			errorEdit->clear();
			if(base != 10) valueEdit->setText(QString::fromStdString(m_undefined.print(po)));
		} else {
			if(sbin.length() < bits) sbin.insert(0, bits - sbin.length(), '0');
			Number exponent, significand;
			exponent.set(sbin.substr(sgnpos == 0 ? 1 : 0, expbits), pa);
			Number expbias(2);
			expbias ^= (expbits - 1);
			expbias--;
			bool subnormal = exponent.isZero();
			exponent -= expbias;
			std::string sfloat, sfloathex;
			bool b_approx = false;
			po.is_approximate = &b_approx;
			if(exponent > expbias) {
				if(sbin[0] != '0') sfloat = nr_minus_inf.print(po);
				else sfloat = nr_plus_inf.print(po);
				sfloathex = sfloat;
			} else {
				if(subnormal) exponent++;
				if(subnormal) significand.set(std::string("0.") + sbin.substr((bits == 80 ? 2 : 1) + expbits), pa);
				else significand.set(std::string("1.") + sbin.substr((bits == 80 ? 2 : 1) + expbits), pa);
				if(sbin[sgnpos] != '0') significand.negate();
				int exp_bak = po.min_exp;
				po.min_exp = 0;
				sfloat = significand.print(po);
				if(!subnormal || !significand.isZero()) {
					sfloat += " ";
					sfloat += settings->multiplicationSign();
					sfloat += " ";
					sfloat += "2^";
					sfloat += exponent.print(po);
				}
				if(b_approx) sfloat.insert(0, SIGN_ALMOST_EQUAL " ");
				b_approx = false;
				po.base = 16;
				po.lower_case_numbers = true;
				po.decimalpoint_sign = ".";
				po.use_unicode_signs = false;
				if(significand.isNegative()) {
					significand.negate();
					sfloathex = "-";
				}
				sfloathex += "0x";
				sfloathex += significand.print(po);
				po.base = 10;
				sfloathex += 'p';
				if(sfloathex == "0") sfloathex += "-1";
				else sfloathex += exponent.print(po);
				if(b_approx) sfloat.insert(0, SIGN_ALMOST_EQUAL " ");
				po.decimalpoint_sign = settings->printops.decimalpoint_sign;
				po.lower_case_numbers = false;
				po.min_exp = exp_bak;
				po.use_unicode_signs = settings->printops.use_unicode_signs;
			}
			hexExp2Edit->setText(QString::fromStdString(sfloathex));
			exp2Edit->setText(QString::fromStdString(sfloat));
			b_approx = false;
			std::string svalue = value.print(po);
			if(base != 10) valueEdit->setText(QString::fromStdString(svalue));
			if(b_approx) svalue.insert(0, SIGN_ALMOST_EQUAL " ");
			decEdit->setText(QString::fromStdString(svalue));
			Number nr_error;
			if(use_decnum && (!decnum.isInfinite() || !value.isInfinite())) {
				nr_error = value;
				nr_error -= decnum;
				nr_error.abs();
				if(decnum.isApproximate() && prec_bak < CALCULATOR->getPrecision()) CALCULATOR->setPrecision(prec_bak);
			}
			b_approx = false;
			std::string serror = nr_error.print(po);
			if(b_approx) serror.insert(0, SIGN_ALMOST_EQUAL " ");
			errorEdit->setText(QString::fromStdString(serror));
		}
		CALCULATOR->setPrecision(prec_bak);
	}
	valueEdit->blockSignals(false);
	binEdit->blockSignals(false);
	hexEdit->blockSignals(false);
}
void FPConversionDialog::setValue(const QString &str) {
	valueEdit->blockSignals(true);
	valueEdit->setText(str);
	valueEdit->blockSignals(false);
	updateFields(10);
	valueEdit->selectAll();
	valueEdit->setFocus();
}
void FPConversionDialog::setValue(const MathStructure &m) {
	updateFields(0, &m);
	valueEdit->selectAll();
	valueEdit->setFocus();
}
void FPConversionDialog::setBin(const QString &str) {
	binEdit->blockSignals(true);
	binEdit->setPlainText(str);
	QTextCursor cursor = binEdit->textCursor();
	cursor.select(QTextCursor::Document);
	QTextBlockFormat textBlockFormat = cursor.blockFormat();
	textBlockFormat.setAlignment(Qt::AlignRight);
	cursor.mergeBlockFormat(textBlockFormat);
	cursor.setPosition(0);
	binEdit->setTextCursor(cursor);
	binEdit->blockSignals(false);
	updateFields(2);
	binEdit->setFocus();
}
void FPConversionDialog::setHex(const QString &str) {
	hexEdit->blockSignals(true);
	hexEdit->setText(str);
	hexEdit->blockSignals(false);
	updateFields(16);
	hexEdit->selectAll();
	hexEdit->setFocus();
}
void FPConversionDialog::hexChanged() {
	updateFields(16);
}
void FPConversionDialog::binChanged() {
	updateFields(2);
}
void FPConversionDialog::valueChanged() {
	updateFields(10);
}

