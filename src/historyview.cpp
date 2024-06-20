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
#include <QMimeData>
#include <QDate>
#include <QDesktopServices>
#include <QToolTip>
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

QString unhtmlize(QString str, bool b_ascii) {
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
				QString str2 = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1), b_ascii);
				if(!b_ascii && str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i, SIGN_POWER_2);
				else if(!b_ascii && str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i, SIGN_POWER_3);
				else if(str.length() == i3 + 7 && (str2.length() == 1 || !qstring_has_nondigit(str2))) str.replace(i, i3 - i, "^" + str2);
				else str.replace(i, i3 - i, "^(" + str2 + ")");
				continue;
			}
		} else if(i_sub > i && i2 > i_sub) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				if(i_small > i && i2 > i_small) str.remove(i, i3 - i);
				else str.replace(i, i3 - i, "_" + unhtmlize(str.mid(i2 + 1, i3 - i2 - 1), b_ascii));
				continue;
			}
		} else if(i_italic > i && i2 > i_italic) {
			int i3 = str.indexOf("</", i2 + 1);
			if(i3 >= 0) {
				QString name = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1), b_ascii);
				if(!name.isEmpty() && name[0] != '\''  && name[0] != '\"') {
					Variable *v = CALCULATOR->getActiveVariable(name.toStdString());
					bool replace_all_i = (!v || v->isKnown());
					if(replace_all_i && name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
						name.insert(0, "\\");
						str.replace(i, i3 - i, name);
						continue;
					} else if(replace_all_i) {
						name.insert(0, "\"");
						name += "\"";
						str.replace(i, i3 - i, name);
						continue;
					}
				}
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
		} else if(i2 - i == 5 && str.mid(i + 1, 4) == "head") {
			int i3 = str.indexOf("</head>", i2 + 1);
			if(i3 > 0) {
				i2 = i3 + 6;
			}
		} else if(i2 - i == 4) {
			if(str.mid(i + 1, 3) == "sup") {
				int i3 = str.indexOf("</sup>", i2 + 1);
				if(i3 >= 0) {
					QString str2 = unhtmlize(str.mid(i + 5, i3 - i - 5), b_ascii);
					if(str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else if(str.length() == i3 + 6 && (str2.length() == 1 || !qstring_has_nondigit(str2))) str.replace(i, i3 - i + 6, "^" + str2);
					else str.replace(i, i3 - i + 6, "^(" + str2 + ")");
					continue;
				}
			} else if(str.mid(i + 1, 3) == "sub") {
				int i3 = str.indexOf("</sub>", i + 4);
				if(i3 >= 0) {
					str.replace(i, i3 - i + 6, "_" + unhtmlize(str.mid(i + 5, i3 - i - 5), b_ascii));
					continue;
				}
			}
		} else if(i2 - i == 17 && str.mid(i + 1, 16) == "i class=\"symbol\"") {
			int i3 = str.indexOf("</i>", i2 + 1);
			if(i3 >= 0) {
				QString name = unhtmlize(str.mid(i2 + 1, i3 - i2 - 1), b_ascii);
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
	str.replace("&thinsp;", THIN_SPACE);
	str.replace("&#8239;", NNBSP);
	str.replace("&quot;", "\"");
	return str;
}

HistoryView::HistoryView(QWidget *parent) : QTextEdit(parent) {
	fileMenu = NULL;
	i_pos = 0;
	i_pos_p = 0;
	previous_cursor = 0;
	previous_temporary = 0;
	last_ans = 0;
#ifdef _WIN32
	has_lock_symbol = 1;
#else
	has_lock_symbol = -1;
#endif
	initial_loaded = false;
	setReadOnly(true);
	QImage img1px(1, 1, QImage::Format_ARGB32);
	img1px.fill(Qt::transparent);
	document()->addResource(QTextDocument::ImageResource, QUrl("data://img1px.png"), QVariant(img1px));
	text_color = palette().text().color();
	setTextColor(text_color);
	cmenu = NULL;
	searchDialog = NULL;
	findAction = new QAction(tr("Search…"), this);
	addAction(findAction);
	connect(findAction, SIGNAL(triggered()), this, SLOT(editFind()));
}
HistoryView::~HistoryView() {}

void HistoryView::replaceColors(QString &str, QColor prev_text_color) {
	if(settings->color == 2) {
		str.replace("color: #585858", "color: #AAAAAA");
		str.replace("color: #800000", "color: #FFAAAA");
		str.replace("color: #000080", "color: #AAAAFF");
		str.replace("color:#005858", "color:#AAFFFF");
		str.replace("color:#585800", "color:#FFFFAA");
		str.replace("color:#580058", "color:#FFAAFF");
		str.replace("color:#800000", "color:#FFAAAA");
		str.replace("color:#000080", "color:#AAAAFF");
		str.replace("color:#008000", "color:#BBFFBB");
		str.replace("color:#668888", "color:#88AAAA");
		str.replace("color:#888866", "color:#AAAA88");
		str.replace("color:#886688", "color:#AA88AA");
		str.replace("color:#666699", "color:#AA8888");
		str.replace("color:#669966", "color:#99BB99");
		str.replace("color: #666666", "color: #AAAAAA");
		str.replace(prev_text_color.name(QColor::HexRgb), text_color.name(QColor::HexRgb));
		str.replace(":/icons/actions", ":/icons/dark/actions");
	} else {
		str.replace("color: #AAAAAA", "color: #585858");
		str.replace("color: #AAAAFF", "color: #000080");
		str.replace("color: #FFAAAA", "color: #800000");
		str.replace("color:#AAFFFF", "color:#005858");
		str.replace("color:#FFFFAA", "color:#585800");
		str.replace("color:#FFAAFF", "color:#580058");
		str.replace("color:#FFAAAA", "color:#800000");
		str.replace("color:#AAAAFF", "color:#000080");
		str.replace("color:#BBFFBB", "color:#008000");
		str.replace("color:#88AAAA", "color:#668888");
		str.replace("color:#AAAA88", "color:#888866");
		str.replace("color:#AA88AA", "color:#886688");
		str.replace("color:#AA8888", "color:#666699");
		str.replace("color:#99BB99", "color:#669966");
		str.replace("color: #AAAAAA", "color: #666666");
		str.replace(prev_text_color.name(QColor::HexRgb), text_color.name(QColor::HexRgb));
		str.replace(":/icons/dark/actions", ":/icons/actions");
	}
}
void replace_colors(std::string &str) {
	if(settings->color == 2) {
		gsub("color:#005858", "color:#AAFFFF", str);
		gsub("color:#585800", "color:#FFFFAA", str);
		gsub("color:#580058", "color:#FFAAFF", str);
		gsub("color:#800000", "color:#FFAAAA", str);
		gsub("color:#000080", "color:#AAAAFF", str);
		gsub("color:#008000", "color:#BBFFBB", str);
		gsub(":/icons/actions", ":/icons/dark/actions", str);
	} else {
		gsub("color:#AAFFFF", "color:#005858", str);
		gsub("color:#FFFFAA", "color:#585800", str);
		gsub("color:#FFAAFF", "color:#580058", str);
		gsub("color:#FFAAAA", "color:#800000", str);
		gsub("color:#AAAAFF", "color:#000080", str);
		gsub("color:#BBFFBB", "color:#008000", str);
		gsub(":/icons/dark/actions", ":/icons/actions", str);
	}
}
std::string uncolorize(std::string str, bool remove_class) {
	size_t i = 0;
	while(true) {
		i = str.find(" style=\"color:#", i);
		if(i == std::string::npos) break;
		str.erase(i, 22);
	}
	if(remove_class) {
		gsub(" class=\"nous\"", "", str);
		gsub(" class=\"symbol\"", "", str);
	}
	i = 0;
	size_t i2 = 0, i1 = 0;
	int depth = 0;
	while(true) {
		i2 = str.find("</span>", i);
		i = str.find("<span", i);
		if(i2 != std::string::npos && (i == std::string::npos || i2 < i)) {
			if(depth > 0) {
				depth--;
				if(depth == 0 && str[i1 + 5] == '>') {
					str.erase(i2, 7);
					str.erase(i1, 6);
					i = i1;
				} else {
					i = i2 + 7;
				}
			} else {
				i = i2 + 7;
			}
		} else if(i != std::string::npos) {
			if(depth == 0) i1 = i;
			i += 6;
			depth++;
		} else {
			break;
		}
	}
	return str;
}

std::string replace_parse_colors(std::string str) {
	if(settings->color == 2) {
		gsub("color:#AAFFFF", "color:#88AAAA", str);
		gsub("color:#FFFFAA", "color:#AAAA88", str);
		gsub("color:#FFAAFF", "color:#AA88AA", str);
		gsub("color:#FFAAAA", "color:#AA8888", str);
		gsub("color:#BBFFBB", "color:#99BB99", str);
	} else {
		gsub("color:#005858", "color:#668888", str);
		gsub("color:#585800", "color:#888866", str);
		gsub("color:#580058", "color:#886688", str);
		gsub("color:#000080", "color:#666699", str);
		gsub("color:#008000", "color:#669966", str);
	}
	return str;
}

void replace_one(QString &str, const QString &origstr, const QString &newstr, bool b_last = false) {
	int index = (b_last ? str.lastIndexOf(origstr) : str.indexOf(origstr));
	if(index > 0) {
		str.replace(index, origstr.length(), newstr);
	}
}

void HistoryView::loadInitial(bool reload) {
	if(reload && !initial_loaded) return;
	s_text.clear();
	i_pos = 0;
	i_pos_p = 0;
	previous_html.clear();
	last_ans = 0;
	last_ref = "";
	i_pos2 = 0;
	i_pos_p2 = 0;
	previous_html2.clear();
	last_ans2 = 0;
	last_ref2 = "";
	previous_temporary = false;
	previous_cursor = 0;
	previous_cursor2 = 0;
	for(size_t i = 0; i < settings->v_expression.size() && !reload;) {
		if(i > 0 && !settings->v_protected[i] && settings->v_expression[i] == settings->v_expression[i - 1] && settings->v_parse[i] == settings->v_parse[i - 1] && settings->v_result[i] == settings->v_result[i - 1] && settings->v_pexact[i] == settings->v_pexact[i] && settings->v_exact[i] == settings->v_exact[i - 1] && settings->v_value[i] == settings->v_value[i - 1] && settings->v_parse[i].find("rand") == std::string::npos && settings->v_expression[i].find("rand") == std::string::npos) {
			settings->v_expression.erase(settings->v_expression.begin() + i);
			settings->v_parse.erase(settings->v_parse.begin() + i);
			settings->v_result.erase(settings->v_result.begin() + i);
			settings->v_protected.erase(settings->v_protected.begin() + i);
			settings->v_time.erase(settings->v_time.begin() + i);
			settings->v_exact.erase(settings->v_exact.begin() + i);
			settings->v_pexact.erase(settings->v_pexact.begin() + i);
			settings->v_value.erase(settings->v_value.begin() + i);
			settings->v_messages.erase(settings->v_messages.begin() + i);
			settings->v_parseerror.erase(settings->v_parseerror.begin() + i);
		} else {
			i++;
		}
	}
	if(settings->v_expression.empty()) {
		if(!settings->clear_history_on_exit && !reload) {
			setHtml("<body color=\"" + text_color.name() + "\">" + tr("Type a mathematical expression above, e.g. \"5 + 2 / 3\", and press the enter key.") + "</body>");
		} else {
			setHtml("<body color=\"" + text_color.name() + "\"></body>");
		}
	} else {
		size_t i = 0;
		for(; i < settings->v_expression.size() - 1; i++) {
			if(!settings->v_messages[i].isEmpty()) replaceColors(settings->v_messages[i]);
			addResult(settings->v_result[i], settings->v_expression[i], settings->v_pexact[i], settings->v_parse[i], true, false, QString(), NULL, 1, i);
		}
		if((settings->color == 2 && (s_text.contains("color:#00") || s_text.contains("color:#58"))) || (settings->color != 2 && (s_text.contains("color:#FF") || s_text.contains("color:#AA")))) {
			replaceColors(s_text);
			for(size_t i = 0; i < settings->v_expression.size(); i++) {
				replace_colors(settings->v_parse[i]);
				for(size_t i2 = 0; i2 < settings->v_result[i].size(); i2++) {
					replace_colors(settings->v_result[i][i2]);
				}
			}
		}
		setHtml("<body color=\"" + text_color.name() + "\">" + s_text + "</body>");
		if(!settings->v_messages[i].isEmpty()) {
			replaceColors(settings->v_messages[i]);
			if(!reload || settings->history_answer.empty() || !settings->current_result) {
				settings->current_result = NULL;
				settings->v_messages[i].replace("font-size:normal", "font-size:small ");
				settings->v_messages[i].replace("width=\"2\"", "width=\"1\"");
			}
		}
		addResult(settings->v_result[i], settings->v_expression[i], settings->v_pexact[i], settings->v_parse[i], true, false, QString(), NULL, reload && !settings->history_answer.empty() && settings->current_result ? 2 : 1, i);
	}
	initial_loaded = true;
	QFontMetrics fm(font());
	prev_fonti = fm.boundingRect("Öy");
}

#define PASTE_H QFontMetrics fm(font()); \
	int paste_h = fm.ascent() * 0.75; \
	if(paste_h < 12) paste_h = 12;

void HistoryView::clearTemporary() {
	if(!previous_temporary || previous_html.isEmpty()) return;
	QTextCursor cur = textCursor();
	cur.setPosition(previous_cursor);
	cur.setPosition(0, QTextCursor::KeepAnchor);
	setTextCursor(cur);
	cur.deleteChar();
	if(!previous_html2.isEmpty()) {
		cur.setPosition(previous_cursor2);
		cur.setPosition(0, QTextCursor::KeepAnchor);
		setTextCursor(cur);
		cur.deleteChar();
		insertHtml("<table width=\"100%\">" + previous_html2 + "</table>");
		previous_html = previous_html2;
		previous_cursor = textCursor().position();
	} else {
		previous_html.clear();
		previous_cursor = 0;
	}
	previous_temporary = false;
	i_pos = i_pos2;
	i_pos_p = i_pos_p2;
	last_ans = last_ans2;
	last_ref = last_ref2;
	previous_cursor2 = 0;
	i_pos2 = 0;
	i_pos_p2 = 0;
	previous_html2.clear();
	last_ans2 = 0;
	last_ref2 = "";
}

void HistoryView::addResult(std::vector<std::string> values, std::string expression, bool pexact, std::string parse, int exact, bool dual_approx, const QString &image, bool *implicit_warning, int initial_load, size_t index, bool temporary) {
	if(temporary && !previous_temporary) {
		previous_cursor2 = previous_cursor;
	}
	if(!temporary && previous_temporary && expression.empty() && parse.empty()) {
		clearTemporary();
	}
	QString serror;
	bool b_parse_error = (initial_load && !expression.empty() && settings->v_parseerror[index] && !settings->v_messages[index].isEmpty());
	if(!initial_load && CALCULATOR->message()) {
		do {
			if(CALCULATOR->message()->category() == MESSAGE_CATEGORY_IMPLICIT_MULTIPLICATION && (settings->implicit_question_asked || implicit_warning)) {
				if(!settings->implicit_question_asked) *implicit_warning = true;
			} else {
				QString mstr = QString::fromStdString(CALCULATOR->message()->message());
				if(settings->printops.use_unicode_signs) {
					mstr.replace(">=", SIGN_GREATER_OR_EQUAL);
					mstr.replace("<=", SIGN_LESS_OR_EQUAL);
					mstr.replace("!=", SIGN_NOT_EQUAL);
				}
				MessageType mtype = CALCULATOR->message()->type();
				if(temporary) {
					if(mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) {
						if(!serror.isEmpty()) {
							if(!serror.startsWith("• ")) serror = "• " + serror;
							serror += "\n• ";
						}
						serror += mstr;
					}
				} else {
					serror += "<tr><td class=\"message\" colspan=\"2\" style=\"text-align:left; font-size:normal";
					if(mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) {
						serror += "; color: ";
						if(mtype == MESSAGE_ERROR) {
							if(settings->color == 2) serror += "#FFAAAA";
							else serror += "#800000";
						} else {
							if(settings->color == 2) serror += "#AAAAFF";
							else serror += "#000080";
						}
						serror += "";
						if(CALCULATOR->message()->stage() == MESSAGE_STAGE_PARSING) b_parse_error = true;
					}
					serror += "\">";
					if(!mstr.startsWith("-")) serror += "- ";
					serror += mstr.toHtmlEscaped();
					if(!temporary) serror += "</td></tr>";
				}
			}
		} while(CALCULATOR->nextMessage());
		if(serror.isEmpty() && values.empty() && expression.empty() && parse.empty()) return;
	}
	PASTE_H
	QString str;
	if(!expression.empty() || !parse.empty()) {
		if(initial_load && index != settings->v_expression.size() - 1) str += QString("<tr><td colspan=\"2\" style=\"padding-bottom: %1 px; padding-top: %3px; border-top: 1px dashed %2; text-align:left\">").arg(paste_h / 4).arg(text_color.name()).arg(paste_h / 2);
		else str += QString("<tr><td colspan=\"2\" style=\"padding-bottom: %1 px; padding-top: 0px; border-top: 0px none %2; text-align:left\">").arg(paste_h / 4).arg(text_color.name());
		if(temporary) {
			parse_tmp = parse;
			result_tmp = (values.empty() ? "" : values[0]);
		} else if(!initial_load) {
			settings->v_expression.push_back(expression);
			settings->v_parse.push_back(parse);
			settings->v_pexact.push_back(pexact);
			settings->v_protected.push_back(false);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
			settings->v_time.push_back(QDateTime::currentSecsSinceEpoch());
#else
			settings->v_time.push_back(QDateTime::currentMSecsSinceEpoch() / 1000);
#endif
			settings->v_messages.push_back(serror);
			settings->v_parseerror.push_back(b_parse_error);
			settings->v_result.push_back(values);
			settings->v_exact.push_back(std::vector<int>());
			settings->v_value.push_back(std::vector<size_t>());
			for(size_t i = 0; i < values.size(); i++) {
				settings->v_exact[settings->v_exact.size() - 1].push_back(exact || i < values.size() - 1);
				settings->v_value[settings->v_value.size() - 1].push_back(dual_approx && i == 0 ? settings->history_answer.size() - 1 : settings->history_answer.size());
			}
		}
		gsub("</i>", "<img src=\"data://img1px.png\" width=\"1\"/></i>", parse);
		if(!temporary && !expression.empty() && (settings->history_expression_type > 0 || parse.empty())) {
			if(!parse.empty() && settings->history_expression_type > 1 && parse != expression) {
				str += QString("<a name=\"e%1\" style=\"text-decoration: none\">").arg(initial_load ? (int) index : settings->v_expression.size() - 1);
				str += QString::fromStdString(expression).toHtmlEscaped();
				str += "</a>";
				if(settings->color == 2) str += "&nbsp; <i style=\"color: #AAAAAA\">";
				else str += "&nbsp; <i style=\"color: #666666\">";
				str += QString("<a name=\"a%1\" style=\"text-decoration: none\">").arg(initial_load ? (int) index : settings->v_expression.size() - 1);
				if(pexact) str += "= ";
				else str += SIGN_ALMOST_EQUAL " ";
				if(!settings->colorize_result) str += QString::fromStdString(uncolorize(parse, false));
				else str += QString::fromStdString(replace_parse_colors(parse));
				str += "</a>";
				str += "</i>";
			} else {
				str += QString("<a name=\"%1\" style=\"text-decoration: none\">").arg(initial_load ? (int) index : settings->v_expression.size() - 1);
				str += QString::fromStdString(expression).toHtmlEscaped();
				str += "</a>";
			}
		} else {
			if(temporary) str += "<a name=\"TP\" style=\"text-decoration: none\">";
			else str += QString("<a name=\"%1\" style=\"text-decoration: none\">").arg(initial_load ? (int) index : settings->v_expression.size() - 1);
			if(!pexact) str += SIGN_ALMOST_EQUAL " ";
			if(!settings->colorize_result) str += QString::fromStdString(uncolorize(parse, false));
			else str += QString::fromStdString(parse);
			str += "</a>";
			if(!temporary && b_parse_error && !expression.empty()) {
				str += "&nbsp;&nbsp;&nbsp; ";
				if(settings->color == 2) str += "<i style=\"color: #AAAAAA\">";
				else str += "<i style=\"color: #666666\">";
				str += QString("<a name=\"e%1\" style=\"text-decoration: none\">").arg(initial_load ? (int) index : settings->v_expression.size() - 1);
				str += QString::fromStdString(replace_parse_colors(expression));
				str += "</a>";
				str += "</i>";
			}
		}
		if(initial_load && settings->v_protected[index]) {
			if(has_lock_symbol < 0) {
				QFontDatabase database;
				if(database.families(QFontDatabase::Symbol).contains("Noto Color Emoji")) has_lock_symbol = 1;
				else has_lock_symbol = 0;
			}
			if(has_lock_symbol) str += " <small><sup>🔒</sup></small>";
			else str += " <small><sup>[P]</sup></small>";
		}
		if(temporary) {
			temporary_error = serror;
			if(!serror.isEmpty()) str += QString("<img src=\"data://img1px.png\" width=\"2\"/><img valign=\"top\" src=\"%1\" height=\"%2\" alt=\"temporary_error\"/>").arg(":/icons/actions/scalable/warning.svg").arg(fm.ascent());
		}
		str += "</td></tr>";
	} else if(!initial_load && !settings->v_result.empty()) {
		for(size_t i = values.size(); i > 0; i--) {
			settings->v_result[settings->v_result.size() - 1].insert(settings->v_result[settings->v_result.size() - 1].begin(), values[i - 1]);
			settings->v_exact[settings->v_exact.size() - 1].insert(settings->v_exact[settings->v_exact.size() - 1].begin(), exact || i < values.size());
			settings->v_value[settings->v_value.size() - 1].insert(settings->v_value[settings->v_value.size() - 1].begin(), dual_approx && i == 1 ? settings->history_answer.size() - 1 : settings->history_answer.size());
		}
		if(!serror.isEmpty()) {
			settings->v_messages[settings->v_messages.size() - 1] += serror;
		}
	}
	if(!temporary) str += serror;
	if(initial_load) str += settings->v_messages[index];
	str.replace("</i>", "<img src=\"data://img1px.png\" width=\"1\"/></i>");
	if((!expression.empty() || !parse.empty()) && !temporary) {
		i_pos = str.length();
		if(initial_load != 1) i_pos_p = i_pos;
	}
	size_t i_answer_pre = 0;
	if(temporary && values.empty()) {
		values.push_back("");
	}
	for(size_t i = 0; i < values.size(); i++) {
		size_t i_answer = 0;
		if(initial_load) i_answer = settings->v_value[index][i];
		else if(!temporary) i_answer = dual_approx && i == 0 ? settings->history_answer.size() - 1 : settings->history_answer.size();
		QFontMetrics fm(font());
		int w = fm.boundingRect("#9999").width();
		str += "<tr><td valign=\"center\" width=\""; str += QString::number(w); str += "\">";
		w = fm.boundingRect("#9999" + unhtmlize(QString::fromStdString(values[i]))).width();
		if(i_answer != 0 && i_answer != i_answer_pre) {
			if(!initial_load && expression.empty() && parse.empty() && last_ans == i_answer && !last_ref.isEmpty()) {
				previous_html.remove(last_ref);
			}
			QString sref;
			if(settings->color == 2) {
				sref = QString("<a href=\"#%1:%2:%3\" style=\"text-decoration: none; text-align:left; color: #AAAAAA\">#%1</a>").arg(i_answer).arg(initial_load ? (int) index : settings->v_expression.size() - 1).arg(initial_load ? (int) i : settings->v_result[settings->v_result.size() - 1].size() - i - 1);
			} else {
				sref = QString("<a href=\"#%1:%2:%3\" style=\"text-decoration: none; text-align:left; color: #585858\">#%1</a>").arg(i_answer).arg(initial_load ? (int) index : settings->v_expression.size() - 1).arg(initial_load ? (int) i : settings->v_result[settings->v_result.size() - 1].size() - i - 1);
			}
			bool b = true;
			if(initial_load) {
				for(size_t i2 = 0; i2 < i; i2++) {
					if(settings->v_value[index][i2] == i_answer) {
						b = false;
					}
				}
			}
			if(b) str += sref;
			if((initial_load > 1 && i == 0) || (!initial_load && (!dual_approx || i == values.size() - 1))) {
				last_ans = i_answer;
				last_ref = sref;
			}
		}
		str += "</td><td style=\"text-align:right";
		if(initial_load > 1 && i > 0 && i_answer <= i_answer_pre) initial_load = 1;
		if(initial_load == 1 || w > width() * 2 || !settings->format_result) {
			gsub("</i>", "<img src=\"data://img1px.png\" width=\"1\"/></i>", values[i]);
		} else if(w * 2 > width()) {
			str += "; font-size:large";
			gsub("</i>", "<img src=\"data://img1px.png\" width=\"2\"/></i>", values[i]);
		} else {
			str += "; font-size:x-large";
			gsub("</i>", "<img src=\"data://img1px.png\" width=\"2\"/></i>", values[i]);
		}
		if((!expression.empty() || !parse.empty()) && i == values.size() - 1) str += QString("; padding-bottom: %1 px").arg(paste_h / 4);
		str += "\">";
		int b_exact = 1;
		if(initial_load) {
			b_exact = settings->v_exact[index][i];
		} else {
			if(exact < 0) b_exact = -1;
			else if(exact == 0 && i == values.size() - 1) b_exact = 0;
		}
		if(temporary && !values[i].empty()) {
			str += "<a name=\"TR\" style=\"text-decoration: none\">";
		} else if(!temporary) {
			if(i_answer == 0) str += QString("<a name=\"%1:%2\" style=\"text-decoration: none\">").arg(initial_load ? index : settings->v_expression.size() - 1).arg(initial_load ? (int) i : settings->v_result[settings->v_result.size() - 1].size() - i - 1);
			else str += QString("<a name=\"p%1:%2:%3\" style=\"text-decoration: none\">").arg(i_answer).arg(initial_load ? index : settings->v_expression.size() - 1).arg(initial_load ? (int) i : settings->v_result[settings->v_result.size() - 1].size() - i - 1);
		}
		if(!temporary || !values[i].empty()) {
			if(b_exact > 0) str += "= ";
			else if(b_exact == 0) str += SIGN_ALMOST_EQUAL " ";
		}
		if(!settings->colorize_result) str += QString::fromStdString(uncolorize(values[i], false));
		else str += QString::fromStdString(values[i]);
		if(!image.isEmpty() && i == values.size() - 1 && w * 2 <= width()) {
			str += QString("<img src=\"data://img1px.png\" width=\"2\"/><img valign=\"top\" src=\"%1\" height=\"%2\"").arg(image).arg(fm.ascent());
			CALCULATOR->setExchangeRatesUsed(-100);
			int i = CALCULATOR->exchangeRatesUsed();
			CALCULATOR->setExchangeRatesUsed(-100);
			if(i > 0) {
				QString sources_str;
				int n = 0;
				if(i & 0b0001) {sources_str += "\\n"; sources_str += QString::fromStdString(CALCULATOR->getExchangeRatesUrl(1)); n++;}
				if(i & 0b0010) {sources_str += "\\n"; sources_str += QString::fromStdString(CALCULATOR->getExchangeRatesUrl(2)); n++;}
				if(i & 0b0100) {sources_str += "\\n"; sources_str += QString::fromStdString(CALCULATOR->getExchangeRatesUrl(3)); n++;}
				if(i & 0b1000) {sources_str += "\\n"; sources_str += QString::fromStdString(CALCULATOR->getExchangeRatesUrl(4)); n++;}
				if(n > 0) {
					str += " alt=\"";
					str += tr("Exchange rate source(s):", "", n);
					str += sources_str;
					str += "\\n(";
					str += tr("updated %1", "", n).arg(QString::fromStdString(QalculateDateTime(CALCULATOR->getExchangeRatesTime(CALCULATOR->exchangeRatesUsed())).toISOString()));
					str += ")";
					str += "\"";
				}
			}
			str += "/>";
		}
		if(!temporary || !values[i].empty()) str += "</a>";
		str += "</td></tr>";
		i_answer_pre = i_answer;
	}
	str.replace("\n", "<br>");
	int i = 0;
	if(!previous_temporary && (!expression.empty() || !parse.empty())) {
		previous_html2 = previous_html;
		i_pos2 = i_pos;
		i_pos_p2 = i_pos_p;
		last_ans2 = last_ans;
		last_ref2 = last_ref;
	}
	if(!initial_load) {
		if(settings->format_result) {
			previous_html.replace("font-size:normal", "font-size:small ");
			previous_html.replace("width=\"2\"", "width=\"1\"");
		}
		while(true) {
			i = previous_html.indexOf("<img valign=\"top\"", i);
			if(i < 0) break;
			int i2 = previous_html.indexOf(">", i);
			previous_html.remove(i, i2 - i + 1);
			if(i < i_pos_p) i_pos_p -= (i2 - i + 1);
		}
		QTextCursor cur = textCursor();
		cur.setPosition(previous_cursor);
		cur.setPosition(0, QTextCursor::KeepAnchor);
		setTextCursor(cur);
		cur.deleteChar();
	}
	if(expression.empty() && parse.empty()) {
		if(!initial_load && settings->format_result) {
			i = 0;
			while(true) {
				i = previous_html.indexOf("; font-size:x-large", i);
				if(i < 0) break;
				previous_html.remove(i, 19);
				if(i < i_pos_p) i_pos_p -= 19;
			}
			i = 0;
			while(true) {
				i = previous_html.indexOf("; font-size:large", i);
				if(i < 0) break;
				previous_html.remove(i, 17);
				if(i < i_pos_p) i_pos_p -= 17;
			}
			i = 0;
		}
		if(initial_load) {
			s_text.insert(i_pos, str);
		} else {
			QTextCursor cur = textCursor();
			cur.setPosition(0);
			setTextCursor(cur);
			previous_html.insert(i_pos_p, str);
			insertHtml("<table width=\"100%\">" + previous_html + "</table>");
			previous_cursor = textCursor().position();
			previous_html2 = previous_html;
			i_pos2 = i_pos;
			i_pos_p2 = i_pos_p;
			last_ans2 = last_ans;
			last_ref2 = last_ref;
		}
	} else {
		if(initial_load) {
			if(index == settings->v_expression.size() - 1) {
				QTextCursor cur = textCursor();
				cur.setPosition(0);
				setTextCursor(cur);
				insertHtml("<table width=\"100%\">" + str + "</table>");
				previous_html = str;
				previous_cursor = textCursor().position();
			} else {
				s_text.insert(0, "<table width=\"100%\">" + str + "</table>");
			}
		} else {
			if(previous_html.isEmpty()) {
				setHtml("<body color=\"" + text_color.name() + "\"></body>");
			} else if(!previous_temporary) {
				if(settings->format_result) {
					previous_html.remove("; font-size:x-large");
					previous_html.remove("; font-size:large");
				}
				replace_one(previous_html, "border-top: 0px none", "border-top: 1px dashed");
				replace_one(previous_html, "padding-top: 0px", "padding-top: " + QString::number(paste_h / 2) + "px");
				insertHtml("<table width=\"100%\">" + previous_html + "</table>");
			}
			QTextCursor cur = textCursor();
			cur.setPosition(0);
			setTextCursor(cur);
			insertHtml("<table width=\"100%\">" + str + "</table>");
			previous_html = str;
			previous_cursor = textCursor().position();
		}
	}
	previous_temporary = temporary;
}
void HistoryView::changeEvent(QEvent *e) {
	if(e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) {
		text_color = palette().text().color();
		setTextColor(text_color);
		for(size_t i = 0; i < settings->v_expression.size(); i++) {
			replace_colors(settings->v_parse[i]);
			for(size_t i2 = 0; i2 < settings->v_result[i].size(); i2++) {
				replace_colors(settings->v_result[i][i2]);
			}
		}
		reloadHistory();
	} else if(e->type() == QEvent::FontChange) {
		QFontMetrics fm(font());
		if(fm.boundingRect("Öy") != prev_fonti) {
			reloadHistory();
		}
		updateMinimumHeight();
	}
	QTextEdit::changeEvent(e);
}
void HistoryView::addMessages() {
	std::vector<std::string> values;
	addResult(values, "", true, "", true);
}
void HistoryView::mouseMoveEvent(QMouseEvent *e) {
	QString str = anchorAt(e->pos());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
	bool b_tooltip = false;
	if(str.isEmpty() && (document()->documentLayout()->imageAt(e->pos()).indexOf(":/icons") == 0 || (e->pos().x() > width() - 150 && document()->documentLayout()->imageAt(e->pos()).indexOf(":/data/flags") == 0))) {
		QPoint p(viewport()->mapFromParent(e->pos()));
		for(int n = 0; n < 5; n++) {
			QTextCursor cur = cursorForPosition(p);
			cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
			QString str = cur.selection().toHtml();
			int i = str.lastIndexOf(" alt=\"");
			if(i >= 0) {
				i += 6;
				int i2 = str.indexOf("\"", i + 1);
				if(i2 > i) {
					str = str.mid(i, i2 - i);
					if(str == "temporary_error") str = temporary_error;
					else str.replace("\\n", "\n");
					QToolTip::showText(mapToGlobal(e->pos()), str);
					b_tooltip = true;
				}
				break;
			}
			p.rx() += QFontMetrics(font()).ascent() / 2;
		}
	}
	if(!b_tooltip) QToolTip::hideText();
#endif
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
					if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i].empty() ? settings->v_parse[i] : settings->v_expression[i]);
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
			} else if(str[0] == 'e' || str[0] == 'a') {
				int i = str.mid(1).toInt();
				if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i].empty() ? settings->v_parse[i] : settings->v_expression[i]);
			} else {
				int index = str.indexOf(":");
				if(index < 0) {
					int i = str.toInt();
					if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i].empty() ? settings->v_parse[i] : settings->v_expression[i]);
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
		if(str.startsWith("https://")) QDesktopServices::openUrl(QUrl(str));
		if(str[0] == 'p') {
			int index = str.indexOf(":");
			if(index < 0) return;
			str = str.mid(index + 1);
			index = str.indexOf(":");
			if(index < 0) {
				int i = str.toInt();
				if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i].empty() ? settings->v_parse[i] : settings->v_expression[i]);
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
				if(i >= 0 && (size_t) i < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i].empty() ? settings->v_parse[i] : settings->v_expression[i]);
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
	for(size_t i1 = 0; i1 < settings->v_protected.size();) {
		if(!settings->v_protected[i1]) {
			if(i1 == settings->v_protected.size() - 1) settings->current_result = NULL;
			settings->v_expression.erase(settings->v_expression.begin() + i1);
			settings->v_parse.erase(settings->v_parse.begin() + i1);
			settings->v_result.erase(settings->v_result.begin() + i1);
			settings->v_protected.erase(settings->v_protected.begin() + i1);
			settings->v_time.erase(settings->v_time.begin() + i1);
			settings->v_exact.erase(settings->v_exact.begin() + i1);
			settings->v_pexact.erase(settings->v_pexact.begin() + i1);
			settings->v_value.erase(settings->v_value.begin() + i1);
			settings->v_messages.erase(settings->v_messages.begin() + i1);
			settings->v_parseerror.erase(settings->v_parseerror.begin() + i1);
		} else {
			i1++;
		}
	}
	verticalScrollBar()->setValue(0);
	reloadHistory();
}
void HistoryView::editMoveToTop() {
	int i1 = -1, i2 = -1;
	indexAtPos(context_pos, &i1, &i2);
	if(i1 < 0 || i1 >= (int) settings->v_expression.size()) return;
	settings->current_result = NULL;
	settings->v_expression.push_back(settings->v_expression[i1]);
	settings->v_parse.push_back(settings->v_parse[i1]);
	settings->v_pexact.push_back(settings->v_pexact[i1]);
	settings->v_protected.push_back(settings->v_protected[i1]);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
	settings->v_time.push_back(QDateTime::currentSecsSinceEpoch());
#else
	settings->v_time.push_back(QDateTime::currentMSecsSinceEpoch() / 1000);
#endif
	settings->v_result.push_back(settings->v_result[i1]);
	settings->v_exact.push_back(settings->v_exact[i1]);
	settings->v_value.push_back(settings->v_value[i1]);
	settings->v_messages.push_back(settings->v_messages[i1]);
	settings->v_parseerror.push_back(settings->v_parseerror[i1]);
	settings->v_expression.erase(settings->v_expression.begin() + i1);
	settings->v_parse.erase(settings->v_parse.begin() + i1);
	settings->v_result.erase(settings->v_result.begin() + i1);
	settings->v_protected.erase(settings->v_protected.begin() + i1);
	settings->v_time.erase(settings->v_time.begin() + i1);
	settings->v_exact.erase(settings->v_exact.begin() + i1);
	settings->v_pexact.erase(settings->v_pexact.begin() + i1);
	settings->v_value.erase(settings->v_value.begin() + i1);
	settings->v_messages.erase(settings->v_messages.begin() + i1);
	settings->v_parseerror.erase(settings->v_parseerror.begin() + i1);
	verticalScrollBar()->setValue(0);
	reloadHistory();
}
void HistoryView::reloadHistory() {
	int vpos = verticalScrollBar()->value();
	loadInitial(true);
	emit historyReloaded();
	verticalScrollBar()->setValue(vpos);
}
void HistoryView::editRemove() {
	int i1 = -1, i2 = -1;
	indexAtPos(context_pos, &i1, &i2);
	if(i1 < 0 || i1 >= (int) settings->v_expression.size()) return;
	if(i2 >= 0 && i2 < (int) settings->v_result[i1].size()) {
		if(settings->v_result[i1].size() == 1) {
			i2 = -1;
		} else {
			settings->v_result[i1].erase(settings->v_result[i1].begin() + i2);
		}
	}
	if(i2 < 0) {
		if((size_t) i1 == settings->v_expression.size() - 1) settings->current_result = NULL;
		settings->v_expression.erase(settings->v_expression.begin() + i1);
		settings->v_parse.erase(settings->v_parse.begin() + i1);
		settings->v_result.erase(settings->v_result.begin() + i1);
		settings->v_protected.erase(settings->v_protected.begin() + i1);
		settings->v_time.erase(settings->v_time.begin() + i1);
		settings->v_exact.erase(settings->v_exact.begin() + i1);
		settings->v_pexact.erase(settings->v_pexact.begin() + i1);
		settings->v_value.erase(settings->v_value.begin() + i1);
		settings->v_messages.erase(settings->v_messages.begin() + i1);
		settings->v_parseerror.erase(settings->v_parseerror.begin() + i1);
	}
	int vpos = verticalScrollBar()->value();
	reloadHistory();
	verticalScrollBar()->setValue(vpos);
}
void HistoryView::editProtect() {
	int i1 = -1, i2 = -1;
	indexAtPos(context_pos, &i1, &i2);
	if(i1 < 0 || i1 >= (int) settings->v_protected.size()) return;
	settings->v_protected[i1] = protectAction->isChecked();
	if(i1 == 0) settings->current_result = NULL;
	int vpos = verticalScrollBar()->value();
	reloadHistory();
	verticalScrollBar()->setValue(vpos);
}
void HistoryView::indexAtPos(const QPoint &pos, int *expression_index, int *result_index, int *value_index, QString *anchorstr) {
	*expression_index = -1;
	*result_index = -1;
	if(value_index) *value_index = -1;
	QString sref = anchorAt(pos);
	if(sref.isEmpty()) {
		QTextCursor cur = cursorForPosition(viewport()->mapFromParent(pos));
		cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
		QString str = cur.selection().toHtml();
		int i = str.lastIndexOf("<a name=\"");
		if(i >= 0) {
			i += 9;
			int i2 = str.indexOf("\"", i);
			if(i2 >= 0) sref = str.mid(i, i2 - i);
		}
	}
	if(sref.isEmpty()) {
		QTextCursor cur = cursorForPosition(viewport()->mapFromParent(pos));
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
	if(sref.isEmpty() || sref == "TR" || sref == "TP") return;
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
	} else if(sref[0] == 'a' || sref[0] == 'e') {
		*expression_index = sref.mid(1).toInt();
	} else if(i >= 0) {
		*expression_index = sref.left(i).toInt();
		*result_index = sref.mid(i + 1).toInt();
	} else {
		*expression_index = sref.toInt();
	}
}
void HistoryView::updateMinimumHeight() {
	QFontMetrics fm(font());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	setMinimumHeight(fm.lineSpacing() * 4.4 + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + document()->documentMargin() * 2 + viewportMargins().bottom() + viewportMargins().top());
#else
	setMinimumHeight(fm.lineSpacing() * 4.4 + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + document()->documentMargin());
#endif
}
void HistoryView::contextMenuEvent(QContextMenuEvent *e) {
	if(!cmenu) {
		cmenu = new QMenu(this);
		insertValueAction = cmenu->addAction(tr("Insert Value"), this, SLOT(editInsertValue()));
		insertTextAction = cmenu->addAction(tr("Insert Text"), this, SLOT(editInsertText()));
		copyAction = new QAction(tr("Copy"), this);
		connect(copyAction, SIGNAL(triggered()), this, SLOT(editCopy()));
		copyAction->setShortcut(QKeySequence::Copy);
		copyAction->setShortcutContext(Qt::WidgetShortcut);
		copyFormattedAction = cmenu->addAction(tr("Copy"), this, SLOT(editCopyFormatted()));
		copyAsciiAction = cmenu->addAction(tr("Copy unformatted ASCII"), this, SLOT(editCopyAscii()));
		selectAllAction = cmenu->addAction(tr("Select All"), this, SLOT(selectAll()));
		selectAllAction->setShortcut(QKeySequence::SelectAll);
		selectAllAction->setShortcutContext(Qt::WidgetShortcut);
		cmenu->addAction(findAction);
		cmenu->addSeparator();
		protectAction = cmenu->addAction(tr("Protect"), this, SLOT(editProtect()));
		protectAction->setCheckable(true);
		movetotopAction = cmenu->addAction(tr("Move to Top"), this, SLOT(editMoveToTop()));
		cmenu->addSeparator();
		delAction = cmenu->addAction(tr("Remove"), this, SLOT(editRemove()));
		clearAction = cmenu->addAction(tr("Clear"), this, SLOT(editClear()));
		if(fileMenu) {
			fileSeparator = cmenu->addSeparator();
			cmenu->addAction(modeMenu->menuAction());
			cmenu->addAction(fileMenu->menuAction());
			cmenu->addAction(tbAction);
		}
	}
	int i1 = -1, i2 = -1, i3 = -1;
	QString astr;
	context_pos = e->pos();
	indexAtPos(context_pos, &i1, &i2, &i3, &astr);
	bool b_tmp = (i1 < 0 && (astr == "TR" || astr == "TP"));
	selectAllAction->setEnabled(!document()->isEmpty());
	protectAction->setChecked(i1 >= 0 && i1 < (int) settings->v_protected.size() && settings->v_protected[i1]);
	if((i1 >= 0 || b_tmp) && e->reason() == QContextMenuEvent::Mouse && !textCursor().hasSelection()) {
		insertValueAction->setEnabled(i3 >= 0);
		insertTextAction->setEnabled(true);
		copyAction->setEnabled(true);
		copyFormattedAction->setEnabled(true);
		copyAsciiAction->setEnabled(true);
		delAction->setEnabled(i1 >= 0);
		protectAction->setEnabled(i1 >= 0);
		movetotopAction->setEnabled(i1 >= 0 && !settings->v_expression.empty());
	} else {
		copyAction->setEnabled(textCursor().hasSelection());
		copyFormattedAction->setEnabled(textCursor().hasSelection());
		copyAsciiAction->setEnabled(textCursor().hasSelection());
		delAction->setEnabled(false);
		protectAction->setEnabled(false);
		movetotopAction->setEnabled(false);
		insertValueAction->setEnabled(false);
		insertTextAction->setEnabled(false);
	}
	clearAction->setEnabled(!settings->v_expression.empty());
	cmenu->popup(e->globalPos());
}
void HistoryView::setMenuAndToolbarItems(QMenu *mode, QMenu *menu, QAction *action) {
	if(!menu && cmenu && fileMenu) {
		cmenu->removeAction(fileSeparator);
		cmenu->removeAction(modeMenu->menuAction());
		cmenu->removeAction(fileMenu->menuAction());
		cmenu->removeAction(tbAction);
	}
	modeMenu = mode;
	fileMenu = menu;
	tbAction = action;
	if(cmenu && fileMenu) {
		fileSeparator = cmenu->addSeparator();
		cmenu->addAction(modeMenu->menuAction());
		cmenu->addAction(fileMenu->menuAction());
		cmenu->addAction(tbAction);
	}
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
	if(searchDialog) {
		searchDialog->setWindowState((searchDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		searchDialog->show();
		qApp->processEvents();
		searchDialog->raise();
		searchDialog->activateWindow();
		searchEdit->clear();
		searchEdit->setFocus();
		return;
	}
	searchDialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(searchDialog);
	if(settings->always_on_top) searchDialog->setWindowFlags(searchDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	searchDialog->setWindowTitle(tr("Search"));
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Text:"), this), 0, 0);
	searchEdit = new QLineEdit(this);
	searchEdit->setFocus();
	grid->addWidget(searchEdit, 0, 1);
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, searchDialog);
	connect(buttonBox->addButton(tr("Search"), QDialogButtonBox::AcceptRole), SIGNAL(clicked()), this, SLOT(doFind()));
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), searchDialog, SLOT(reject()));
	box->addWidget(buttonBox);
	searchDialog->show();
}

void remove_separator(std::string &copy_text) {
	for(size_t i = ((CALCULATOR->local_digit_group_separator.empty() || CALCULATOR->local_digit_group_separator == " " || CALCULATOR->local_digit_group_separator == settings->printops.decimalpoint()) ? 1 : 0); i < 4; i++) {
		std::string str_sep;
		if(i == 0) str_sep = CALCULATOR->local_digit_group_separator;
		else if(i == 1) str_sep = THIN_SPACE;
		else if(i == 2) str_sep = NNBSP;
		else str_sep = " ";
		size_t index = copy_text.find(str_sep);
		while(index != std::string::npos) {
			if(index > 0 && index + str_sep.length() < copy_text.length() && copy_text[index - 1] >= '0' && copy_text[index - 1] <= '9' && copy_text[index + str_sep.length()] >= '0' && copy_text[index + str_sep.length()] <= '9') {
				copy_text.erase(index, str_sep.length());
			} else {
				index++;
			}
			index = copy_text.find(str_sep, index);
		}
	}
}
QString replace_first_minus(const QString &qstr) {
	if(qstr.indexOf(SIGN_MINUS) == 0) {
		std::string str = qstr.toStdString();
		if(str.find_first_of(OPERATORS) == std::string::npos) {
			for(size_t i = strlen(SIGN_MINUS); i < str.length(); i++) {
				if((signed char) str[i] < 0) return qstr;
			}
			std::string str_new = str;
			str_new.replace(0, strlen(SIGN_MINUS), "-");
			return QString::fromStdString(str_new);
		}
	}
	return qstr;
}

std::string replace_first_minus(const std::string &str) {
	if(str.find(SIGN_MINUS) == 0 && str.find_first_of(OPERATORS) == std::string::npos) {
		for(size_t i = strlen(SIGN_MINUS); i < str.length(); i++) {
			if((signed char) str[i] < 0) return str;
		}
		std::string str_new = str;
		str_new.replace(0, strlen(SIGN_MINUS), "-");
		return str_new;
	}
	return str;
}

std::string unformat(std::string str, bool restorable) {
	remove_separator(str);
	gsub(SIGN_MINUS, "-", str);
	gsub(SIGN_MULTIPLICATION, "*", str);
	gsub(SIGN_MULTIDOT, "*", str);
	gsub(SIGN_MIDDLEDOT, "*", str);
	gsub(THIN_SPACE, " ", str);
	gsub(NNBSP, " ", str);
	gsub(SIGN_DIVISION, "/", str);
	gsub(SIGN_DIVISION_SLASH, "/", str);
	gsub(SIGN_SQRT, "sqrt", str);
	gsub("Ω", "ohm", str);
	gsub("µ", restorable ? "µ" : "u", str);
	return str;
}

void HistoryView::editCopy(int ascii) {
	if(ascii < 0) ascii = settings->copy_ascii;
	if(textCursor().hasSelection()) {
		QString str = textCursor().selection().toHtml(), str_nohtml;
		int i = str.indexOf("<!--StartFragment-->");
		if(i >= 0) {
			int i2 = str.indexOf("<!--EndFragment-->", i + 20);
			if(i2 >= 0) str = str.mid(i + 20, i2 - i - 20).trimmed();
			str_nohtml = unhtmlize(str, ascii);
		} else if(str.startsWith("<!DOCTYPE")) {
			i = str.indexOf("<body>");
			int i2 = str.indexOf("</body>", i + 6);
			if(i2 >= 0) str = str.mid(i + 6, i2 - i - 6).trimmed();
			str_nohtml = unhtmlize(str, ascii).trimmed();
			str_nohtml.replace("\n\n", "\n");
			str_nohtml.replace("\n ", "\n");
		} else {
			str_nohtml = unhtmlize(textCursor().selectedText(), ascii);
		}
		if(ascii) {
			QApplication::clipboard()->setText(QString::fromStdString(unformat(str_nohtml.toStdString())));
		} else {
			i = 0;
			while(true) {
				i = str.indexOf("color:", i);
				if(i < 0) break;
				int i2 = str.indexOf(";", i);
				int i3 = str.indexOf("\"", i);
				if(i2 < 0 || i3 < i2) i2 = i3 - 1;
				if(i2 < 0) break;
				str.remove(i, i2 - i);
			}
			i = 0;
			while(true) {
				i = str.indexOf("<img", i);
				if(i < 0) break;
				int i2 = str.indexOf("/>", i);
				if(i2 < 0) break;
				str.remove(i, i2 - i + 2);
			}
			str.remove("font-size:x-large;");
			str.remove("font-size:large;");
			str.replace("; ;", ";");
			str.replace("style=\" ", "style=\"");
			str.replace("style=\" ", "style=\"");
			str.replace("style=\"; ", "style=\"");
			str.remove(" style=\"\"");
			str.remove(" style=\";\"");
			int i2 = 0;
			QList<int> i1;
			i = 0;
			int depth = 0;
			while(true) {
				i2 = str.indexOf("</span>", i);
				i = str.indexOf("<span", i);
				if(i2 >= 0 && (i < 0 || i2 < i)) {
					if(depth > 0) {
						depth--;
						int i3 = str.indexOf(">", i1.last());
						if(i3 < i1.last()) break;
						QString str2 = str.mid(i1.last() + 5, i3 - (i1.last() + 5) + 1);
						if(str2 == ">") {
							str.remove(i2, 7);
							str.remove(i1.last(), 6);
							i = i1.last();
						} else if(str2 == " style=\"vertical-align:super;\">") {
							str.replace(i2, 7, "</sup>");
							str.replace(i1.last(), str2.length() + 5, "<sup>");
							i = i1.last();
						} else if(str2 == " style=\"font-style:italic; vertical-align:super;\">") {
							str.replace(i2, 7, "</i></sup>");
							str.replace(i1.last(), str2.length() + 5, "<sup><i>");
							i = i1.last();
						} else if(str2 == " style=\"font-style:italic;\">") {
							str.replace(i2, 7, "</i>");
							str.replace(i1.last(), str2.length() + 5, "<i>");
							i = i1.last();
						} else if(str2 == " style=\"vertical-align:sub;\">") {
							str.replace(i2, 7, "</sub>");
							str.replace(i1.last(), str2.length() + 5, "<sub>");
							i = i1.last();
						} else {
							i = i2 + 7;
						}
						i1.removeLast();
					} else {
						i = i2 + 7;
					}
				} else if(i >= 0) {
					i1 << i;
					i += 6;
					depth++;
				} else {
					break;
				}
			}
			i = 0;
			while(true) {
				i = str.indexOf("<a", i);
				if(i < 0) break;
				i2 = str.indexOf(">", i);
				if(i2 < 0) break;
				str.remove(i, i2 - i + 1);
			}
			str.remove("</a>");
			QMimeData *qm = new QMimeData();
			qm->setHtml(str);
			qm->setText(str_nohtml);
			qm->setObjectName("history_selection");
			QApplication::clipboard()->setMimeData(qm);
		}
	} else {
		int i1 = -1, i2 = -1;
		QString astr;
		indexAtPos(context_pos, &i1, &i2, NULL, &astr);
		if(i1 < 0 && astr == "TP") {
			if(ascii) {
				QApplication::clipboard()->setText(QString::fromStdString(unformat(unhtmlize(parse_tmp, true))));
			} else {
				QMimeData *qm = new QMimeData();
				qm->setHtml(QString::fromStdString(uncolorize(parse_tmp)));
				qm->setText(QString::fromStdString(unhtmlize(parse_tmp)));
				qm->setObjectName("history_expression");
				QApplication::clipboard()->setMimeData(qm);
			}
			return;
		}
		bool b_temp = (i1 < 0 && astr == "TR");
		if(i1 < 0 && !b_temp) return;
		if(i2 < 0 && !b_temp) {
			if(i1 >= 0 && (size_t) i1 < settings->v_expression.size()) {
				if(astr[0] == 'e' || (astr[0] != 'a' && settings->history_expression_type == 1 && !settings->v_expression[i1].empty())) {
					if(ascii > 0 || (ascii < 0 && settings->copy_ascii)) {
						QApplication::clipboard()->setText(QString::fromStdString(unformat(settings->v_expression[i1])));
					} else {
						QApplication::clipboard()->setText(QString::fromStdString(settings->v_expression[i1]));
					}
				} else {
					if(ascii) {
						QApplication::clipboard()->setText(QString::fromStdString(unformat(unhtmlize(settings->v_parse[i1], true))));
					} else {
						QMimeData *qm = new QMimeData();
						qm->setHtml(QString::fromStdString(uncolorize(settings->v_parse[i1])));
						qm->setText(QString::fromStdString(unhtmlize(settings->v_parse[i1])));
						qm->setObjectName("history_expression");
						QApplication::clipboard()->setMimeData(qm);
					}
				}
			}
		} else {
			if(b_temp || ((size_t) i1 < settings->v_result.size() && (size_t) i2 < settings->v_result[i1].size())) {
				if(ascii) {
					std::string str = (b_temp ? result_tmp : settings->v_result[i1][i2]);
					if(settings->copy_ascii_without_units) {
						size_t i = str.find("<span style=\"color:#008000\">");
						if(i == std::string::npos) i = str.find("<span style=\"color:#BBFFBB\">");
						if(i != std::string::npos && str.find("</span>", i) == str.length() - 7) {
							str = str.substr(0, i);
						}
					}
					QApplication::clipboard()->setText(QString::fromStdString(unformat(unhtmlize(str, true))).trimmed());
				} else {
					QMimeData *qm = new QMimeData();
					if(b_temp) {
						qm->setHtml(QString::fromStdString(uncolorize(result_tmp)));
						qm->setText(QString::fromStdString(unhtmlize((result_tmp))));
					} else {
						qm->setHtml(QString::fromStdString(uncolorize(settings->v_result[i1][i2])));
						qm->setText(QString::fromStdString(unhtmlize((settings->v_result[i1][i2]))));
					}
					qm->setObjectName("history_result");
					QApplication::clipboard()->setMimeData(qm);
				}
			}
		}
	}
}
void HistoryView::editCopyAscii() {
	editCopy(1);
}
void HistoryView::editCopyFormatted() {
	editCopy(0);
}
void HistoryView::editInsertValue() {
	int i1 = -1, i2 = -1, i3 = -1;
	indexAtPos(context_pos, &i1, &i2, &i3);
	if(i3 > 0) emit insertValueRequested(i3);
}
void HistoryView::editInsertText() {
	int i1 = -1, i2 = -1;
	QString astr;
	indexAtPos(context_pos, &i1, &i2, NULL, &astr);
	if(i1 < 0 && astr == "TR") {
		emit insertTextRequested(result_tmp);
	} else if(i1 < 0 && astr == "TP") {
		emit insertTextRequested(parse_tmp);
	} else if(i2 >= 0) {
		if(i1 >= 0 && (size_t) i1 < settings->v_result.size() && (size_t) i2 < settings->v_result[i1].size()) emit insertTextRequested(settings->v_result[i1][i2]);
	} else {
		if(i1 >= 0 && (size_t) i1 < settings->v_expression.size()) emit insertTextRequested(settings->v_expression[i1].empty() ? settings->v_parse[i1] : settings->v_expression[i1]);
	}
}

