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

class ItemProxyModel : public QSortFilterProxyModel {

	Q_OBJECT

	public:

		ItemProxyModel(QObject *parent = NULL);
		~ItemProxyModel();

		void setFilter(std::string, std::string = "");
		void setSecondaryFilter(std::string);
		void setShowHidden(bool);
		std::string currentFilter() const;
		std::string currentSecondaryFilter() const;

	protected:

		std::string cat, subcat, filter;
		bool show_hidden;

		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

};

#endif //ITEM_PROXY_MODEL_H
