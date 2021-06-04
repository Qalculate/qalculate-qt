/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QFontMetrics>
#include <QKeyEvent>
#include <QCompleter>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>

#include <libqalculate/qalculate.h>

#include "expressionedit.h"
#include "qalculateqtsettings.h"

extern QalculateQtSettings *settings;

#define ITEM_ROLE (Qt::UserRole + 10)
#define TYPE_ROLE (Qt::UserRole + 11)

class HTMLDelegate : public QStyledItemDelegate {
	protected:
		void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
		QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

void HTMLDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
	QStyleOptionViewItem optionV4 = option;
	initStyleOption(&optionV4, index);

	QStyle *style = optionV4.widget? optionV4.widget->style() : QApplication::style();

	QTextDocument doc;
	doc.setHtml(optionV4.text);

	/// Painting item without text
	optionV4.text = QString();
	style->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

	QAbstractTextDocumentLayout::PaintContext ctx;

	// Highlighting text if item is selected
	if(optionV4.state & QStyle::State_Selected) ctx.palette.setColor(QPalette::Text, optionV4.palette.color(QPalette::Active, QPalette::HighlightedText));

	QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);
	painter->save();
	painter->translate(textRect.topLeft());
	painter->setClipRect(textRect.translated(-textRect.topLeft()));
	doc.documentLayout()->draw(painter, ctx);
	painter->restore();
}

QSize HTMLDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
	QStyleOptionViewItem optionV4 = option;
	initStyleOption(&optionV4, index);

	QTextDocument doc;
	doc.setHtml(optionV4.text);
	return QSize(doc.idealWidth(), doc.size().height());
}



ExpressionProxyModel::ExpressionProxyModel(QObject *parent) : QSortFilterProxyModel(parent) {}
ExpressionProxyModel::~ExpressionProxyModel() {}

std::string sub_suffix(const ExpressionName *ename) {
	size_t i = ename->name.rfind('_');
	bool b = i == std::string::npos || i == ename->name.length() - 1 || i == 0;
	size_t i2 = 1;
	std::string str;
	if(b) {
		if(is_in(NUMBERS, ename->name[ename->name.length() - 1])) {
			while(ename->name.length() > i2 + 1 && is_in(NUMBERS, ename->name[ename->name.length() - 1 - i2])) {
					i2++;
			}
		}
		str += ename->name.substr(0, ename->name.length() - i2);
	} else {
		str += ename->name.substr(0, i);
	}
	str += "<span size=\"small\"><sub>";
	if(b) str += ename->name.substr(ename->name.length() - i2, i2);
	else str += ename->name.substr(i + 1, ename->name.length() - (i + 1));
	str += "</sub></span>";
	return str;
}

bool equalsIgnoreCase(const std::string &str1, const std::string &str2, size_t i2, size_t i2_end, size_t minlength) {
	if(str1.empty() || str2.empty()) return false;
	size_t l = 0;
	if(i2_end == std::string::npos) i2_end = str2.length();
	for(size_t i1 = 0;; i1++, i2++) {
		if(i2 >= i2_end) {
			return i1 >= str1.length();
		}
		if(i1 >= str1.length()) break;
		if((str1[i1] < 0 && i1 + 1 < str1.length()) || (str2[i2] < 0 && i2 + 1 < str2.length())) {
			size_t iu1 = 1, iu2 = 1;
			if(str1[i1] < 0) {
				while(iu1 + i1 < str1.length() && str1[i1 + iu1] < 0) {
					iu1++;
				}
			}
			if(str2[i2] < 0) {
				while(iu2 + i2 < str2.length() && str2[i2 + iu2] < 0) {
					iu2++;
				}
			}
			bool isequal = (iu1 == iu2);
			if(isequal) {
				for(size_t i = 0; i < iu1; i++) {
					if(str1[i1 + i] != str2[i2 + i]) {
						isequal = false;
						break;
					}
				}
			}
			if(!isequal) {
				char *gstr1 = utf8_strdown(str1.c_str() + (sizeof(char) * i1), iu1);
				char *gstr2 = utf8_strdown(str2.c_str() + (sizeof(char) * i2), iu2);
				if(!gstr1 || !gstr2) return false;
				bool b = strcmp(gstr1, gstr2) == 0;
				free(gstr1);
				free(gstr2);
				if(!b) return false;
			}
			i1 += iu1 - 1;
			i2 += iu2 - 1;
		} else if(str1[i1] != str2[i2] && !((str1[i1] >= 'a' && str1[i1] <= 'z') && str1[i1] - 32 == str2[i2]) && !((str1[i1] <= 'Z' && str1[i1] >= 'A') && str1[i1] + 32 == str2[i2])) {
			return false;
		}
		l++;
	}
	return l >= minlength;
}

bool ExpressionProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
	QModelIndex index = sourceModel()->index(source_row, 0);
	int type = index.data(TYPE_ROLE).toInt();
	if(type == 1) {
		ExpressionItem *item = (ExpressionItem*) index.data(ITEM_ROLE).value<void*>();
		if(!item) return false;
		int b = false;
		for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
			if(item->getName(i2).case_sensitive) {
				if(s_filter == item->getName(i2).name.substr(0, s_filter.length())) {
					b = 1;
					break;
				}
			} else {
				if(equalsIgnoreCase(s_filter, item->getName(i2).name, 0, s_filter.length(), 0)) {
					b = 1;
					break;
				}
			}
		}
		return b;
	} else if(type == 2) {
		Prefix *prefix = (Prefix*) index.data(ITEM_ROLE).value<void*>();
		if(!prefix) return false;
		int b = false;
		for(size_t i2 = 1; i2 <= prefix->countNames(); i2++) {
			if(prefix->getName(i2).case_sensitive) {
				if(s_filter == prefix->getName(i2).name.substr(0, s_filter.length())) {
					b = 1;
					break;
				}
			} else {
				if(equalsIgnoreCase(s_filter, prefix->getName(i2).name, 0, s_filter.length(), 0)) {
					b = 1;
					break;
				}
			}
		}
		return b;
	}
	return false;
}
void ExpressionProxyModel::setFilter(std::string str) {
	s_filter = str;
	invalidateFilter();
}

ExpressionEdit::ExpressionEdit(QWidget *parent) : QTextEdit(parent) {
	completionModel = new ExpressionProxyModel(this);
	sourceModel = new QStandardItemModel(this);
	sourceModel->setColumnCount(2);
	updateCompletion();
	completionModel->setSourceModel(sourceModel);
	completer = new QCompleter(completionModel, this);
	completer->setWidget(this);
	completer->setMaxVisibleItems(20);
	completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	completer->setModel(completionModel);
	completionView = new QTableView();
	completionView->setSelectionBehavior(QAbstractItemView::SelectRows);
	completionView->setShowGrid(false);
	completionView->verticalHeader()->hide();
	completionView->horizontalHeader()->hide();
	completionView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	HTMLDelegate* delegate = new HTMLDelegate();
	completionView->setItemDelegateForColumn(0, delegate);
	completionView->setItemDelegateForColumn(1, delegate);
	completer->setPopup(completionView);
}
ExpressionEdit::~ExpressionEdit() {}

void ExpressionEdit::updateCompletion() {
	std::string str, title;
	QList<QStandardItem *> items;
	QFont ifont(font());
	ifont.setStyle(QFont::StyleItalic);
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(CALCULATOR->functions[i]->isActive()) {
			const ExpressionName *ename, *ename_r;
			ename_r = &CALCULATOR->functions[i]->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, completionView);
			if(ename_r->suffix && ename_r->name.length() > 1) {
				str = sub_suffix(ename_r);
			} else {
				str = ename_r->name;
			}
			str += "()";
			for(size_t name_i = 1; name_i <= CALCULATOR->functions[i]->countNames(); name_i++) {
				ename = &CALCULATOR->functions[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), this))) {
					str += " <i>";
					if(ename->suffix && ename->name.length() > 1) {
						str += sub_suffix(ename);
					} else {
						str += ename->name;
					}
					str += "()</i>";
				}
			}
			items.clear();
			QStandardItem *item = new QStandardItem(QString::fromStdString(str));
			item->setData(QVariant::fromValue((void*) CALCULATOR->functions[i]), ITEM_ROLE);
			item->setData(QVariant::fromValue(1), TYPE_ROLE);
			items.append(item);
			item = new QStandardItem(QString::fromStdString(CALCULATOR->functions[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView)));
			item->setData(ifont, Qt::FontRole);
			items.append(item);
			sourceModel->appendRow(items);
			//gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, CALCULATOR->functions[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView).c_str(), 2, CALCULATOR->functions[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);
		}
	}
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isActive()) {
			const ExpressionName *ename, *ename_r;
			bool b = false;
			ename_r = &CALCULATOR->variables[i]->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, completionView);
			for(size_t name_i = 1; name_i <= CALCULATOR->variables[i]->countNames(); name_i++) {
				ename = &CALCULATOR->variables[i]->getName(name_i);
				if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), completionView))) {
					if(!b) {
						if(ename_r->suffix && ename_r->name.length() > 1) {
							str = sub_suffix(ename_r);
						} else {
							str = ename_r->name;
						}
						b = true;
					}
					str += " <i>";
					if(ename->suffix && ename->name.length() > 1) {
						str += sub_suffix(ename);
					} else {
						str += ename->name;
					}
					str += "</i>";
				}
			}
			if(!b && ename_r->suffix && ename_r->name.length() > 1) {
				str = sub_suffix(ename_r);
				b = true;
			}
			if(settings->printops.use_unicode_signs && can_display_unicode_string_function("→", completionView)) {
				size_t pos = 0;
				if(b) {
					pos = str.find("_to_");
				} else {
					pos = ename_r->name.find("_to_");
					if(pos != std::string::npos) {
						str = ename_r->name;
						b = true;
					}
				}
				if(b) {
					while(pos != std::string::npos) {
						if((pos == 1 && str[0] == 'm') || (pos > 1 && str[pos - 1] == 'm' && str[pos - 2] == '>')) {
							str.replace(pos, 4, "<span size=\"small\"><sup>-1</sup></span>→");
						} else {
							str.replace(pos, 4, "→");
						}
						pos = str.find("_to_", pos);
					}
				}
			}
			if(!CALCULATOR->variables[i]->title(false).empty()) {
				items.clear();
				QStandardItem *item = new QStandardItem(QString::fromStdString(b ? str : ename_r->name));
				item->setData(QVariant::fromValue((void*) CALCULATOR->variables[i]), ITEM_ROLE);
				item->setData(QVariant::fromValue(1), TYPE_ROLE);
				items.append(item);
				item = new QStandardItem(QString::fromStdString(CALCULATOR->variables[i]->title()));
				item->setData(ifont, Qt::FontRole);
				items.append(item);
				sourceModel->appendRow(items);
				/*if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, CALCULATOR->variables[i]->title().c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, CALCULATOR->variables[i]->title().c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);*/
			} else {
				Variable *v = CALCULATOR->variables[i];
				if(settings->isAnswerVariable(v)) {
					title = tr("a previous result").toStdString();
				} else if(v->isKnown()) {
					if(((KnownVariable*) v)->isExpression()) {
						ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
						title = CALCULATOR->localizeExpression(((KnownVariable*) v)->expression(), pa);
						if(title.length() > 30) {title = title.substr(0, 30); title += "…";}
						else if(!((KnownVariable*) v)->unit().empty() && ((KnownVariable*) v)->unit() != "auto") {title += " "; title += ((KnownVariable*) v)->unit();}
					} else {
						if(((KnownVariable*) v)->get().isMatrix()) {
							title = tr("matrix").toStdString();
						} else if(((KnownVariable*) v)->get().isVector()) {
							title = tr("vector").toStdString();
						} else {
							PrintOptions po;
							po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
							title = CALCULATOR->print(((KnownVariable*) v)->get(), 30, po);
							if(title.length() > 30) {title = title.substr(0, 30); title += "…";}
						}
					}
				} else {
					if(((UnknownVariable*) v)->assumptions()) {
						switch(((UnknownVariable*) v)->assumptions()->sign()) {
							case ASSUMPTION_SIGN_POSITIVE: {title = tr("positive").toStdString(); break;}
							case ASSUMPTION_SIGN_NONPOSITIVE: {title = tr("non-positive").toStdString(); break;}
							case ASSUMPTION_SIGN_NEGATIVE: {title = tr("negative").toStdString(); break;}
							case ASSUMPTION_SIGN_NONNEGATIVE: {title = tr("non-negative").toStdString(); break;}
							case ASSUMPTION_SIGN_NONZERO: {title = tr("non-zero").toStdString(); break;}
							default: {}
						}
						if(!title.empty() && ((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_NONE) title += " ";
						switch(((UnknownVariable*) v)->assumptions()->type()) {
							case ASSUMPTION_TYPE_BOOLEAN: {title += tr("boolean").toStdString(); break;}
							case ASSUMPTION_TYPE_INTEGER: {title += tr("integer").toStdString(); break;}
							case ASSUMPTION_TYPE_RATIONAL: {title += tr("rational").toStdString(); break;}
							case ASSUMPTION_TYPE_REAL: {title += tr("real").toStdString(); break;}
							case ASSUMPTION_TYPE_COMPLEX: {title += tr("complex").toStdString(); break;}
							case ASSUMPTION_TYPE_NUMBER: {title += tr("number").toStdString(); break;}
							case ASSUMPTION_TYPE_NONMATRIX: {title += tr("(not matrix)").toStdString(); break;}
							default: {}
						}
						if(title.empty()) title = tr("unknown").toStdString();
					} else {
						title = tr("default assumptions").toStdString();
					}
				}
				items.clear();
				QStandardItem *item = new QStandardItem(QString::fromStdString(b ? str : ename_r->name));
				item->setData(QVariant::fromValue((void*) CALCULATOR->variables[i]), ITEM_ROLE);
				item->setData(QVariant::fromValue(1), TYPE_ROLE);
				items.append(item);
				item = new QStandardItem(QString::fromStdString(title));
				item->setData(ifont, Qt::FontRole);
				items.append(item);
				sourceModel->appendRow(items);
				/*if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, title.c_str(), 2, CALCULATOR->variables[i], 3, FALSE, 4, 0, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);*/
			}
		}
	}
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		Unit *u = CALCULATOR->units[i];
		if(u->isActive()) {
			if(u->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				const ExpressionName *ename, *ename_r;
				bool b = false;
				ename_r = &u->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, completionView);
				for(size_t name_i = 1; name_i <= u->countNames(); name_i++) {
					ename = &u->getName(name_i);
					if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), completionView))) {
						if(!b) {
							if(ename_r->suffix && ename_r->name.length() > 1) {
								str = sub_suffix(ename_r);
							} else {
								str = ename_r->name;
							}
							b = true;
						}
						str += " <i>";
						if(ename->suffix && ename->name.length() > 1) {
							str += sub_suffix(ename);
						} else {
							str += ename->name;
						}
						str += "</i>";
					}
				}
				if(!b && ename_r->suffix && ename_r->name.length() > 1) {
					str = sub_suffix(ename_r);
					b = true;
				}
				//std::unordered_map<std::string, GdkPixbuf*>::const_iterator it_flag = flag_images.end();
				title = u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView);
				if(u->isCurrency()) {
					//it_flag = flag_images.find(u->referenceName());
				} else if(u->isSIUnit() && !u->category().empty() && title[title.length() - 1] != ')') {
					size_t i_slash = std::string::npos;
					if(u->category().length() > 1) i_slash = u->category().rfind("/", u->category().length() - 2);
					if(i_slash != std::string::npos) i_slash++;
					if(title.length() + u->category().length() - (i_slash == std::string::npos ? 0 : i_slash) < 35) {
						title += " (";
						if(i_slash == std::string::npos) title += u->category();
						else title += u->category().substr(i_slash, u->category().length() - i_slash);
						title += ")";
					}
				}
				items.clear();
				QStandardItem *item = new QStandardItem(QString::fromStdString(b ? str : ename_r->name));
				item->setData(QVariant::fromValue((void*) u), ITEM_ROLE);
				item->setData(QVariant::fromValue(1), TYPE_ROLE);
				items.append(item);
				item = new QStandardItem(QString::fromStdString(title));
				item->setData(ifont, Qt::FontRole);
				items.append(item);
				sourceModel->appendRow(items);
				/*if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, u, 3, FALSE, 4, 0, 5, it_flag == flag_images.end() ? NULL : it_flag->second, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);
				else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, title.c_str(), 2, u, 3, FALSE, 4, 0, 5, it_flag == flag_images.end() ? NULL : it_flag->second, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);*/
			} else if(!u->isHidden()) {
				CompositeUnit *cu = (CompositeUnit*) u;
				Prefix *prefix = NULL;
				int exp = 1;
				if(cu->countUnits() == 1 && (u = cu->get(1, &exp, &prefix)) != NULL && prefix != NULL && exp == 1) {
					str = "";
					for(size_t name_i = 0; name_i < 2; name_i++) {
						const ExpressionName *ename;
						ename = &prefix->preferredInputName(name_i != 1, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, completionView);
						if(!ename->name.empty() && (ename->abbreviation == (name_i != 1))) {
							bool b_italic = !str.empty();
							if(b_italic) str += " <i>";
							if(ename->suffix && ename->name.length() > 1) {
								str += sub_suffix(ename);
							} else {
								str += ename->name;
							}
							str += u->preferredInputName(name_i != 1, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, completionView).name;
							if(b_italic) str += "</i>";
						}
					}
				} else {
					str = cu->print(false, true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView);
					size_t i_pow = str.find("^");
					while(i_pow != std::string::npos) {
						size_t i_end = str.find_first_of(NUMBERS);
						if(i_end == std::string::npos) break;
						if(i_end != str.length() - 1) {
							i_end = str.find_first_not_of(NUMBERS, i_end + 1);
						}
						str.erase(i_pow, 1);
						if(i_end == std::string::npos) str += "</sup></span>";
						else str.insert(i_end, "</sup></span>");
						str.insert(i_pow, "<span size=\"small\"><sup>");
						if(i_end == std::string::npos) break;
						i_pow = str.find("^", i_pow + 1);
					}
					//if(settings->printops.multiplication_sign == MULTIPLICATION_SIGN_DOT) gsub(saltdot, sdot, str);
					gsub("_unit", "", str);
					gsub("_eunit", "<span size=\"small\"><sub>e</sub></span>", str);
				}
				size_t i_slash = std::string::npos;
				if(cu->category().length() > 1) i_slash = cu->category().rfind("/", cu->category().length() - 2);
				if(i_slash != std::string::npos) i_slash++;
				title = cu->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView);
				if(cu->isSIUnit() && !cu->category().empty()) {
					if(title.length() + cu->category().length() - (i_slash == std::string::npos ? 0 : i_slash) < 35 && title[title.length() - 1] != ')') {
						title += " (";
						if(i_slash == std::string::npos) title += cu->category();
						else title += cu->category().substr(i_slash, cu->category().length() - i_slash);
						title += ")";
					} else {
						if(i_slash == std::string::npos) title = cu->category();
						else title = cu->category().substr(i_slash, cu->category().length() - i_slash);
					}
				}
				items.clear();
				QStandardItem *item = new QStandardItem(QString::fromStdString(str));
				item->setData(QVariant::fromValue((void*) cu), ITEM_ROLE);
				item->setData(QVariant::fromValue(1), TYPE_ROLE);
				items.append(item);
				item = new QStandardItem(QString::fromStdString(title));
				item->setData(ifont, Qt::FontRole);
				items.append(item);
				sourceModel->appendRow(items);
				//gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, title.c_str(), 2, cu, 3, FALSE, 4, 0, 5, NULL, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 1, -1);
			}
		}
	}
	std::string sprefix = tr("Prefix").toStdString();
	for(size_t i = 1; ; i++) {
		Prefix *p = CALCULATOR->getPrefix(i);
		if(!p) break;
		str = "";
		const ExpressionName *ename, *ename_r;
		bool b = false;
		ename_r = &p->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, completionView);
		for(size_t name_i = 1; name_i <= p->countNames(); name_i++) {
			ename = &p->getName(name_i);
			if(ename && ename != ename_r && !ename->completion_only && !ename->plural && (!ename->unicode || can_display_unicode_string_function(ename->name.c_str(), completionView))) {
				if(!b) {
					if(ename_r->suffix && ename_r->name.length() > 1) {
						str = sub_suffix(ename_r);
					} else {
						str = ename_r->name;
					}
					b = true;
				}
				str += " <i>";
				if(ename->suffix && ename->name.length() > 1) {
					str += sub_suffix(ename);
				} else {
					str += ename->name;
				}
				str += "</i>";
			}
		}
		if(!b && ename_r->suffix && ename_r->name.length() > 1) {
			str = sub_suffix(ename_r);
			b = true;
		}
		title = sprefix;
		title += ": ";
		switch(p->type()) {
			case PREFIX_DECIMAL: {
				title += "10<sup>"; title += i2s(((DecimalPrefix*) p)->exponent()); title += "</sup>";
				break;
			}
			case PREFIX_BINARY: {
				title += "2<sup>"; title += i2s(((BinaryPrefix*) p)->exponent()); title += "</sup>";
				break;
			}
			case PREFIX_NUMBER: {
				title += ((NumberPrefix*) p)->value().print();
				break;
			}
		}
		items.clear();
		QStandardItem *item = new QStandardItem(QString::fromStdString(b ? str : ename_r->name));
		item->setData(QVariant::fromValue((void*) p), ITEM_ROLE);
		item->setData(QVariant::fromValue(2), TYPE_ROLE);
		items.append(item);
		item = new QStandardItem(QString::fromStdString(title));
		item->setData(ifont, Qt::FontRole);
		items.append(item);
		sourceModel->appendRow(items);
		/*if(b) gtk_list_store_set(completion_store, &iter, 0, str.c_str(), 1, gstr, 2, p, 3, FALSE, 4, 0, 5, NULL, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 2, -1);
		else gtk_list_store_set(completion_store, &iter, 0, ename_r->name.c_str(), 1, gstr, 2, p, 3, FALSE, 4, 0, 5, NULL, 6, PANGO_WEIGHT_NORMAL, 7, 0, 8, 2, -1);*/
	}
}

void ExpressionEdit::setExpression(std::string str) {
	setPlainText(QString::fromStdString(str));
}
std::string ExpressionEdit::expression() const {
	return toPlainText().toStdString();
}
QSize ExpressionEdit::sizeHint() const {
	QSize size = QTextEdit::sizeHint();
	QFontMetrics fm(font());
	size.setHeight(fm.boundingRect("Äy").height() * 3);
	return size;
}
void ExpressionEdit::keyPressEvent(QKeyEvent *event) {
	if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
		switch(event->key()) {
			case Qt::Key_Asterisk: {
				insertPlainText(SIGN_MULTIPLICATION);
				return;
			}
			case Qt::Key_Minus: {
				insertPlainText(SIGN_MINUS);
				return;
			}
			case Qt::Key_Dead_Circumflex: {
				insertPlainText("^");
				return;
			}
			case Qt::Key_Return: {}
			case Qt::Key_Enter: {
				completionModel->setFilter(toPlainText().toStdString());
				completionView->resizeColumnsToContents();
				completionView->resizeRowsToContents();
				completionView->selectRow(0);
				QRect rect = cursorRect();
				rect.setWidth(completionView->sizeHint().width());
				completer->complete(rect);
				//emit returnPressed();
				return;
			}
		}
	}
	if(event->key() == Qt::Key_Asterisk && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::KeypadModifier) || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
		insertPlainText("^");
		return;
	}
	QTextEdit::keyPressEvent(event);
}

