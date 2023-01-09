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
class QComboBox;

class PreferencesDialog : public QDialog {

	Q_OBJECT

	protected:

		QCheckBox *decimalCommaBox, *ignoreCommaBox, *ignoreDotBox, *statusBox;
		QSpinBox *exratesSpin, *statusDelayWidget;
		QComboBox *styleCombo, *parseCombo, *tcCombo;

		void closeEvent(QCloseEvent*) override;

	protected slots:

		void ignoreLocaleToggled(bool);
		void keepAboveToggled(bool);
		void expressionStatusToggled(bool);
		void statusDelayChanged(int);
		void binTwosToggled(bool);
		void hexTwosToggled(bool);
		void lowerCaseToggled(bool);
		void multiplicationDotToggled(bool);
		void divisionSlashToggled(bool);
		void spellOutToggled(bool);
		void eToggled(bool);
		void imaginaryJToggled(bool);
		void decimalCommaToggled(bool);
		void ignoreCommaToggled(bool);
		void ignoreDotToggled(bool);
		void colorizeToggled(bool);
		void formatToggled(bool);
		void parsingModeChanged(int);
		void temperatureCalculationChanged(int);
		void exratesToggled(bool);
		void exratesChanged(int);
		void binaryPrefixesToggled(bool);
		void readPrecisionToggled(bool);
		void simplifiedPercentageToggled(bool);
		void intervalCalculationChanged(int);
		void complexFormChanged(int);
		void roundingChanged(int);
		void repeatingDecimalsToggled(bool);
		void mixedUnitsToggled(bool);
		void abbreviateNamesToggled(bool);
		void conversionChanged(int);
		void prefixesChanged(int);
		void allPrefixesToggled(bool);
		void denominatorPrefixToggled(bool);
		void variableUnitsToggled(bool);
		void groupingChanged(int);
		void intervalDisplayChanged(int);
		void limitImplicitToggled(bool);
		void titleChanged(int);
		void resultFontClicked();
		void resultFontToggled(bool);
		void expressionFontClicked();
		void expressionFontToggled(bool);
		void keypadFontClicked();
		void keypadFontToggled(bool);
		void appFontClicked();
		void appFontToggled(bool);
		void darkModeToggled(bool);
		void styleChanged(int);
		void factorizeToggled(bool);
		void rpnKeysToggled(bool);
		void replaceExpressionChanged(int);
		void multipleInstancesToggled(bool);
		void clearHistoryToggled(bool);
		void historyExpressionChanged(int);
		void copyAsciiToggled(bool);
		void caretAsXorToggled(bool);

	public:

		PreferencesDialog(QWidget *parent = NULL);
		virtual ~PreferencesDialog();

		void updateDot();
		void updateParsingMode();
		void updateTemperatureCalculation();
		void updateExpressionStatus();

	signals:

		void resultFormatUpdated();
		void resultDisplayUpdated();
		void expressionCalculationUpdated(int);
		void expressionFormatUpdated(bool);
		void alwaysOnTopChanged();
		void titleTypeChanged();
		void resultFontChanged();
		void expressionFontChanged();
		void keypadFontChanged();
		void appFontChanged();
		void symbolsUpdated();
		void historyExpressionTypeChanged();
		void dialogClosed();

};

#endif //PREFERENCES_DIALOG_H

