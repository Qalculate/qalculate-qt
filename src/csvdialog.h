/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef CSV_DIALOG_H
#define CSV_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QRadioButton;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QPushButton;
class QAbstractButton;

class CSVDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit, *delimiterEdit, *fileEdit;
		QComboBox *delimiterCombo;
		QRadioButton *resultButton, *variableButton, *matrixButton, *vectorButton;
		QPushButton *okButton;
		QCheckBox *headingsBox;
		QSpinBox *rowSpin;
		bool b_import;
		KnownVariable *o_variable;
		MathStructure *m_result;

	public:

		CSVDialog(bool do_import, QWidget *parent = NULL, MathStructure *current_result = NULL, KnownVariable *var = NULL);
		virtual ~CSVDialog();

		bool importExport();

		static bool importCSVFile(QWidget *parent);
		static bool exportCSVFile(QWidget *parent, MathStructure *current_result = NULL, KnownVariable *var = NULL);

	protected slots:

		void onNameEdited(const QString&);
		void enableDisableOk();
		void exportTypeToggled(QAbstractButton*, bool);
		void onSelectFile();
		void onDelimiterEdited();
		void onFileEdited();
		void onDelimiterChanged(int);

};

#endif //CSV_DIALOG_H
