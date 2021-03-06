/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef VARIABLE_EDIT_DIALOG_H
#define VARIABLE_EDIT_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QCheckBox;
class QPushButton;
class MatrixWidget;
class QComboBox;
class QPlainTextEdit;
class NamesEditDialog;

class VariableEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit, *titleEdit;
		QComboBox *categoryEdit;
		QCheckBox *temporaryBox, *hideBox;
		QPlainTextEdit *descriptionEdit, *valueEdit;
		QPushButton *okButton;
		MatrixWidget *matrixEdit;
		NamesEditDialog *namesEditDialog;
		Variable *o_variable;
		bool b_empty, b_matrix, b_changed, name_edited;

	protected slots:

		void onNameEdited(const QString&);
		void onValueEdited();
		void onVariableChanged();
		void onMatrixDimensionChanged();
		void onMatrixChanged();
		void editNames();
		void temporaryClicked();
		void categoryChanged(const QString&);

	public:

		VariableEditDialog(QWidget *parent = NULL, bool allow_empty_value = true, bool edit_matrix = false);
		virtual ~VariableEditDialog();

		KnownVariable *createVariable(MathStructure *default_value = NULL, ExpressionItem **replaced_item = NULL);
		bool modifyVariable(KnownVariable *v, MathStructure *default_value = NULL, ExpressionItem **replaced_item = NULL);
		void setVariable(KnownVariable *v);
		void setValue(const QString&);
		void disableValue();
		bool valueHasChanged() const;
		QString value() const;
		void setName(const QString&);

		static bool editVariable(QWidget *parent, KnownVariable *v, ExpressionItem **replaced_item = NULL);
		static KnownVariable *newVariable(QWidget *parent, MathStructure *default_value = NULL, const QString &value_str = QString(), ExpressionItem **replaced_item = NULL);
		static KnownVariable *newMatrix(QWidget *parent, ExpressionItem **replaced_item = NULL);

};

#endif //VARIABLE_EDIT_DIALOG_H
