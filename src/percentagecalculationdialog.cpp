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
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFontMetrics>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "percentagecalculationdialog.h"

PercentageCalculationDialog::PercentageCalculationDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle(tr("Percentage"));
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Value 1"), this), 0, 0, Qt::AlignRight);
	value1Edit = new MathLineEdit(this); value1Edit->setAlignment(Qt::AlignRight); grid->addWidget(value1Edit, 0, 1);
	QFontMetrics fm(value1Edit->font());
	QString str; str.fill('0', 27);
	value1Edit->setMinimumWidth(fm.boundingRect(str).width());
	grid->addWidget(new QLabel(tr("Value 2"), this), 1, 0, Qt::AlignRight);
	value2Edit = new MathLineEdit(this); value2Edit->setAlignment(Qt::AlignRight); grid->addWidget(value2Edit, 1, 1);
	grid->addWidget(new QLabel(tr("Change from 1 to 2"), this), 2, 0, Qt::AlignRight);
	changeEdit = new MathLineEdit(this); changeEdit->setAlignment(Qt::AlignRight); grid->addWidget(changeEdit, 2, 1);
	grid->addWidget(new QLabel(tr("Change from 1 to 2"), this), 3, 0, Qt::AlignRight);
	change1Edit = new MathLineEdit(this); change1Edit->setAlignment(Qt::AlignRight); grid->addWidget(change1Edit, 3, 1);
	grid->addWidget(new QLabel("%", this), 3, 2, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("Change from 2 to 1"), this), 4, 0, Qt::AlignRight);
	change2Edit = new MathLineEdit(this); change2Edit->setAlignment(Qt::AlignRight); grid->addWidget(change2Edit, 4, 1);
	grid->addWidget(new QLabel("%", this), 4, 2, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("2 compared to 1"), this), 5, 0, Qt::AlignRight);
	compare1Edit = new MathLineEdit(this); compare1Edit->setAlignment(Qt::AlignRight); grid->addWidget(compare1Edit, 5, 1);
	grid->addWidget(new QLabel("%", this), 5, 2, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("1 compared to 2"), this), 6, 0, Qt::AlignRight);
	compare2Edit = new MathLineEdit(this); compare2Edit->setAlignment(Qt::AlignRight); grid->addWidget(compare2Edit, 6, 1);
	grid->addWidget(new QLabel("%", this), 6, 2, Qt::AlignRight);
	value1Edit->setFocus();
	QLabel *descr = new QLabel("<i>" + tr("Enter two values, of which at most one is a percentage, and the others will be calculated for you.") + "</i>", this);
	descr->setWordWrap(true);
	descr->setContentsMargins(0, 6, 0, 12);
	grid->addWidget(descr, 7, 0, 1, 3);
	box->addLayout(grid);
	box->addStretch(1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	clearButton = buttonBox->addButton(tr("Clear"), QDialogButtonBox::ResetRole);
	clearButton->setEnabled(false);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(clearButton, SIGNAL(clicked()), this, SLOT(onClearClicked()));
	connect(value1Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onValue1EditChanged(const QString&)));
	connect(value2Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onValue2EditChanged(const QString&)));
	connect(changeEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onChangeEditChanged(const QString&)));
	connect(change1Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onChange1EditChanged(const QString&)));
	connect(change2Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onChange2EditChanged(const QString&)));
	connect(compare1Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onCompare1EditChanged(const QString&)));
	connect(compare2Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onCompare2EditChanged(const QString&)));
	updating_percentage_entries = false;
}
PercentageCalculationDialog::~PercentageCalculationDialog() {}

void PercentageCalculationDialog::resetValues(const QString &str) {
	onClearClicked();
	if(!str.trimmed().isEmpty()) {
		value1Edit->setText(str.trimmed());
		onValue1EditChanged(value1Edit->text());
	}
	value1Edit->setFocus();
}
void PercentageCalculationDialog::onClearClicked() {
	updating_percentage_entries = true;
	value1Edit->clear();
	value2Edit->clear();
	changeEdit->clear();
	change1Edit->clear();
	change2Edit->clear();
	compare1Edit->clear();
	compare2Edit->clear();
	clearButton->setEnabled(false);
	value1Edit->setFocus();
	percentage_entries_changes.clear();
	updating_percentage_entries = false;
}
void PercentageCalculationDialog::onValue1EditChanged(const QString &str) {percentageEntryChanged(1, str.trimmed().isEmpty());}
void PercentageCalculationDialog::onValue2EditChanged(const QString &str) {percentageEntryChanged(2, str.trimmed().isEmpty());}
void PercentageCalculationDialog::onChangeEditChanged(const QString &str) {percentageEntryChanged(4, str.trimmed().isEmpty());}
void PercentageCalculationDialog::onChange1EditChanged(const QString &str) {percentageEntryChanged(8, str.trimmed().isEmpty());}
void PercentageCalculationDialog::onChange2EditChanged(const QString &str) {percentageEntryChanged(16, str.trimmed().isEmpty());}
void PercentageCalculationDialog::onCompare1EditChanged(const QString &str) {percentageEntryChanged(32, str.trimmed().isEmpty());}
void PercentageCalculationDialog::onCompare2EditChanged(const QString &str) {percentageEntryChanged(64, str.trimmed().isEmpty());}
void PercentageCalculationDialog::percentageEntryChanged(int entry_id, bool is_empty) {
	if(updating_percentage_entries) return;
	for(size_t i = 0; i < percentage_entries_changes.size(); i++) {
		if(percentage_entries_changes[i] == entry_id) {
			percentage_entries_changes.erase(percentage_entries_changes.begin() + i);
			break;
		}
	}
	if(!is_empty) {
		percentage_entries_changes.push_back(entry_id);
		updatePercentageEntries();
	}
	clearButton->setEnabled(!percentage_entries_changes.empty());
}
void PercentageCalculationDialog::updatePercentageEntries() {
	if(updating_percentage_entries) return;
	if(percentage_entries_changes.size() < 2) return;
	int variant = percentage_entries_changes[percentage_entries_changes.size() - 1];
	int variant2 = percentage_entries_changes[percentage_entries_changes.size() - 2];
	if(variant > 4) {
		for(int i = percentage_entries_changes.size() - 3; i >= 0 && variant2 > 4; i--) {
			variant2 = percentage_entries_changes[(size_t) i];
		}
		if(variant2 > 4) return;
	}
	variant += variant2;
	updating_percentage_entries = true;
	MathStructure m1, m2, m3, m4, m5, m6, m7, m1_pre, m2_pre;
	QString str1, str2;
	switch(variant) {
		case 3: {str1 = value1Edit->text(); str2 = value2Edit->text(); break;}
		case 5: {str1 = value1Edit->text(); str2 = changeEdit->text(); break;}
		case 9: {str1 = value1Edit->text(); str2 = change1Edit->text(); break;}
		case 17: {str1 = value1Edit->text(); str2 = change2Edit->text(); break;}
		case 33: {str1 = value1Edit->text(); str2 = compare1Edit->text(); break;}
		case 65: {str1 = value1Edit->text(); str2 = compare2Edit->text(); break;}
		case 6: {str1 = value2Edit->text(); str2 = changeEdit->text(); break;}
		case 10: {str1 = value2Edit->text(); str2 = change1Edit->text(); break;}
		case 18: {str1 = value2Edit->text(); str2 = change2Edit->text(); break;}
		case 34: {str1 = value2Edit->text(); str2 = compare1Edit->text(); break;}
		case 66: {str1 = value2Edit->text(); str2 = compare2Edit->text(); break;}
		case 12: {str1 = changeEdit->text(); str2 = change1Edit->text(); break;}
		case 20: {str1 = changeEdit->text(); str2 = change2Edit->text(); break;}
		case 36: {str1 = changeEdit->text(); str2 = compare1Edit->text(); break;}
		case 68: {str1 = changeEdit->text(); str2 = compare2Edit->text(); break;}
		default: {variant = 0;}
	}
	if(str2 == SIGN_MINUS || str1 == SIGN_MINUS) {updating_percentage_entries = false; return;}
	EvaluationOptions eo;
	eo.parse_options = settings->evalops.parse_options;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	eo.parse_options.base = 10;
	eo.assume_denominators_nonzero = true;
	eo.warn_about_denominators_assumed_nonzero = false;
	if(variant != 0) {
		m1_pre.set(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str1.toStdString(), eo.parse_options), eo.parse_options));
		m2_pre.set(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(str2.toStdString(), eo.parse_options), eo.parse_options));
	}
	bool b_divzero = false;
	MathStructure mtest;
	if(variant == 17 || variant == 65 || variant == 10 || variant == 34 || variant == 12 || variant == 20 || variant == 36 || variant == 68) {
		mtest = m2_pre;
		CALCULATOR->calculate(&mtest, 500, eo);
		if(!mtest.isNumber()) mtest = m_one;
	}
	switch(variant) {
		case 3: {m1 = m1_pre; m2 = m2_pre; break;}
		case 5: {m1 = m1_pre; m2 = m2_pre; m2 += m1; break;}
		case 9: {m1 = m1_pre; m2 = m2_pre; m2 /= 100; m2 += 1; m2 *= m1; break;}
		case 17: {
			ComparisonResult cr = mtest.number().compare(-100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m2_pre += 1; m2 = m1; m2 /= m2_pre;
			break;
		}
		case 33: {m1 = m1_pre; m2 = m2_pre; m2 /= 100; m2 *= m1; break;}
		case 65: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m2 = m1; m2 /= m2_pre;
			break;
		}
		case 6: {m2 = m1_pre; m1 = m1_pre; m1 -= m2_pre; break;}
		case 10: {
			ComparisonResult cr = mtest.number().compare(-100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m2 = m1_pre; m2_pre /= 100; m2_pre += 1; m1 = m2; m1 /= m2_pre;
			break;
		}
		case 18: {m2 = m1_pre; m2_pre /= 100; m2_pre += 1; m1 = m2; m1 *= m2_pre; break;}
		case 34: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m2 = m1_pre; m2_pre /= 100; m1 = m2; m1 /= m2_pre;
			break;
		}
		case 66: {m2 = m1_pre; m2_pre /= 100; m1 = m2; m1 *= m2_pre; break;}
		case 12: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m1 /= m2_pre; m2 = m1; m2 += m1_pre;
			break;
		}
		case 20: {
			if(!mtest.number().isNonZero()) {b_divzero = true; break;}
			m1_pre.negate(); m2 = m1_pre; m2_pre /= 100; m2 /= m2_pre; m1 = m2; m1 += m1_pre;
			break;
		}
		case 36: {
			ComparisonResult cr = mtest.number().compare(100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m1 = m1_pre; m2_pre /= 100; m2_pre -= 1; m1 /= m2_pre; m2 = m1; m2 += m1_pre;
			break;
		}
		case 68: {
			ComparisonResult cr = mtest.number().compare(100);
			if(cr == COMPARISON_RESULT_EQUAL || COMPARISON_MIGHT_BE_EQUAL(cr)) {b_divzero = true; break;}
			m1_pre.negate(); m2 = m1_pre; m2_pre /= 100; m2_pre -= 1; m2 /= m2_pre; m1 = m2; m1 += m1_pre;
			break;
		}
		default: {variant = 0;}
	}
	if(b_divzero) {
		if(variant != 3 && variant != 5 && variant != 9 && variant != 17 && variant != 33 && variant != 65) value1Edit->clear();
		if(variant != 3 && variant != 6 && variant != 10 && variant != 18 && variant != 34 && variant != 66) value2Edit->clear();
		if(variant != 5 && variant != 6 && variant != 12 && variant != 20 && variant != 36 && variant != 68) changeEdit->clear();
		if(variant != 9 && variant != 10 && variant != 12) change1Edit->clear();
		if(variant != 17 && variant != 18 && variant != 20) change2Edit->clear();
		if(variant != 33 && variant != 34 && variant != 36) compare1Edit->clear();
		if(variant != 65 && variant != 66 && variant != 68) compare2Edit->clear();
	} else if(variant != 0) {
		CALCULATOR->calculate(&m1, 500, eo);
		CALCULATOR->calculate(&m2, 500, eo);
		m3 = m2; m3 -= m1;
		m6 = m2; m6 /= m1;
		m7 = m1; m7 /= m2;
		m4 = m6; m4 -= 1;
		m5 = m7; m5 -= 1;
		m4 *= 100; m5 *= 100; m6 *= 100; m7 *= 100;
		CALCULATOR->calculate(&m1, 500, eo);
		CALCULATOR->calculate(&m2, 500, eo);
		CALCULATOR->calculate(&m3, 500, eo);
		if(!m1.isZero()) CALCULATOR->calculate(&m4, 500, eo);
		if(!m2.isZero()) CALCULATOR->calculate(&m5, 500, eo);
		if(!m1.isZero()) CALCULATOR->calculate(&m6, 500, eo);
		if(!m2.isZero()) CALCULATOR->calculate(&m7, 500, eo);
		PrintOptions po = settings->printops;
		po.base = 10;
		po.number_fraction_format = FRACTION_DECIMAL;
		po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		value1Edit->setText(QString::fromStdString(m1.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m1, 200, po)));
		value2Edit->setText(QString::fromStdString(m2.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m2, 200, po)));
		changeEdit->setText(QString::fromStdString(m3.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m3, 200, po)));
		po.max_decimals = 2;
		po.use_max_decimals = true;
		change1Edit->setText(m1.isZero() ? QString() : QString::fromStdString(m4.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m4, 200, po)));
		change2Edit->setText(m2.isZero() ? QString() : QString::fromStdString(m5.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m5, 200, po)));
		compare1Edit->setText(m1.isZero() ? QString() : QString::fromStdString(m6.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m6, 200, po)));
		compare2Edit->setText(m2.isZero() ? QString() : QString::fromStdString(m7.isAborted() ? CALCULATOR->timedOutString() : CALCULATOR->print(m7, 200, po)));
	}
	CALCULATOR->clearMessages();
	updating_percentage_entries = false;
}

