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
#include <QScrollBar>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDialog>
#include <QDebug>

#include <libqalculate/qalculate.h>

#include "expressionedit.h"
#include "historyview.h"
#include "qalculateqtsettings.h"

bool qstring_has_nondigit(const QString &str) {
	for(int i = 0; i < str.length(); i++) {
		if(!str[i].isDigit()) return true;
	}
	return false;
}

QString unhtmlize(QString str) {
	int i = 0, i2;
	str.remove("\n");
	while(true) {
		i = str.indexOf("<", i);
		if(i < 0) break;
		i2 = str.indexOf(">", i + 1);
		if(i2 < 0) break;
		int i_sup = str.indexOf("vertical-align:super", i);
		int i_sub = str.indexOf("vertical-align:sub", i);
		int i_small = str.indexOf("font-size:small", i);
		int i_italic = str.indexOf("font-style:italic", i);
		if(i_sup > i && i2 > i_sup) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				QString str2 = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1));
				if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i, SIGN_POWER_2);
				else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i, SIGN_POWER_3);
				else if(str.length() == i3 + 7 && (str2.length() == 1 || !qstring_has_nondigit(str2))) str.replace(i, i3 - i, "^" + str2);
				else str.replace(i, i3 - i, "^(" + str2 + ")");
				continue;
			}
		} else if(i_sub > i && i2 > i_sub) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				if(i_small > i && i2 > i_small) str.remove(i, i3 - i);
				else str.replace(i, i3 - i, "_" + unhtmlize(str.mid(i2 + 1, i3 - i2 - 1)));
				continue;
			}
		} else if(i_italic > i && i2 > i_italic) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				QString name = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1));
				Variable *v = CALCULATOR->getActiveVariable(name.toStdString());
				bool replace_all_i = (!v || v->isKnown());
				if(replace_all_i && name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, "\\");
				} else if(replace_all_i) {
					name.insert(0, "\"");
					name += "\"";
				}
				str.replace(i, i3 - i, name);
				continue;
			}
		} else if((i2 - i == 3 && str.mid(i + 1, 2) == "br") || (i2 - i == 4 && str.mid(i + 1, 3) == "/tr")) {
			str.replace(i, i2 - i + 1, "\n");
			continue;
		} else if(i2 - i > 5 && str.mid(i + 1, 4) == "span" && i2 + 1 < str.length() && str[i2 + 1] == '#') {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0 && str.indexOf("</span></a>") == i3) {
				str.remove(i, i3 - i);
				continue;
			}
		} else if(i2 - i == 4) {
			if(str.mid(i + 1, 3) == "sup") {
				int i3 = str.indexOf("</sup>", i2 + 1);
				if(i3 >= 0) {
					QString str2 = unhtmlize(str.mid(i + 5, i3 - i - 5));
					if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else if(str.length() == i3 + 6 && (str2.length() == 1 || !qstring_has_nondigit(str2))) str.replace(i, i3 - i + 6, "^" + str2);
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
		} else if(i2 - i == 17 && str.mid(i + 1, 16) == "i class=\"symbol\"") {
			int i3 = str.indexOf("</i>", i2 + 1);
			if(i3 >= 0) {
				QString name = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1));
				if(name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, "\\");
				} else {
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
	str.replace("&nbsp;", " ");
	str.replace("&quot;", "\"");
	return str;
}

HistoryView::HistoryView(QWidget *parent) : QTextEdit(parent), i_pos(0) {
	i_pos = 0;
	last_ans = 0;
	setReadOnly(true);
	QImage img1px(1, 1, QImage::Format_ARGB32);
	img1px.fill(Qt::transparent);
	document()->addResource(QTextDocument::ImageResource, QUrl("data://img1px.png"), QVariant(img1px));
	setTextColor(palette().text().color());
	prev_color = textColor();
	cmenu = NULL;
}
HistoryView::~HistoryView() {}

void replace_colors(QString &s_text) {
	if(settings->color == 2) {
		s_text.replace("color: #585858", "color: #AAAAAA");
		s_text.replace("color: #800000", "color: #FFAAAA");
		s_text.replace("color: #000080", "color: #AAAAFF");
		s_text.replace("color:#005858", "color:#AAFFFF");
		s_text.replace("color:#585800", "color:#FFFFAA");
		s_text.replace("color:#580058", "color:#FFAAFF");
		s_text.replace("color:#000080", "color:#FFAAAA");
		s_text.replace("color:#008000", "color:#BBFFBB");
		s_text.replace(":/icons/actions", ":/icons/dark/actions");
	} else {
		s_text.replace("color: #AAAAAA", "color: #585858");
		s_text.replace("color: #AAAAFF", "color: #000080");
		s_text.replace("color: #FFAAAA", "color: #800000");
		s_text.replace("color:#AAFFFF", "color:#005858");
		s_text.replace("color:#FFFFAA", "color:#585800");
		s_text.replace("color:#FFAAFF", "color:#580058");
		s_text.replace("color:#FFAAAA", "color:#000080");
		s_text.replace("color:#BBFFBB", "color:#008000");
		s_text.replace(":/icons/dark/actions", ":/icons/actions");
	}
}
void replace_colors(std::string &str) {
	if(settings->color == 2) {
		gsub("color:#005858", "color:#AAFFFF", str);
		gsub("color:#585800", "color:#FFFFAA", str);
		gsub("color:#580058", "color:#FFAAFF", str);
		gsub("color:#000080", "color:#FFAAAA", str);
		gsub("color:#008000", "color:#BBFFBB", str);
		gsub(":/icons/actions", ":/icons/dark/actions", str);
	} else {
		gsub("color:#AAFFFF", "color:#005858", str);
		gsub("color:#FFFFAA", "color:#585800", str);
		gsub("color:#FFAAFF", "color:#580058", str);
		gsub("color:#FFAAAA", "color:#000080", str);
		gsub("color:#BBFFBB", "color:#008000", str);
		gsub(":/icons/dark/actions", ":/icons/actions", str);
	}
}

void replace_one(QString &str, const QString &origstr, const QString &newstr, bool b_last = false) {
	int index = (b_last ? str.lastIndexOf(origstr) : str.indexOf(origstr));
	if(index > 0) {
		str.replace(index, origstr.length(), newstr);
	}
}

void remove_top_border(QString &s_text) {
	if(!s_text.isEmpty() && s_text.indexOf("border-top: 0px none") < 0) {
		replace_one(s_text, "border-top: 1px dashed", "border-top: 0px none");
		int index_a = s_text.indexOf("padding-top: ") + strlen("padding-top: ");
		int index_b = s_text.indexOf("px", index_a);
		if(index_b >= 0) {
			s_text.replace(index_a, index_b - index_a, "0");
		}
	}
}

void HistoryView::loadInitial() {
	s_text.clear();
	i_pos = 0;
	last_ans = 0;
	last_ref = "";
	if(!settings->v_expression.empty()) {
		for(size_t i = 0; i < settings->v_expression.size(); i++) {
			addResult(settings->v_result[i], settings->v_expression[i], true, false, QString(), NULL, true, i);
		}
	}
	if(!s_text.isEmpty()) {
		if((settings->color == 2 && (s_text.contains("color:#00") || s_text.contains("color:#58"))) || (settings->color != 2 && (s_text.contains("color:#FF") || s_text.contains("color:#AA")))) {
			replace_colors(s_text);
			for(size_t i = 0; i < settings->v_expression.size(); i++) {
				replace_colors(settings->v_expression[i]);
				for(size_t i2 = 0; i2 < settings->v_result[i].size(); i2++) {
					replace_colors(settings->v_result[i][i2]);
				}
			}
		}
		setHtml("<body color=\"" + textColor().name() + "\"><table width=\"100%\">" + s_text + "</table></body>");
	}
}

#define PASTE_H QFontMetrics fm(font()); \
	int paste_h = fm.ascent(); \
	if(paste_h < 16) {paste_h = 12;} \
	else if(paste_h < 24) {paste_h = 16;} \
	else {paste_h = 24;} \

void HistoryView::addResult(std::vector<std::string> values, std::string expression, int exact, bool dual_approx, const QString &image, bool *implicit_warning, bool initial_load, size_t index) {
	PASTE_H
	QString str;
	if(!expression.empty()) {
		str += QString("<tr><td colspan=\"2\" style=\"padding-bottom: %2 px; padding-top: 0px; border-top: 0px none %3; text-align:left\"><a name=\"%1\" style=\"text-decoration: none\">").arg(initial_load ? (int) index : settings->v_expression.size()).arg(paste_h / 4).arg(textColor().name());
		str += QString::fromStdString(expression);
		str += "</a></td></tr>";
		if(!initial_load) {
			settings->v_expression.push_back(expression);
			settings->v_protected.push_back(false);
			settings->v_delexpression.push_back(false);
			settings->v_result.push_back(values);
			settings->v_exact.push_back(std::vector<int>());
			settings->v_delresult.push_back(std::vector<bool>());
			for(size_t i = 0; i < values.size(); i++) {
				settings->v_exact[settings->v_exact.size() - 1].push_back(exact || i < values.size() - 1);
				settings->v_delresult[settings->v_delresult.size() - 1].push_back(false);
			}
		}
	} else if(!initial_load && !settings->v_result.empty()) {
		for(size_t i = values.size(); i > 0; i--) {
			settings->v_result[settings->v_result.size() - 1].insert(settings->v_result[settings->v_result.size() - 1].begin(), values[i - 1]);
			settings->v_exact[settings->v_exact.size() - 1].insert(settings->v_exact[settings->v_exact.size() - 1].begin(), exact || i < values.size());
			settings->v_delresult[settings->v_delresult.size() - 1].insert(settings->v_delresult[settings->v_delresult.size() - 1].begin(), false);
		}
	}
	if(!initial_load && CALCULATOR->message()) {
		do {
			if(CALCULATOR->message()->category() == MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION && (settings->implicit_question_asked || implicit_warning)) {
				if(!settings->implicit_question_asked) *implicit_warning = true;
			} else {
				MessageType mtype = CALCULATOR->message()->type();
				str += "<tr><td class=\"message\" colspan=\"2\" style=\"text-align:left; font-size:normal";
				if(mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) {
					str += "; color: ";
					if(mtype == MESSAGE_ERROR) {
						if(settings->color == 2) str += "#FFAAAA";
						else str += "#800000";
					} else {
						if(settings->color == 2) str += "#AAAAFF";
						else str += "#000080";
					}
					str += "";
				}
				str += "\">";
				QString mstr = QString::fromStdString(CALCULATOR->message()->message());
				if(!mstr.startsWith("-")) str += "- ";
				if(settings->printops.use_unicode_signs) {
					mstr.replace(">=", SIGN_GREATER_OR_EQUAL);
					mstr.replace("<=", SIGN_LESS_OR_EQUAL);
					mstr.replace("!=", SIGN_NOT_EQUAL);
				}
				str += mstr.toHtmlEscaped();
				str += "</td></tr>";
			}
		} while(CALCULATOR->nextMessage());
		if(str.isEmpty() && values.empty() && expression.empty()) return;
	}
	str.replace("</i>", "<img src=\"data://img1px.png\" width=\"1\"/></i>");
	if(!expression.empty()) i_pos = str.length();
	for(size_t i = 0; i < values.size(); i++) {
		size_t i_answer = -1;
		if(!initial_load) i_answer = dual_approx && i == 0 ? settings->history_answer.size() - 1 : settings->history_answer.size();
		QFontMetrics fm(font());
		int w = fm.boundingRect("#9999").width();
		str += "<tr><td valign=\"center\" width=\""; str += QString::number(w); str += "\">";
		w = fm.boundingRect("#9999" + unhtmlize(QString::fromStdString(values[i]))).width();
		if(!initial_load && (dual_approx || i == 0)) {
			if(expression.empty() && last_ans == i_answer && !last_ref.isEmpty()) s_text.remove(last_ref);
			QString sref;
			if(settings->color == 2) {
				sref = QString("<a href=\"#%1:%2:%3\" style=\"text-decoration: none; text-align:left; color: #AAAAAA\">#%1</a>").arg(i_answer).arg(settings->v_expression.size() - 1).arg(settings->v_result[settings->v_result.size() - 1].size() - i - 1);
			} else {
				sref = QString("<a href=\"#%1:%2:%3\" style=\"text-decoration: none; text-align:left; color: #585858\">#%1</a>").arg(i_answer).arg(settings->v_expression.size() - 1).arg(settings->v_result[settings->v_result.size() - 1].size() - i - 1);
			}
			str += sref;
			if(!dual_approx || i == values.size()) {
				last_ans = i_answer;
				last_ref = sref;
			}
		}
		str += "</td><td style=\"text-align:right";
		if(initial_load || w > width() * 2) {
			gsub("</i>", "<img src=\"data://img1px.png\" width=\"1\"/></i>", values[i]);
		} else if(w * 2 > width()) {
			str += "; font-size:large";
			gsub("</i>", "<img src=\"data://img1px.png\" width=\"2\"/></i>", values[i]);
		} else {
			str += "; font-size:x-large";
			gsub("</i>", "<img src=\"data://img1px.png\" width=\"2\"/></i>", values[i]);
		}
		if(!expression.empty() && i == values.size() - 1) str += QString("; padding-bottom: %1 px").arg(paste_h / 2);
		str += "\">";
		int b_exact = 1;
		if(initial_load) {
			b_exact = settings->v_exact[index][i];
		} else {
			if(exact < 0) b_exact = -1;
			else if(exact == 0 && i == values.size() - 1) b_exact = 0;
		}
		if(initial_load) str += QString("<a name=\"%1:%2\" style=\"text-decoration: none\">").arg((int) index).arg((int) i);
		else str += QString("<a name=\"p%1:%2:%3\" style=\"text-decoration: none\">").arg(i_answer).arg(settings->v_expression.size() - 1).arg(settings->v_result[settings->v_result.size() - 1].size() - i - 1);
		if(b_exact > 0) str += "= ";
		else if(b_exact == 0) str += SIGN_ALMOST_EQUAL " ";
		str += QString::fromStdString(values[i]);
		if(!image.isEmpty() && w * 2 <= width()) str += QString("<img src=\"data://img1px.png\" width=\"2\"/><img valign=\"top\" src=\"%1\"/>").arg(image);
		str += "</a></td></tr>";
	}
	str.replace("\n", "<br>");
	int i = 0;
	if(!initial_load) {
		s_text.replace("font-size:normal", "font-size:small ");
		s_text.replace("width=\"2\"", "width=\"1\"");
		while(true) {
			i = s_text.indexOf("<img valign=\"top\"", i);
			if(i < 0) break;
			int i2 = s_text.indexOf(">", i);
			s_text.remove(i, i2 - i + 1);
			if(i < i_pos) i_pos -= (i2 - i + 1);
		}
	}
	if(expression.empty()) {
		if(!initial_load) {
			i = 0;
			while(true) {
				i = s_text.indexOf("; font-size:x-large", i);
				if(i < 0) break;
				s_text.remove(i, 19);
				if(i < i_pos) i_pos -= 19;
			}
			i = 0;
			while(true) {
				i = s_text.indexOf("; font-size:large", i);
				if(i < 0) break;
				s_text.remove(i, 17);
				if(i < i_pos) i_pos -= 17;
			}
			i = 0;
		}
		s_text.insert(i_pos, str);
	} else {
		if(!initial_load) {
			s_text.remove("; font-size:x-large");
			s_text.remove("; font-size:large");
		}
		replace_one(s_text, "border-top: 0px none", "border-top: 1px dashed");
		replace_one(s_text, "padding-top: 0px", "padding-top: " + QString::number(paste_h / 2) + "px");
		s_text.insert(0, str);
	}
	if(!initial_load) setHtml("<body color=\"" + textColor().name() + "\"><table width=\"100%\">" + s_text + "</table></body>");
}
void HistoryView::changeEvent(QEvent *e) {
	if(e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) {
		setTextColor(palette().text().color());
		for(size_t i = 0; i < settings->v_expression.size(); i++) {
			replace_colors(settings->v_expression[i]);
			for(size_t i2 = 0; i2 < settings->v_result[i].size(); i2++) {
				replace_colors(settings->v_result[i][i2]);
			}
		}
		if(!s_text.isEmpty()) {
			replace_colors(s_text);
			s_text.replace("1px dashed " + prev_color.name(), "1px dashed " + textColor().name());
			s_text.replace("0px none " + prev_color.name(), "0px none " + textColor().name());
			setHtml("<body color=\"" + textColor().name() + "\"><table width=\"100%\">" + s_text + "</table></body>");
		}
		prev_color = textColor();
	}
	QTextEdit::changeEvent(e);
}
void HistoryView::addMessages() {
	std::vector<std::string> values;
	addResult(values, "", true);
}
void HistoryView::mouseMoveEvent(QMouseEvent *e) {
	QString str = anchorAt(e->pos());
	if(str.isEmpty()) viewport()->setCursor(Qt::IBeamCursor);
	else viewport()->setCursor(Qt::PointingHandCursor);
	QTextEdit::mouseMoveEvent(e);
}
void HistoryView::mouseDoubleClickEvent(QMouseEvent *e) {
	QString str = anchorAt(e->pos());
	if(str.isEmpty() && e->button() == Qt::LeftButton) {
		int i1 = -1, i2 = -1;
		indexAtPos(e->pos(), &i1, &i2, NULL, &str);
		if(i1 >= 0 && str.length() > 0) {
			if(str[0] == 'p') {
				int index = str.indexOf(":");
				if(index < 0) return;
				str = str.mid(index + 1);
				index = str.indexOf(":");
				if(index < 0) {
					int i = str.toInt();
					if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i]);
				} else {
					int i1 = str.left(index).toInt();
					int i2 = str.mid(index + 1).toInt();
					if(i1 >= 0 && i1 < (int) settings->v_result.size() && i2 < (int) settings->v_result[i1].size()) {
						emit insertTextRequested(settings->v_result[i1][settings->v_result[i1].size() - i2 - 1]);
					}
				}
			} else if(str[0] == '#') {
				int index = str.indexOf(":");
				if(index < 0) emit insertValueRequested(str.mid(1).toInt());
				else emit insertValueRequested(str.mid(1, index - 1).toInt());
			} else {
				int index = str.indexOf(":");
				if(index < 0) {
					int i = str.toInt();
					if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i]);
				} else {
					int i1 = str.left(index).toInt();
					int i2 = str.mid(index + 1).toInt();
					if(i1 >= 0 && (size_t) i1 < settings->v_result.size() && i2 >= 0 && (size_t) i2 < settings->v_result[i1].size()) emit insertTextRequested(settings->v_result[i1][i2]);
				}
			}
		}
	} else {
		QTextEdit::mouseDoubleClickEvent(e);
	}
}
void HistoryView::mouseReleaseEvent(QMouseEvent *e) {
	QString str = anchorAt(e->pos());
	if(!str.isEmpty() && e->button() == Qt::LeftButton) {
		if(str[0] == 'p') {
			int index = str.indexOf(":");
			if(index < 0) return;
			str = str.mid(index + 1);
			index = str.indexOf(":");
			if(index < 0) {
				int i = str.toInt();
				if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i]);
			} else {
				int i1 = str.left(index).toInt();
				int i2 = str.mid(index + 1).toInt();
				if(i1 >= 0 && i1 < (int) settings->v_result.size() && i2 < (int) settings->v_result[i1].size()) {
					emit insertTextRequested(settings->v_result[i1][settings->v_result[i1].size() - i2 - 1]);
				}
			}
		} else if(str[0] == '#') {
			int index = str.indexOf(":");
			if(index < 0) emit insertValueRequested(str.mid(1).toInt());
			else emit insertValueRequested(str.mid(1, index - 1).toInt());
		} else {
			int index = str.indexOf(":");
			if(index < 0) {
				int i = str.toInt();
				if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i]);
			} else {
				int i1 = str.left(index).toInt();
				int i2 = str.mid(index + 1).toInt();
				if(i1 >= 0 && (size_t) i1 < settings->v_result.size() && i2 >= 0 && (size_t) i2 < settings->v_result[i1].size()) emit insertTextRequested(settings->v_result[i1][i2]);
			}
		}
	} else {
		QTextEdit::mouseReleaseEvent(e);
	}
}

void HistoryView::keyPressEvent(QKeyEvent *e) {
	if(e->matches(QKeySequence::Copy)) {
		editCopy();
		return;
	}
	QTextEdit::keyPressEvent(e);
	if(!e->isAccepted() && (e->key() != Qt::Key_Control && e->key() != Qt::Key_Meta && e->key() != Qt::Key_Alt)) {
		expressionEdit->setFocus();
		expressionEdit->keyPressEvent(e);
	}
}
void HistoryView::inputMethodEvent(QInputMethodEvent *e) {
	QTextEdit::inputMethodEvent(e);
	if(!e->isAccepted() && !e->commitString().isEmpty()) {
		expressionEdit->setFocus();
		expressionEdit->inputMethodEvent(e);
	}
}
void HistoryView::editClear() {
	for(size_t i1 = 0; i1 < settings->v_protected.size(); i1++) {
		if(!settings->v_protected[i1]) {
			if(i1 == 0) settings->current_result = NULL;
			settings->v_delexpression[i1] = true;
			int index = s_text.indexOf("<a name=\"" + QString::number(i1) + "\"");
			if(index >= 0) {
				int index2 = s_text.indexOf("<tr><td colspan=\"2\"", index);
				int index1 = s_text.lastIndexOf("<tr><td colspan=\"2\"", index);
				QString new_text;
				if(index1 > 0) new_text = s_text.left(index1);
				if(index2 >= 0) new_text += s_text.mid(index2);
				s_text = new_text;
			}
		}
	}
	remove_top_border(s_text);
	setHtml("<body color=\"" + textColor().name() + "\"><table width=\"100%\">" + s_text + "</table></body>");
}
void HistoryView::editMoveToTop() {
	int i1 = -1, i2 = -1;
	indexAtPos(context_pos, &i1, &i2, NULL, NULL);
	if(i1 < 0 || i1 >= (int) settings->v_delexpression.size()) return;
	settings->v_delexpression[i1] = true;
	settings->v_expression.push_back(settings->v_expression[i1]);
	settings->v_protected.push_back(settings->v_protected[i1]);
	settings->v_delexpression.push_back(false);
	settings->v_result.push_back(settings->v_result[i1]);
	settings->v_exact.push_back(settings->v_exact[i1]);
	settings->v_delresult.push_back(settings->v_delresult[i1]);
	int index = s_text.indexOf("<a name=\"" + QString::number(i1) + "\"");
	if(index >= 0) {
		int index2 = s_text.indexOf("<tr><td colspan=\"2\"", index);
		int index1 = s_text.lastIndexOf("<tr><td colspan=\"2\"", index);
		QString new_text = s_text.mid(index1, index2 - index1);
		int index_a = new_text.indexOf("<a name=\"");
		bool b_expr = true;
		while(index_a >= 0 && index_a + 9 < new_text.length() - 1) {
			index_a += 9;
			int index_b = -1;
			if(b_expr) {
				index_b = new_text.indexOf("\"", index_a);
			} else {
				index_b = new_text.indexOf(":", index_a);
				if(new_text[index_a] == '#' || new_text[index_a] == 'p') {
					if(index_b < 0) break;
					index_a = index_b + 1;
					index_b = new_text.indexOf(":", index_b + 1);
				}
			}
			if(index_b < 0) break;
			new_text.replace(index_a, index_b - index_a, QString::number(settings->v_expression.size() - 1));
			index_a = new_text.indexOf("<a name=\"", index_b + 1);
			b_expr = false;
		}
		replace_one(s_text, "border-top: 0px none", "border-top: 1px dashed");
		PASTE_H
		replace_one(s_text, "padding-top: 0px", "padding-top: " + QString::number(paste_h / 2) + "px");
		if(index1 > 0) new_text += s_text.left(index1);
		if(index2 >= 0) new_text += s_text.mid(index2);
		s_text = new_text;
		remove_top_border(s_text);
	}
	settings->current_result = NULL;
	setHtml("<body color=\"" + textColor().name() + "\"><table width=\"100%\">" + s_text + "</table></body>");
}
void HistoryView::editRemove() {
	int i1 = -1, i2 = -1;
	QString sref;
	indexAtPos(context_pos, &i1, &i2, NULL, &sref);
	if(i1 < 0 || i1 >= (int) settings->v_delexpression.size()) return;
	if(i2 >= 0 && i2 < (int) settings->v_delresult[i1].size()) {
		settings->v_delresult[i1][i2] = true;
		bool b = true;
		for(size_t i = 0; i < settings->v_delresult[i1].size(); i++) {
			if(!settings->v_delresult[i1][i]) {b = false; break;}
		}
		if(b) i2 = -1;
	}
	if(i2 < 0) {
		sref = QString::number(i1);
		settings->v_delexpression[i1] = true;
	}
	int index = s_text.indexOf("<a name=\"" + sref + "\"");
	if(index >= 0) {
		int index2 = s_text.indexOf(i2 < 0 ? "<tr><td colspan=\"2\"" : "</a></td></tr>", index);
		if(index2 >= 0) index2 += (i2 < 0 ? 0 : 14);
		int index1 = s_text.lastIndexOf(i2 < 0 ? "<tr><td colspan=\"2\"" : "<tr", index);
		QString new_text;
		if(index1 > 0) new_text = s_text.left(index1);
		if(i2 >= 0) {
			bool b = true;
			for(size_t i = i2; i < settings->v_delresult[i1].size(); i++) {
				if(!settings->v_delresult[i1][i]) {b = false; break;}
			}
			if(b) {
				PASTE_H
				int index_a = new_text.lastIndexOf("<td style=\"text-align:right") + strlen("<td style=\"text-align:right");
				index_a = new_text.indexOf("\"", index_a);
				if(index_a >= 0) new_text.insert(index_a, "; padding-bottom: " + QString::number(paste_h / 2) + "px");
			}
		}
		if(index2 >= 0) new_text += s_text.mid(index2);
		s_text = new_text;
		if(i2 < 0) {
			remove_top_border(s_text);
		}
	}
	if(i1 == 0) settings->current_result = NULL;
	int vpos = verticalScrollBar()->value();
	setHtml("<body color=\"" + textColor().name() + "\"><table width=\"100%\">" + s_text + "</table></body>");
	verticalScrollBar()->setValue(vpos);
}
void HistoryView::editProtect() {
	int i1 = -1, i2 = -1;
	indexAtPos(context_pos, &i1, &i2);
	if(i1 < 0 || i1 >= (int) settings->v_protected.size()) return;
	settings->v_protected[i1] = protectAction->isChecked();
}
void HistoryView::indexAtPos(const QPoint &pos, int *expression_index, int *result_index, int *value_index, QString *anchorstr) {
	*expression_index = -1;
	*result_index = -1;
	if(value_index) *value_index = -1;
	QString sref = anchorAt(pos);
	if(sref.isEmpty()) {
		QTextCursor cur = cursorForPosition(pos);
		cur.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
		cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
		QString str = cur.selection().toHtml();
		int i = str.lastIndexOf("<a name=\"");
		if(i >= 0) {
			i += 9;
			int i2 = str.indexOf("\"", i);
			if(i2 >= 0) sref = str.mid(i, i2 - i);
		}
	}
	if(anchorstr) *anchorstr = sref;
	if(sref.isEmpty()) return;
	int i = sref.indexOf(":");
	if(sref[0] == '#' || sref[0] == 'p') {
		if(i < 0) return;
		int i2 = sref.indexOf(":", i + 1);
		if(i2 >= 0) {
			*expression_index = sref.mid(i + 1, i2 - (i + 1)).toInt();
			*result_index = sref.mid(i2 + 1).toInt();
			if(*expression_index >= 0 && *expression_index < (int) settings->v_result.size() && *result_index < (int) settings->v_result[*expression_index].size()) {
				*result_index = settings->v_result[*expression_index].size() - *result_index - 1;
			}
			if(value_index) *value_index = sref.mid(1, i - 1).toInt();
		} else {
			*expression_index = sref.mid(i + 1).toInt();
		}
	} else if(i >= 0) {
		*expression_index = sref.left(i).toInt();
		*result_index = sref.mid(i + 1).toInt();
	} else {
		*expression_index = sref.toInt();
	}
}
void HistoryView::contextMenuEvent(QContextMenuEvent *e) {
	if(!cmenu) {
		cmenu = new QMenu(this);
		insertValueAction = cmenu->addAction(tr("Insert Value"), this, SLOT(editInsertValue()));
		insertTextAction = cmenu->addAction(tr("Insert Text"), this, SLOT(editInsertText()));
		copyAction = cmenu->addAction(tr("Copy"), this, SLOT(editCopy()));
		copyAction->setShortcut(QKeySequence::Copy);
		copyAction->setShortcutContext(Qt::WidgetShortcut);
		copyFormattedAction = cmenu->addAction(tr("Copy Formatted Text"), this, SLOT(editCopyFormatted()));
		selectAllAction = cmenu->addAction(tr("Select All"), this, SLOT(selectAll()));
		selectAllAction->setShortcut(QKeySequence::SelectAll);
		selectAllAction->setShortcutContext(Qt::WidgetShortcut);
		findAction = cmenu->addAction(tr("Search…"), this, SLOT(editFind()));
		findAction->setShortcut(QKeySequence::Find);
		findAction->setShortcutContext(Qt::WidgetShortcut);
		cmenu->addSeparator();
		protectAction = cmenu->addAction(tr("Protect"), this, SLOT(editProtect()));
		protectAction->setCheckable(true);
		movetotopAction = cmenu->addAction(tr("Move to Top"), this, SLOT(editMoveToTop()));
		cmenu->addSeparator();
		delAction = cmenu->addAction(tr("Remove"), this, SLOT(editRemove()));
		clearAction = cmenu->addAction(tr("Clear"), this, SLOT(editClear()));
	}
	int i1 = -1, i2 = -1, i3 = -1;
	context_pos = e->pos();
	indexAtPos(context_pos, &i1, &i2, &i3);
	selectAllAction->setEnabled(!s_text.isEmpty());
	protectAction->setChecked(i1 >= 0 && i1 < (int) settings->v_protected.size() && settings->v_protected[i1]);
	if(i1 >= 0 && e->reason() == QContextMenuEvent::Mouse && !textCursor().hasSelection()) {
		insertValueAction->setEnabled(i3 >= 0);
		insertTextAction->setEnabled(true);
		copyAction->setEnabled(true);
		copyFormattedAction->setEnabled(true);
		delAction->setEnabled(true);
		protectAction->setEnabled(true);
		bool b = false;
		for(size_t i = i1; i < settings->v_delexpression.size(); i++) {
			if(!settings->v_delexpression[i]) {
				b = true;
				break;
			}
		}
		movetotopAction->setEnabled(b);
	} else {
		copyAction->setEnabled(textCursor().hasSelection());
		copyFormattedAction->setEnabled(textCursor().hasSelection());
		delAction->setEnabled(false);
		protectAction->setEnabled(false);
		movetotopAction->setEnabled(false);
		insertValueAction->setEnabled(false);
		insertTextAction->setEnabled(false);
	}
	clearAction->setEnabled(!s_text.isEmpty());
	cmenu->popup(e->globalPos());
}
void HistoryView::doFind() {
	if(!find(searchEdit->text())) {
		QTextCursor c = textCursor();
		QTextCursor cbak = c;
		c.movePosition(QTextCursor::Start);
		setTextCursor(c);
		if(!find(searchEdit->text())) {
			setTextCursor(cbak);
		}
	}
}
void HistoryView::editFind() {
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Search"));
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Text:"), this), 0, 0);
	searchEdit = new QLineEdit(this);
	grid->addWidget(searchEdit, 0, 1);
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, dialog);
	connect(buttonBox->addButton(tr("Search"), QDialogButtonBox::AcceptRole), SIGNAL(clicked()), this, SLOT(doFind()));
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), dialog, SLOT(reject()));
	box->addWidget(buttonBox);
	dialog->exec();
	dialog->deleteLater();
}
void HistoryView::editCopyFormatted() {
	if(textCursor().hasSelection()) {
		copy();
	} else {
		int i1 = -1, i2 = -1;
		indexAtPos(context_pos, &i1, &i2);
		if(i1 < 0) return;
		if(i2 < 0) {
			if(i1 >= 0 && (size_t) i1 < settings->v_expression.size()) QApplication::clipboard()->setText(QString::fromStdString(settings->v_expression[i1]));
		} else {
			if((size_t) i1 < settings->v_result.size() && (size_t) i2 < settings->v_result[i1].size()) QApplication::clipboard()->setText(QString::fromStdString(settings->v_result[i1][i2]));
		}
	}
}
void HistoryView::editCopy() {
	if(textCursor().hasSelection()) {
		QString str = textCursor().selection().toHtml();
		int i = str.indexOf("<!--StartFragment-->");
		if(i >= 0) {
			int i2 = str.indexOf("<!--EndFragment-->", i + 20);
			if(i2 >= 0) str = str.mid(i + 20, i2 - i - 20).trimmed();
			str = unhtmlize(str);
		} else if(str.startsWith("<!DOCTYPE")) {
			i = str.indexOf("<body>");
			int i2 = str.indexOf("</body>", i + 6);
			if(i2 >= 0) str = str.mid(i + 6, i2 - i - 6).trimmed();
			str = unhtmlize(str).trimmed();
			str.replace("\n\n", "\n");
			str.replace("\n ", "\n");
		} else {
			str = unhtmlize(textCursor().selectedText());
		}
		QApplication::clipboard()->setText(str);
	} else {
		int i1 = -1, i2 = -1;
		indexAtPos(context_pos, &i1, &i2);
		if(i1 < 0) return;
		if(i2 < 0) {
			if(i1 >= 0 && (size_t) i1 < settings->v_expression.size()) QApplication::clipboard()->setText(QString::fromStdString(unhtmlize(settings->v_expression[i1])));
		} else {
			if((size_t) i1 < settings->v_result.size() && (size_t) i2 < settings->v_result[i1].size()) QApplication::clipboard()->setText(QString::fromStdString(unhtmlize(settings->v_result[i1][i2])));
		}
	}
}
void HistoryView::editInsertValue() {
	int i1 = -1, i2 = -1, i3 = -1;
	indexAtPos(context_pos, &i1, &i2, &i3);
	if(i3 > 0) emit insertValueRequested(i3);
}
void HistoryView::editInsertText() {
	int i1 = -1, i2 = -1;
	indexAtPos(context_pos, &i1, &i2);
	if(i2 >= 0) {
		if(i1 >= 0 && (size_t) i1 < settings->v_result.size() && (size_t) i2 < settings->v_result[i1].size()) emit insertTextRequested(settings->v_result[i1][i2]);
	} else {
		if(i1 >= 0 && (size_t) i1 < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i1]);
	}
}

