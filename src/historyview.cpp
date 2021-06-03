/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QDebug>

#include <libqalculate/qalculate.h>

#include "historyview.h"
#include "qalculateqtsettings.h"

HistoryView::HistoryView(QWidget *parent) : QTextBrowser(parent) {
}
HistoryView::~HistoryView() {}

void HistoryView::addResult(std::vector<std::string> values, std::string expression, bool exact) {
	QString str = "<div align=\"left\">";
	str += QString::fromStdString(expression);
	str += "</div>";
	for(size_t i = 0; i < values.size(); i++) {
		str += "<div align=\"right\">";
		if(exact || i < values.size() - 1) str += "= ";
		else str += SIGN_ALMOST_EQUAL " ";
		qDebug() << QString::fromStdString(values[i]);
		str += QString::fromStdString(values[i]);
		str += "</div>";
	}
	if(!s_text.isEmpty()) str += "<hr>";
	s_text.insert(0, str);
	setHtml(s_text);
}
