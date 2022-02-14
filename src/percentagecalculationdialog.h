/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PERCENTAGE_CALCULATION_DIALOG_H
#define PERCENTAGE_CALCULATION_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QPushButton;
class QLineEdit;

class PercentageCalculationDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *value1Edit, *value2Edit, *changeEdit, *change1Edit, *change2Edit, *compare1Edit, *compare2Edit;
		QPushButton *clearButton;
		void updatePercentageEntries();
		void percentageEntryChanged(int, bool = false);
		std::vector<int> percentage_entries_changes;
		bool updating_percentage_entries;

	protected slots:

		void onPercentageEntryChanged(const QString&);
		void onPercentageEntryEditingFinished();
		void onClearClicked();

	public:

		PercentageCalculationDialog(QWidget *parent = NULL);
		virtual ~PercentageCalculationDialog();

		void resetValues(const QString &str = QString());

};

#endif //PERCENTAGE_CALCULATION_DIALOG_H

