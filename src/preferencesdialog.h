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

class QAbstractButton;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QLineEdit;

class PreferencesDialog : public QDialog {

	Q_OBJECT

	protected:

		QAbstractButton *decimalCommaBox, *ignoreCommaBox, *ignoreDotBox, *ignoreLocaleBox, *variableUnitsBox, *conciseUncertaintyInputBox, *rpnKeysBox, *adaptiveDelayBox, *autocalcSelectionBox;
		QCheckBox *preserveHeightBox;
		QSpinBox *exratesSpin, *statusDelayWidget, *calculateDelayWidget, *cgfSpin;
		QComboBox *styleCombo, *parseCombo, *tcCombo, *langCombo, *complexFormCombo, *intervalDisplayCombo, *intervalCalculationCombo, *statusCombo;
		QLineEdit *cgsEdit;

		void closeEvent(QCloseEvent*) override;

	protected slots:

		void ignoreLocaleToggled(bool);
		void keepAboveToggled(bool);
		void tooltipsChanged(int);
		void preserveHeightChanged(int);
		void statusModeChanged(int);
		void statusDelayChanged(int);
		void autocalcSelectionToggled(bool);
		void calculateDelayChanged(int);
		void adaptiveDelayToggled(bool);
		void binTwosToggled(bool);
		void hexTwosToggled(bool);
		void binTwosInputToggled(bool);
		void hexTwosInputToggled(bool);
		void bitsChanged(int);
		void lowerCaseToggled(bool);
		void duodecimalSymbolsToggled(bool);
		void multiplicationSignChanged(int);
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
		void unknownsToggled(bool);
		void temperatureCalculationChanged(int);
		void defaultCurrencyChanged(const QString&);
		void exratesToggled(bool);
		void exratesChanged(int);
		void unitsToggled(bool);
		void binaryPrefixesToggled(bool);
		void readPrecisionToggled(bool);
		void simplifiedPercentageToggled(bool);
		void intervalCalculationChanged(int);
		void complexFormChanged(int);
		void roundingChanged(int);
		void repeatingDecimalsChanged(int);
		void mixedUnitsToggled(bool);
		void abbreviateNamesToggled(bool);
		void conversionChanged(int);
		void prefixesChanged(int);
		void allPrefixesToggled(bool);
		void denominatorPrefixToggled(bool);
		void variableUnitsToggled(bool);
		void updateCustomDigitGrouping();
		void groupingChanged(int);
		void cgfChanged(int);
		void cgsChanged(const QString&);
		void intervalDisplayChanged(int);
		void conciseUncertaintyInputToggled(bool);
		void limitImplicitToggled(bool);
		void titleChanged(int);
		void resultFontClicked();
		void resultFontToggled(bool);
		void expressionFontClicked();
		void expressionFontToggled(bool);
		void statusFontClicked();
		void statusFontToggled(bool);
		void keypadFontClicked();
		void keypadFontToggled(bool);
		void appFontClicked();
		void appFontToggled(bool);
		void darkModeToggled(bool);
		void styleChanged(int);
		void langChanged(int);
		void factorizeToggled(bool);
		void rpnKeysToggled(bool);
		void replaceExpressionChanged(int);
		void autocopyResultToggled(bool);
		void multipleInstancesToggled(bool);
		void checkVersionToggled(bool);
		void clearHistoryToggled(bool);
		void maxHistoryLinesChanged(int);
		void historyExpressionChanged(int);
		void copyAsciiToggled(bool);
		void copyAsciiWithoutUnitsToggled(bool);
		void caretAsXorToggled(bool);
		void automaticDigitGroupingToggled(bool);
		void closeWithEscToggled(bool);
		void disableCursorBlinkingToggled(bool);
		void showPercentInNumpadToggled(bool);
		void expressionPositionToggled(bool);
		void showStatusBarToggled(bool);

	public:

		PreferencesDialog(QWidget *parent = NULL);
		virtual ~PreferencesDialog();

		void updateDot();
		void updateParsingMode();
		void updateTemperatureCalculation();
		void updateExpressionStatus();
		void updateVariableUnits();
		void updateComplexForm();
		void updateIntervalDisplay();
		void updateIntervalCalculation();
		void updateConciseUncertaintyInput();

	signals:

		void resultFormatUpdated();
		void resultDisplayUpdated();
		void expressionCalculationUpdated(int);
		void expressionFormatUpdated(bool);
		void alwaysOnTopChanged();
		void enableTooltipsChanged();
		void titleTypeChanged();
		void resultFontChanged();
		void expressionFontChanged();
		void statusFontChanged();
		void keypadFontChanged();
		void appFontChanged();
		void symbolsUpdated();
		void historyExpressionTypeChanged();
		void binaryBitsChanged();
		void statusModeChanged();
		void buttonLocationChanged();
		void automaticDigitGroupingChanged();
		void expressionPositionChanged();
		void showStatusBarChanged();
		void dialogClosed();

};

#endif //PREFERENCES_DIALOG_H

