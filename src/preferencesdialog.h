/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QCheckBox;
class QSpinBox;

class PreferencesDialog : public QDialog {

	Q_OBJECT

	protected:

		QCheckBox *ignoreCommaBox, *ignoreDotBox;
		QSpinBox *exratesSpin;

		void closeEvent(QCloseEvent*) override;

	protected slots:

		void ignoreLocaleToggled(bool);
		void keepAboveToggled(bool);
		void expressionStatusToggled(bool);
		void binTwosToggled(bool);
		void hexTwosToggled(bool);
		void lowerCaseToggled(bool);
		void spellOutToggled(bool);
		void eToggled(bool);
		void imaginaryJToggled(bool);
		void decimalCommaToggled(bool);
		void ignoreCommaToggled(bool);
		void ignoreDotToggled(bool);
		void colorizeToggled(bool);
		void parsingModeChanged(int);
		void temperatureCalculationChanged(int);
		void exratesToggled(bool);
		void exratesChanged(int);
		void binaryPrefixesToggled(bool);
		void readPrecisionToggled(bool);
		void intervalCalculationChanged(int);
		void complexFormChanged(int);
		void roundEvenToggled(bool);
		void mixedUnitsToggled(bool);
		void conversionChanged(int);
		void prefixesChanged(int);
		void allPrefixesToggled(bool);
		void denominatorPrefixToggled(bool);
		void variableUnitsToggled(bool);
		void groupingChanged(int);
		void intervalDisplayChanged(int);
		void limitImplicitToggled(bool);
		void titleChanged(int);

	public:

		PreferencesDialog(QWidget *parent = NULL);
		virtual ~PreferencesDialog();

	signals:

		void resultFormatUpdated();
		void resultDisplayUpdated();
		void expressionCalculationUpdated(int);
		void expressionFormatUpdated(bool);
		void alwaysOnTopChanged();
		void titleTypeChanged();
		void symbolsUpdated();
		void dialogClosed();

};

#endif //PREFERENCES_DIALOG_H

