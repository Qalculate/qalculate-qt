/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QDebug>

#include "itemproxymodel.h"

#include <libqalculate/qalculate.h>
#include "qalculateqtsettings.h"

ItemProxyModel::ItemProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {
	setSortCaseSensitivity(Qt::CaseInsensitive);
	setSortLocaleAware(true);
	setDynamicSortFilter(false);
}
ItemProxyModel::~ItemProxyModel() {}

bool ItemProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const {
	QModelIndex index = sourceModel()->index(source_row, 0);
	if(!index.isValid()) return false;
	ExpressionItem *item = (ExpressionItem*) index.data(Qt::UserRole).value<void*>();
	if(cat.empty()) return false;
	if(cat == "All") {
		if(!item->isActive()) return false;
	} else if(cat == "Inactive") {
		if(item->isActive()) return false;
	} else if(cat == "Uncategorized") {
		if(!item->isActive() || !item->category().empty()) return false;
	} else if(cat == "User items") {
		if(!item->isActive() || !item->isLocal()) return false;
	} else if(cat == "Favorites") {
		bool b = false;
		if(item->type() == TYPE_UNIT) {
			for(size_t i = 0; i < settings->favourite_units.size(); i++) {
				if(settings->favourite_units[i] == item) {b = true; break;}
			}
		}
		if(item->type() == TYPE_FUNCTION) {
			for(size_t i = 0; i < settings->favourite_functions.size(); i++) {
				if(settings->favourite_functions[i] == item) {b = true; break;}
			}
		}
		if(item->type() == TYPE_VARIABLE) {
			for(size_t i = 0; i < settings->favourite_variables.size(); i++) {
				if(settings->favourite_variables[i] == item) {b = true; break;}
			}
		}
		if(!b) return false;
	} else {
		if(!item->isActive()) return false;
		if(!subcat.empty()) {
			size_t l1 = subcat.length(), l2;
			l2 = item->category().length();
			if((l2 != l1 && (l2 <= l1 || item->category()[l1] != '/')) || item->category().substr(0, l1) != subcat) return false;
		} else {
			if(item->category() != cat) return false;
		}
	}
	if(filter.empty()) return true;
	if(item->type() == TYPE_UNIT) {
		return name_matches(item, filter) || title_matches(item, filter) || country_matches((Unit*) item, filter);
	}
	std::string title = item->title(true, settings->printops.use_unicode_signs);
	remove_blank_ends(title);
	while(title.length() >= filter.length()) {
		if(equalsIgnoreCase(filter, title.substr(0, filter.length()))) {
			return true;
		}
		size_t i = title.find(' ');
		if(i == std::string::npos) break;
		title = title.substr(i + 1);
		remove_blank_ends(title);
	}
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		if(item->getName(i2).case_sensitive) {
			if(filter == item->getName(i2).name.substr(0, filter.length())) return true;
		} else {
			if(equalsIgnoreCase(filter, item->getName(i2).name.substr(0, filter.length()))) return true;
		}
	}
	return false;
}
void ItemProxyModel::setFilter(std::string scat, std::string sfilter) {
	remove_blank_ends(sfilter);
	if(cat != scat || filter != sfilter) {
		cat = scat;
		if(cat[0] == '/') subcat = cat.substr(1, cat.length() - 1);
		else subcat = "";
		filter = sfilter;
		invalidateFilter();
	}
}
void ItemProxyModel::setSecondaryFilter(std::string sfilter) {
	remove_blank_ends(sfilter);
	if(filter != sfilter) {
		filter = sfilter;
		invalidateFilter();
	}
}
std::string ItemProxyModel::currentFilter() const {
	return cat;
}
std::string ItemProxyModel::currentSecondaryFilter() const {
	return filter;
}

