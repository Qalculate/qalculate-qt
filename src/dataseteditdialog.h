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
class QCheckBox;
class QTreeWidget;
class QTreeWidgetItem;

class DataSetEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QTabWidget *tabs;
		QLineEdit *nameEdit, *titleEdit, *fileEdit, *arg1Edit, *arg2Edit, *default2Edit;
		QPlainTextEdit *descriptionEdit, *copyrightEdit;
		QTreeWidget *propertiesView;
		QPushButton *okButton, *addButton, *delButton, *editButton;
		NamesEditDialog *namesEditDialog;
		DataSet *o_dataset;
		std::vector<DataProperty*> tmp_props;
		std::vector<DataProperty*> tmp_props_orig;
		bool name_edited;
		DataProperty *selected_property = NULL;

	protected slots:

		void onDatasetChanged();
		void onNameEdited(const QString&);
		void editNames();
		void addProperty();
		void editProperty();
		void delProperty();
		void selectedPropertyChanged(QTreeWidgetItem*, QTreeWidgetItem*);

	public:

		DataSetEditDialog(QWidget *parent = NULL);
		virtual ~DataSetEditDialog();

		DataSet *createDataset(MathFunction **replaced_item = NULL);
		bool modifyDataset(DataSet *ds, MathFunction **replaced_item = NULL);
		void setDataset(DataSet *ds);

		static bool editDataset(QWidget *parent, DataSet *ds, MathFunction **replaced_item = NULL);
		static DataSet *newDataset(QWidget *parent, MathFunction **replaced_item = NULL);

};

class DataPropertyEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QPushButton *okButton;
		QLineEdit *nameEdit, *titleEdit, *unitEdit;
		QComboBox *typeCombo;
		QPlainTextEdit *descriptionEdit;
		QCheckBox *hideBox, *approxBox, *bracketsBox, *keyBox, *caseBox;
		NamesEditDialog *namesEditDialog;
		DataProperty *o_property;
		bool name_edited;

	protected slots:

		void onPropertyChanged();
		void editNames();

	public:

		DataPropertyEditDialog(QWidget *parent = NULL);
		virtual ~DataPropertyEditDialog();

		DataProperty *createProperty(DataSet *ds);
		bool modifyProperty(DataProperty *dp);
		void setProperty(DataProperty *dp);

		static bool editProperty(QWidget *parent, DataProperty *dp);
		static DataProperty *newProperty(QWidget *parent, DataSet *ds);

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

