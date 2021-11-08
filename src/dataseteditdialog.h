/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef DATASETEDIT_DIALOG_H
#define DATASETEDIT_DIALOG_H

#include <QDialog>
#include <QVector>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QPlainTextEdit;
class QTabWidget;
class NamesEditDialog;
class QPushButton;
class QComboBox;

class DataSetEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QTabWidget *tabs;
		QLineEdit *nameEdit, *titleEdit, *fileEdit;
		QPlainTextEdit *descriptionEdit, *copyrightEdit;
		QPushButton *okButton;
		NamesEditDialog *namesEditDialog;
		DataSet *o_dataset;
		bool name_edited;

	protected slots:

		void onDatasetChanged();
		void onNameEdited(const QString&);
		void editNames();

	public:

		DataSetEditDialog(QWidget *parent = NULL);
		virtual ~DataSetEditDialog();

};

class DataObjectEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QVector<QLineEdit*> valueEdit;
		QVector<QComboBox*> approxCombo;
		QPushButton *okButton;
		DataSet *ds;

	protected slots:

		void onObjectChanged();

	public:

		DataObjectEditDialog(DataSet*, QWidget *parent = NULL);
		virtual ~DataObjectEditDialog();

		DataObject *createObject();
		bool modifyObject(DataObject*);
		void setObject(DataObject*);

		static bool editObject(QWidget *parent, DataObject*);
		static DataObject *newObject(QWidget *parent, DataSet*);

};

#endif //DATASETEDIT_DIALOG_H

