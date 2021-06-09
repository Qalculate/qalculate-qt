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

extern QalculateQtSettings *settings;

HistoryView::HistoryView(QWidget *parent) : QTextBrowser(parent) {
}
HistoryView::~HistoryView() {}

void HistoryView::addResult(std::vector<std::string> values, std::string expression, bool exact) {
	QString str;
	if(!expression.empty()) {
		str += "<div align=\"left\">";
		str += QString::fromStdString(expression);
		str += "</div>";
	}
	if(CALCULATOR->message()) {
		do {
			MessageType mtype = CALCULATOR->message()->type();
			str += "<div align=\"left\"><font size=\"-1\"";
			if(mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) {
				str += " color=\"";
				if(mtype == MESSAGE_ERROR) {
					if(settings->color == 1) str += "#800000";
					else str += "#FFAAAA";
				} else {
					if(settings->color == 1) str += "#000080";
					else str += "#AAAAFF";
				}
				str += "\"";
			}
			str += ">";
			QString mstr = QString::fromStdString(CALCULATOR->message()->message());
			mstr.replace("\n", "<br>");
			if(!mstr.startsWith("-")) str += "- ";
			str += mstr;
			str += "</font>";
			str += "</div>";
		} while(CALCULATOR->nextMessage());
	}
	for(size_t i = 0; i < values.size(); i++) {
		str += "<div align=\"right\">";
		if(exact || i < values.size() - 1) str += "= ";
		else str += SIGN_ALMOST_EQUAL " ";
		str += QString::fromStdString(values[i]);
		str += "</div>";
	}
	if(!s_text.isEmpty() && !expression.empty()) str += "<hr>";
	if(expression.empty()) s_text += str;
	else s_text.insert(0, str);
	setHtml(s_text);
}

