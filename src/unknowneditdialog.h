/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNKNOWN_EDIT_DIALOG_H
#define UNKNOWN_EDIT_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QComboBox;
class NamesEditDialog;

class UnknownEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit;
		QCheckBox *customBox;
		QComboBox *typeCombo, *signCombo;
		QPushButton *okButton;
		NamesEditDialog *namesEditDialog;
		Variable *o_variable;
		bool name_edited;

	protected slots:

		void onNameEdited(const QString&);
		void onTypeChanged(int);
		void onSignChanged(int);
		void onCustomToggled(bool);
		void editNames();

	public:

		UnknownEditDialog(QWidget *parent = NULL);
		virtual ~UnknownEditDialog();

		UnknownVariable *createVariable(ExpressionItem **replaced_item = NULL);
		bool modifyVariable(UnknownVariable *v, ExpressionItem **replaced_item = NULL);
		void setVariable(UnknownVariable *v);
		void setName(const QString&);

		static bool editVariable(QWidget *parent, UnknownVariable *, ExpressionItem **replaced_item = NULL);
		static UnknownVariable* newVariable(QWidget *parent, ExpressionItem **replaced_item = NULL);

};

#endif //UNKNOWN_EDIT_DIALOG_H
