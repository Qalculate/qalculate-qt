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

class VariableEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit, *valueEdit;
		QCheckBox *temporaryBox;
		QPushButton *okButton;
		bool b_empty;

	protected slots:

		void onNameEdited(const QString&);
		void onValueEdited(const QString&);

	public:

		VariableEditDialog(QWidget *parent = NULL, bool allow_empty_value = true);
		virtual ~VariableEditDialog();

		KnownVariable *createVariable(MathStructure *default_value = NULL, ExpressionItem **replaced_item = NULL);
		bool modifyVariable(KnownVariable *v, MathStructure *default_value = NULL, ExpressionItem **replaced_item = NULL);
		void setVariable(KnownVariable *v);
		void setValue(const QString&);
		QString value() const;
		void setName(const QString&);

		static bool editVariable(QWidget *parent, KnownVariable *v, MathStructure *default_value = NULL, ExpressionItem **replaced_item = NULL);
		static KnownVariable* newVariable(QWidget *parent, MathStructure *default_value = NULL, const QString &value_str = QString(), ExpressionItem **replaced_item = NULL);

};

#endif //VARIABLE_EDIT_DIALOG_H
