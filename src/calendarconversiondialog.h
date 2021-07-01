/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef CALENDAR_CONVERSION_DIALOG_H
#define CALENDAR_CONVERSION_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QComboBox;
class QSpinBox;

class CalendarConversionDialog : public QDialog {

	Q_OBJECT

	protected:

		QSpinBox *yearEdit[10];
		QComboBox *monthCombo[10], *dayCombo[10], *chineseStemCombo, *chineseBranchCombo;
		bool block_calendar_conversion;

	protected slots:

		void updateCalendars();

	public slots:

		void setDate(QalculateDateTime date);

	public:

		CalendarConversionDialog(QWidget *parent = NULL);
		virtual ~CalendarConversionDialog();

};

#endif //CALENDAR_CONVERSION_DIALOG_H

