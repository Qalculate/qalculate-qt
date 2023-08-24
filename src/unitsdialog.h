/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef UNITS_DIALOG_H
#define UNITS_DIALOG_H

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
class QAbstractButton;

class UnitsDialog : public QDialog {

	Q_OBJECT

	protected:

		QTreeView *unitsView;
		QTreeWidget *categoriesView;
		QTextEdit *descriptionView;
		ItemProxyModel *unitsModel, *toModel;
		QStandardItemModel *sourceModel, *toSourceModel;
		QPushButton *deactivateButton, *insertButton, *delButton, *editButton, *newButton, *convertButton;
		QAbstractButton *favouriteButton;
		QLineEdit *searchEdit, *fromEdit, *toEdit;
		QSplitter *vsplitter, *hsplitter;
		QLabel *fromLabel, *equalsLabel;
		QComboBox *toCombo;
		bool last_from;

		std::string selected_category;
		ExpressionItem *selected_item;

		void keyPressEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent*) override;
		void convert(bool from);

	protected slots:

		void selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*);
		void selectedUnitChanged(const QModelIndex&, const QModelIndex&);

		void newClicked();
		void editClicked();
		void delClicked();
		void insertClicked();
		void convertClicked();
		void deactivateClicked();
		void searchChanged(const QString&);
		void fromChanged();
		void toChanged();
		void toUnitChanged();
		void fromUnitChanged();
		void favouriteClicked();
		void onUnitActivated(const QModelIndex&);

	public:

		UnitsDialog(QWidget *parent = NULL);
		virtual ~UnitsDialog();

		void updateUnits();
		void setSearch(const QString&);
		void selectCategory(std::string);
		void unitRemoved(Unit*);
		void unitDeactivated(Unit*);
		bool eventFilter(QObject*, QEvent*) override;

	public slots:

		void reject() override;

	signals:

		void itemsChanged();
		void favouritesChanged();
		void variableRemoved(Variable*);
		void variableDeactivated(Variable*);
		void insertUnitRequest(Unit*);
		void convertToUnitRequest(Unit*);
		void unitActivated(Unit*);

};

#endif //UNITS_DIALOG_H

