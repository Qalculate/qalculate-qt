/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef ITEM_PROXY_MODEL_H
#define ITEM_PROXY_MODEL_H

#include <QSortFilterProxyModel>

#define ADD_INACTIVE_CATEGORY \
	QList<QTreeWidgetItem*> list = categoriesView->findItems("Inactive", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);\
	if(list.isEmpty()) {\
		QStringList l; l << tr("Inactive"); l << "Inactive";\
	}

class ItemProxyModel : public QSortFilterProxyModel {

	Q_OBJECT

	public:

		ItemProxyModel(QObject *parent = NULL);
		~ItemProxyModel();

		void setFilter(std::string, std::string = "");
		void setSecondaryFilter(std::string);
		std::string currentFilter() const;
		std::string currentSecondaryFilter() const;

	protected:

		std::string cat, subcat, filter;

		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

};

#endif //ITEM_PROXY_MODEL_H
