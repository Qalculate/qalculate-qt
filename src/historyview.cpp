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
#include <QContextMenuEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QKeySequence>
#include <QClipboard>
#include <QTextDocumentFragment>
#include <QApplication>
#include <QDebug>

#include <libqalculate/qalculate.h>

#include "historyview.h"
#include "qalculateqtsettings.h"

HistoryView::HistoryView(QWidget *parent) : QTextBrowser(parent), i_pos(0) {
	setOpenLinks(false);
	cmenu = NULL;
}
HistoryView::~HistoryView() {}

void HistoryView::addResult(std::vector<std::string> values, std::string expression, bool exact, bool dual_approx) {
	QFontMetrics fm(font());
	int w = fm.ascent();
	QString str;
	if(!expression.empty()) {
		str += "<div align=\"left\">";
		if(settings->color == 1) str += QString("<a href=\"%1\"><img src=\":/icons/actions/scalable/edit-paste.svg\" height=\"%2\"/></a> ").arg(v_text.size()).arg(w);
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
		if(settings->color == 1) str += QString("<a href=\"#%1\"><img src=\":/icons/actions/scalable/edit-paste.svg\" height=\"%2\"/></a>").arg(dual_approx && i == 0 ? settings->history_answer.size() - 1 : settings->history_answer.size()).arg(w);
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
void HistoryView::keyPressEvent(QKeyEvent *e) {
	if(e->matches(QKeySequence::Copy)) {
		editCopy();
		return;
	}
	QTextBrowser::keyPressEvent(e);
}
void HistoryView::contextMenuEvent(QContextMenuEvent *e) {
	if(!cmenu) {
		cmenu = new QMenu(this);
		copyAction = cmenu->addAction(tr("Copy"), this, SLOT(editCopy()));
		copyAction->setShortcut(QKeySequence::Copy);
		copyAction->setShortcutContext(Qt::WidgetShortcut);
		selectAllAction = cmenu->addAction(tr("Select All"), this, SLOT(selectAll()));
		selectAllAction->setShortcut(QKeySequence::SelectAll);
		selectAllAction->setShortcutContext(Qt::WidgetShortcut);
	}
	copyAction->setEnabled(textCursor().hasSelection());
	selectAllAction->setEnabled(!toPlainText().isEmpty());
	cmenu->popup(e->globalPos());
}
QString unhtmlize(QString str) {
	int i = 0, i2;
	while(true) {
		i = str.indexOf("<", i);
		if(i < 0) break;
		i2 = str.indexOf(">", i + 1);
		if(i2 < 0) break;
		int i_sup = str.indexOf("vertical-align:super", i);
		int i_sub = str.indexOf("vertical-align:sub", i);
		if(i_sup > i && i2 > i_sup) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				QString str2 = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1));
				if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i, SIGN_POWER_2);
				else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i, SIGN_POWER_3);
				else str.replace(i, i3 - i, "^(" + str2 + ")");
				continue;
			}
		} else if(i_sub > i && i2 > i_sub) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				str.replace(i, i3 - i, "_" + unhtmlize(str.mid(i2 + 1, i3 - i2 - 1)));
				continue;
			}
		} else if(i2 - i == 4) {
			if(str.mid(i + 1, 3) == "sup") {
				int i3 = str.indexOf("</sup>", i2 + 1);
				if(i3 >= 0) {
					QString str2 = unhtmlize(str.mid(i + 5, i3 - i - 5));
					if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else str.replace(i, i3 - i + 6, "^(" + str2 + ")");
					continue;
				}
			} else if(str.mid(i + 1, 3) == "sub") {
				int i3 = str.indexOf("</sub>", i + 4);
				if(i3 >= 0) {
					str.replace(i, i3 - i + 6, "_" + unhtmlize(str.mid(i + 5, i3 - i - 5)));
					continue;
				}
			}
		} else if(i2 - i == 2 && str[i + 1] == 'i') {
			int i3 = str.indexOf("</i>", i2 + 1);
			if(i3 >= 0) {
				QString name = unhtmlize(str.mid(i + 3, i3 - i - 3));
				Variable *v = CALCULATOR->getActiveVariable(name.toStdString());
				bool replace_all_i = (!v || v->isKnown());
				if(replace_all_i && name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, "\\");
				} else if(replace_all_i) {
					name.insert(0, "\"");
					name += "\"";
				}
				str.replace(i, i3 - i + 4, name);
				continue;
			}
		}
		str.remove(i, i2 - i + 1);
	}
	str.replace(" " SIGN_DIVISION_SLASH " ", "/");
	str.replace("&amp;", "&");
	str.replace("&gt;", ">");
	str.replace("&lt;", "<");
	return str;
}
void HistoryView::editCopy() {
	QString str = textCursor().selection().toHtml();
	int i = str.indexOf("<!--StartFragment-->");
	int i2 = str.indexOf("<!--EndFragment-->", i + 20);
	if(i >= 0 && i2 >= 0) str = str.mid(i + 20, i2 - i - 20).trimmed();
	else str = textCursor().selectedText();
	QApplication::clipboard()->setText(unhtmlize(str));
}

