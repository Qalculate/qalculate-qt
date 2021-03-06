/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef DATASETS_DIALOG_H
#define DATASETS_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QTreeWidget;
class QTextEdit;
class QTreeWidgetItem;
class QPushButton;
class QSplitter;

class DataSetsDialog : public QDialog {

	Q_OBJECT

	protected:

		QTreeWidget *datasetsView, *objectsView, *propertiesView;
		QTextEdit *descriptionView;
		QPushButton *addDSButton, *delDSButton, *editDSButton, *addObjButton, *delObjButton, *editObjButton;
		QSplitter *vsplitter_l, *vsplitter_r, *hsplitter;
		DataSet *selected_dataset;
		DataObject *selected_object;

		void closeEvent(QCloseEvent*) override;

	protected slots:

		void selectedDatasetChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void selectedObjectChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void propertyDoubleClicked(QTreeWidgetItem*, int);
		void propertyClicked(QTreeWidgetItem*, int);
		void vsplitterrMoved(int, int);
		void vsplitterlMoved(int, int);
		void addDataset();
		void editDataset();
		void delDataset();
		void addObject();
		void editObject();
		void delObject();

	public:

		DataSetsDialog(QWidget *parent = NULL);
		virtual ~DataSetsDialog();

		void updateDatasets();

	public slots:

		void reject() override;

	signals:

		void itemsChanged();
		void insertPropertyRequest(DataObject*, DataProperty*);

};

#endif //DATASETS_DIALOG_H

