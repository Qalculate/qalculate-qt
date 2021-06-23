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
class QComboBox;
class QLabel;

class FunctionsDialog : public QDialog {

	Q_OBJECT

	protected:

		QTreeView *functionsView;
		QTreeWidget *categoriesView;
		QTextEdit *descriptionView;
		ItemProxyModel *functionsModel;
		QStandardItemModel *sourceModel;
		QPushButton *deactivateButton, *calculateButton, *insertButton, *delButton, *editButton, *newButton, *applyButton;
		QLineEdit *searchEdit;
		QSplitter *vsplitter, *hsplitter;

		std::string selected_category;
		ExpressionItem *selected_item;

		void keyPressEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent*) override;

	protected slots:

		void selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void selectedFunctionChanged(const QModelIndex&, const QModelIndex&);

		void newClicked();
		void editClicked();
		void delClicked();
		void applyClicked();
		void insertClicked();
		void calculateClicked();
		void deactivateClicked();
		void searchChanged(const QString&);

	public:

		FunctionsDialog(QWidget *parent = NULL);
		virtual ~FunctionsDialog();

		void updateFunctions();
		void setSearch(const QString&);
		void selectCategory(std::string);

	public slots:

		void reject() override;

	signals:

		void itemsChanged();
		void applyFunctionRequest(MathFunction*);
		void insertFunctionRequest(MathFunction*);
		void calculateFunctionRequest(MathFunction*);

};

#endif //FUNCTIONS_DIALOG_H

