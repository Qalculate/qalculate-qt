/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNIT_EDIT_DIALOG_H
#define UNIT_EDIT_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QComboBox;
class QPlainTextEdit;
class QSpinBox;
class QLabel;
class NamesEditDialog;
class QTabWidget;

class UnitEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit, *titleEdit, *baseEdit, *relationEdit, *inverseEdit;
		QComboBox *categoryEdit, *systemEdit, *typeCombo;
		QCheckBox *hideBox, *mixBox, *prefixBox;
		QSpinBox *exponentEdit, *priorityEdit, *mbunEdit;
		QPlainTextEdit *descriptionEdit;
		QPushButton *okButton;
		QTabWidget *tabs;
		QLabel *relationLabel, *baseLabel, *inverseLabel, *priorityLabel, *mbunLabel, *exponentLabel;
		NamesEditDialog *namesEditDialog;
		Unit *o_unit;
		bool name_edited;

	protected slots:

		void onNameEdited(const QString&);
		void relationChanged(const QString&);
		void systemChanged(const QString&);
		void onUnitChanged();
		void editNames();
		void typeChanged(int);
		void exponentChanged(int);

	public:

		UnitEditDialog(QWidget *parent = NULL);
		virtual ~UnitEditDialog();

		Unit *createUnit(ExpressionItem **replaced_item = NULL);
		Unit *modifyUnit(Unit *u, ExpressionItem **replaced_item = NULL);
		void setUnit(Unit *u);
		void setName(const QString&);

		static Unit *editUnit(QWidget *parent, Unit *u, ExpressionItem **replaced_item = NULL);
		static Unit *newUnit(QWidget *parent, ExpressionItem **replaced_item = NULL);

};

#endif //UNIT_EDIT_DIALOG_H
