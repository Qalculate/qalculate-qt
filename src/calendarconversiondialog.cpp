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
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "calendarconversiondialog.h"

CalendarConversionDialog::CalendarConversionDialog(QWidget *parent) : QDialog(parent), block_calendar_conversion(false) {
	setWindowTitle(tr("Calendar Conversion"));
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	CalendarSystem cs; QString str;
	for(int i = 0; i < 10; i++) {
		switch(i) {
			case 0: {str = tr("Gregorian"); cs = CALENDAR_GREGORIAN; break;}
			case 1: {str = tr("Hebrew"); cs = CALENDAR_HEBREW; break;}
			case 2: {str = tr("Islamic (Hijri)"); cs = CALENDAR_ISLAMIC; break;}
			case 3: {str = tr("Persian (Solar Hijri)"); cs = CALENDAR_PERSIAN; break;}
			case 4: {str = tr("Indian (National)"); cs = CALENDAR_INDIAN; break;}
			case 5: {str = tr("Chinese"); cs = CALENDAR_CHINESE; break;}
			case 6: {str = tr("Julian"); cs = CALENDAR_JULIAN; break;}
			case 7: {str = tr("Revised Julian (Milanković)"); cs = CALENDAR_MILANKOVIC; break;}
			case 8: {str = tr("Coptic"); cs = CALENDAR_COPTIC; break;}
			case 9: {str = tr("Ethiopian"); cs = CALENDAR_ETHIOPIAN; break;}
		}
		grid->addWidget(new QLabel(str, this), i, 0);
		if(cs == CALENDAR_CHINESE) {
			chineseStemCombo = new QComboBox(this); chineseBranchCombo = new QComboBox(this);
			chineseStemCombo->setProperty("QALCULATE INDEX", i); chineseStemCombo->setProperty("QALCULATE CALENDAR", cs);
			chineseBranchCombo->setProperty("QALCULATE INDEX", i); chineseBranchCombo->setProperty("QALCULATE CALENDAR", cs);
			for(int i2 = 1; i2 <= 5; i2++) chineseStemCombo->addItem(QString::fromStdString(chineseStemName(i2 * 2)));
			for(int i2 = 1; i2 <= 12; i2++) chineseBranchCombo->addItem(QString::fromStdString(chineseBranchName(i2)));
			grid->addWidget(chineseStemCombo, i, 1); grid->addWidget(chineseBranchCombo, i, 2);
			connect(chineseStemCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCalendars()));
			connect(chineseBranchCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateCalendars()));
		} else {
			yearEdit[i] = new QSpinBox(this); yearEdit[i]->setRange(INT_MIN, INT_MAX);
			yearEdit[i]->setProperty("QALCULATE INDEX", i); yearEdit[i]->setProperty("QALCULATE CALENDAR", cs);
			yearEdit[i]->setAlignment(Qt::AlignRight);
			grid->addWidget(yearEdit[i], i, 1, 1, 2);
			connect(yearEdit[i], SIGNAL(valueChanged(int)), this, SLOT(updateCalendars()));
		}
		monthCombo[i] = new QComboBox(this); dayCombo[i] = new QComboBox(this);
		for(int i2 = 1; i2 <= numberOfMonths(cs); i2++) monthCombo[i]->addItem(QString::fromStdString(monthName(i2, cs)));
		for(int i2 = 1; i2 <= 31; i2++) dayCombo[i]->addItem(QString::number(i2));
		monthCombo[i]->setProperty("QALCULATE INDEX", i); monthCombo[i]->setProperty("QALCULATE CALENDAR", cs);
		dayCombo[i]->setProperty("QALCULATE INDEX", i); dayCombo[i]->setProperty("QALCULATE CALENDAR", cs);
		grid->addWidget(monthCombo[i], i, 3); grid->addWidget(dayCombo[i], i, 4);
		connect(monthCombo[i], SIGNAL(currentIndexChanged(int)), this, SLOT(updateCalendars()));
		connect(dayCombo[i], SIGNAL(currentIndexChanged(int)), this, SLOT(updateCalendars()));
	}
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
}
CalendarConversionDialog::~CalendarConversionDialog() {}

void CalendarConversionDialog::updateCalendars() {
	if(block_calendar_conversion) return;
	block_calendar_conversion = true;
	int index = sender()->property("QALCULATE INDEX").toInt();
	CalendarSystem cs = (CalendarSystem) sender()->property("QALCULATE CALENDAR").toInt();
	long int y;
	if(cs == CALENDAR_CHINESE) {
		long int cy = chineseStemBranchToCycleYear((chineseStemCombo->currentIndex() * 2) + 1, chineseBranchCombo->currentIndex() + 1);
		if(cy <= 0) {
			QMessageBox::critical(this, tr("Error"), tr("The selected Chinese year does not exist."));
			block_calendar_conversion = false;
			return;
		}
		y = chineseCycleYearToYear(79, cy);
	} else {
		y = yearEdit[index]->value();
	}
	long int m = monthCombo[index]->currentIndex() + 1;
	long int d = dayCombo[index]->currentIndex() + 1;
	QalculateDateTime date;
	if(!calendarToDate(date, y, m, d, cs)) {
		QMessageBox::critical(this, tr("Error"), tr("Conversion to Gregorian calendar failed."));
		block_calendar_conversion = false;
		return;
	}
	QString failed_str;
	for(int i = 0; i < 10; i++) {
		switch(i) {
			case 0: {cs = CALENDAR_GREGORIAN; break;}
			case 1: {cs = CALENDAR_HEBREW; break;}
			case 2: {cs = CALENDAR_ISLAMIC; break;}
			case 3: {cs = CALENDAR_PERSIAN; break;}
			case 4: {cs = CALENDAR_INDIAN; break;}
			case 5: {cs = CALENDAR_CHINESE; break;}
			case 6: {cs = CALENDAR_JULIAN; break;}
			case 7: {cs = CALENDAR_MILANKOVIC; break;}
			case 8: {cs = CALENDAR_COPTIC; break;}
			case 9: {cs = CALENDAR_ETHIOPIAN; break;}
		}
		if(dateToCalendar(date, y, m, d, cs) && y <= INT_MAX && y >= INT_MIN && m <= numberOfMonths(cs) && d <= 31) {
			if(cs == CALENDAR_CHINESE) {
				long int cy, yc, st, br;
				chineseYearInfo(y, cy, yc, st, br);
				chineseStemCombo->setCurrentIndex((st - 1) / 2);
				chineseBranchCombo->setCurrentIndex(br - 1);
			} else {
				yearEdit[i]->setValue(y);
			}
			monthCombo[i]->setCurrentIndex(m - 1);
			dayCombo[i]->setCurrentIndex(d - 1);
		} else {
			if(!failed_str.isEmpty()) failed_str += ", ";
			switch(i) {
				case 0: {failed_str += tr("Gregorian"); break;}
				case 1: {failed_str += tr("Hebrew"); break;}
				case 2: {failed_str += tr("Islamic (Hijri)"); break;}
				case 3: {failed_str += tr("Persian (Solar Hijri)"); break;}
				case 4: {failed_str += tr("Indian (National)"); break;}
				case 5: {failed_str += tr("Chinese"); break;}
				case 6: {failed_str += tr("Julian"); break;}
				case 7: {failed_str += tr("Revised Julian (Milanković)"); break;}
				case 8: {failed_str += tr("Coptic"); break;}
				case 9: {failed_str += tr("Ethiopian"); break;}
			}
		}
	}
	if(!failed_str.isEmpty()) QMessageBox::warning(this, tr("Error"), tr("Calendar conversion failed for: %1.").arg(failed_str));
	block_calendar_conversion = false;
}
void CalendarConversionDialog::setDate(QalculateDateTime date) {
	block_calendar_conversion = true;
	yearEdit[0]->setValue(date.year());
	monthCombo[0]->setCurrentIndex(date.month() - 1);
	block_calendar_conversion = false;
	dayCombo[0]->setCurrentIndex(date.day() - 1);
}

