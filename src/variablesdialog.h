/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef VARIABLES_DIALOG_H
#define VARIABLES_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QTreeView;
class QTreeWidget;
class QTextEdit;
class QTreeWidgetItem;
class QStandardItemModel;
class QPushButton;
class QLineEdit;
class QSplitter;
class ItemProxyModel;

class VariablesDialog : public QDialog {

	Q_OBJECT

	protected:

		QTreeView *variablesView;
		QTreeWidget *categoriesView;
		QTextEdit *descriptionView;
		ItemProxyModel *variablesModel;
		QStandardItemModel *sourceModel;
		QPushButton *deactivateButton, *insertButton, *delButton, *editButton, *newButton;
		QLineEdit *searchEdit;
		QSplitter *vsplitter, *hsplitter;

		std::string selected_category;
		ExpressionItem *selected_item;

		void keyPressEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent*) override;
		void newVariable(int);

	protected slots:

		void selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void selectedVariableChanged(const QModelIndex&, const QModelIndex&);

		void newVariable();
		void newMatrix();
		void newUnknown();
		void editClicked();
		void delClicked();
		void insertClicked();
		void deactivateClicked();
		void searchChanged(const QString&);

	public:

		VariablesDialog(QWidget *parent = NULL);
		virtual ~VariablesDialog();

		void updateVariables();
		void setSearch(const QString&);
		void selectCategory(std::string);
		void variableRemoved(Variable*);

	public slots:

		void reject() override;

	signals:

		void itemsChanged();
		void unitRemoved(Unit*);
		void applyVariableRequest(Variable*);
		void insertVariableRequest(Variable*);

};

#endif //VARIABLES_DIALOG_H

