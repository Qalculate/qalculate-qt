/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FUNCTIONS_DIALOG_H
#define FUNCTIONS_DIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include <libqalculate/qalculate.h>

class QTreeView;
class QTreeWidget;
class QTextBrowser;
class QTreeWidgetItem;
class QStandardItemModel;
class QPushButton;

class ItemProxyModel : public QSortFilterProxyModel {

	Q_OBJECT

	public:

		ItemProxyModel(QObject *parent = NULL);
		~ItemProxyModel();

		void setFilter(std::string);

	protected:

		std::string cat, subcat;

		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

};

class FunctionsDialog : public QDialog {

	Q_OBJECT

	protected:

		QTreeView *functionsView;
		QTreeWidget *categoriesView;
		QTextBrowser *descriptionView;
		ItemProxyModel *functionsModel;
		QStandardItemModel *sourceModel;
		QPushButton *deactivateButton, *insertButton, *delButton, *editButton, *newButton, *applyButton;

		std::string selected_category;
		ExpressionItem *selected_item;

	protected slots:

		void selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void selectedFunctionChanged(const QModelIndex&, const QModelIndex&);

		void newClicked();
		void editClicked();
		void delClicked();
		void applyClicked();
		void insertClicked();
		void deactivateClicked();

	public:

		FunctionsDialog(QWidget *parent = NULL);
		virtual ~FunctionsDialog();

		void updateFunctions();

	signals:

		void itemsChanged();
		void applyFunctionRequest(MathFunction*);

};

#endif //FUNCTIONS_DIALOG_H

