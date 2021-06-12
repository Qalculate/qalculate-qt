/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QApplication>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QDebug>

#include <libqalculate/qalculate.h>

#include "historyview.h"
#include "qalculateqtsettings.h"

HistoryView::HistoryView(QWidget *parent) : QTextBrowser(parent), i_pos(0) {
	setOpenLinks(false);
}
HistoryView::~HistoryView() {}

void HistoryView::addResult(std::vector<std::string> values, std::string expression, bool exact, bool dual_approx) {
	QFontMetrics fm(font());
	int w = fm.ascent();
	QString str;
	if(!expression.empty()) {
		str += "<div align=\"left\">";
		if(settings->color == 1) str += QString("<a href=\"%1\"><img src=\":/icons/dark/actions/scalable/edit-paste.svg\" height=\"%2\"/></a> ").arg(v_text.size()).arg(w);
		else str += QString("<a href=\"%1\"><img src=\":/icons/dark/actions/scalable/edit-paste.svg\" height=\"%2\"/></a> ").arg(v_text.size()).arg(w);
		str += QString::fromStdString(expression);
		str += "</div>";
		v_text.push_back(expression);
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
			if(!mstr.startsWith("-")) str += "- ";
			str += mstr;
			str += "</font>";
			str += "</div>";
		} while(CALCULATOR->nextMessage());
		if(str.isEmpty() && values.empty() && expression.empty()) return;
	}
	for(size_t i = 0; i < values.size(); i++) {
		str += "<div align=\"right\">";
		if(exact || i < values.size() - 1) str += "= ";
		else str += SIGN_ALMOST_EQUAL " ";
		str += QString::fromStdString(values[i]);
		str += " ";
		if(settings->color == 1) str += QString("<a href=\"#%1\"><img src=\":/icons/dark/actions/scalable/edit-paste.svg\" height=\"%2\"/></a>").arg(dual_approx && i == 0 ? settings->history_answer.size() - 1 : settings->history_answer.size()).arg(w);
		else str += QString("<a href=\"#%1\"><img src=\":/icons/dark/actions/scalable/edit-paste.svg\" height=\"%2\"/></a>").arg(dual_approx && i == 0 ? settings->history_answer.size() - 1 : settings->history_answer.size()).arg(w);
		str += "</div>";
		v_text.push_back(values[i]);
	}
	str.replace("\n", "<br>");
	if(expression.empty()) {
		s_text.insert(i_pos, str);
		i_pos += str.length();
	} else {
		i_pos = str.length();
		if(!s_text.isEmpty()) str += "<hr/>";
		s_text.insert(0, str);
	}
	setHtml(s_text);
}
void HistoryView::addMessages() {
	std::vector<std::string> values;
	addResult(values, "", true);
}
void HistoryView::mouseReleaseEvent(QMouseEvent *e) {
	QString str = anchorAt(e->pos());
	if(!str.isEmpty()) {
		if(str[0] == '#') {
			emit insertValueRequested(str.right(1).toInt());
		} else {
			int i = str.toInt();
			if(i >= 0 && (size_t) i < v_text.size()) emit insertTextRequested(v_text[str.toInt()]);
		}
	} else {
		QTextBrowser::mouseReleaseEvent(e);
	}
}

