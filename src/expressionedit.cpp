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
#include <QToolTip>
#include <QDebug>

#include "expressionedit.h"
#include "qalculateqtsettings.h"

extern QalculateQtSettings *settings;

#define ITEM_ROLE (Qt::UserRole + 10)
#define TYPE_ROLE (Qt::UserRole + 11)
#define MATCH_ROLE (Qt::UserRole + 12)
#define IMATCH_ROLE (Qt::UserRole + 13)

bool last_is_operator(std::string str, bool allow_exp = false) {
	remove_blank_ends(str);
	if(str.empty()) return false;
	if(str[str.length() - 1] > 0) {
		if(is_in(OPERATORS "\\" LEFT_PARENTHESIS LEFT_VECTOR_WRAP, str[str.length() - 1]) && (str[str.length() - 1] != '!' || str.length() == 1)) return true;
		if(allow_exp && is_in(EXP, str[str.length() - 1])) return true;
		if(str.length() >= 3 && str[str.length() - 1] == 'r' && str[str.length() - 2] == 'o' && str[str.length() - 3] == 'x') return true;
	} else {
		if(str.length() >= 3 && str[str.length() - 2] < 0) {
			str = str.substr(str.length() - 3);
			if(str == "∧" || str == "∨" || str == "⊻" || str == "≤" || str == "≥" || str == "≠" || str == "∠" || str == SIGN_MULTIPLICATION || str == SIGN_DIVISION_SLASH || SIGN_MINUS) {
				return true;
			}
		}
		if(str.length() >= 2) {
			str = str.substr(str.length() - 2);
			if(str == "¬" || str == SIGN_MULTIPLICATION || str == SIGN_DIVISION_SLASH || str == SIGN_MINUS) return true;
		}
	}
	return false;
}

void fix_to_struct_qt(MathStructure &m) {
	if(m.isPower() && m[0].isUnit()) {
		if(m[0].prefix() == NULL && m[0].unit()->referenceName() == "g") {
			m[0].setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
		} else if(m[0].unit() == CALCULATOR->u_euro) {
			Unit *u = CALCULATOR->getLocalCurrency();
			if(u) m[0].setUnit(u);
		}
	} else if(m.isUnit()) {
		if(m.prefix() == NULL && m.unit()->referenceName() == "g") {
			m.setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
		} else if(m.unit() == CALCULATOR->u_euro) {
			Unit *u = CALCULATOR->getLocalCurrency();
			if(u) m.setUnit(u);
		}
	} else {
		for(size_t i = 0; i < m.size();) {
			if(m[i].isUnit()) {
				if(m[i].prefix() == NULL && m[i].unit()->referenceName() == "g") {
					m[i].setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
				} else if(m[i].unit() == CALCULATOR->u_euro) {
					Unit *u = CALCULATOR->getLocalCurrency();
					if(u) m[i].setUnit(u);
				}
				i++;
			} else if(m[i].isPower() && m[i][0].isUnit()) {
				if(m[i][0].prefix() == NULL && m[i][0].unit()->referenceName() == "g") {
					m[i][0].setPrefix(CALCULATOR->getOptimalDecimalPrefix(3));
				} else if(m[i][0].unit() == CALCULATOR->u_euro) {
					Unit *u = CALCULATOR->getLocalCurrency();
					if(u) m[i][0].setUnit(u);
				}
				i++;
			} else {
				m.delChild(i + 1);
			}
		}
		if(m.size() == 0) m.clear();
		if(m.size() == 1) m.setToChild(1);
	}
}

bool contains_imaginary_number(MathStructure &m) {
	if(m.isNumber() && m.number().hasImaginaryPart()) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_imaginary_number(m[i])) return true;
	}
	return false;
}
bool contains_rational_number(MathStructure &m) {
	if(m.isNumber() && ((m.number().realPartIsRational() && !m.number().realPart().isInteger()) || (m.number().hasImaginaryPart() && m.number().imaginaryPart().isRational() && !m.number().imaginaryPart().isInteger()))) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_rational_number(m[i])) {
			return i != 1 || !m.isPower() || !m[1].isNumber() || m[1].number().denominatorIsGreaterThan(9) || (m[1].number().numeratorIsGreaterThan(9) && !m[1].number().denominatorIsTwo() && !m[0].representsNonNegative(true));
		}
	}
	return false;
}
bool contains_related_unit(const MathStructure &m, Unit *u) {
	if(m.isUnit()) return u == m.unit() || u->containsRelativeTo(m.unit()) || m.unit()->containsRelativeTo(u);
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_related_unit(m[i], u)) return true;
	}
	return false;
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

bool title_matches(ExpressionItem *item, const std::string &str, size_t minlength = 0) {
	bool big_A = false;
	if(minlength > 1 && str.length() == 1) {
		if(str[0] == 'a' || str[0] == 'x' || str[0] == 'y' || str[0] == 'X' || str[0] == 'Y') return false;
		big_A = (str[0] == 'A');
	}
	const std::string &title = item->title(true);
	size_t i = 0;
	while(true) {
		while(true) {
			if(i >= title.length()) return false;
			if(title[i] != ' ') break;
			i++;
		}
		size_t i2 = title.find(' ', i);
		if(big_A && title[i] == str[0] && ((i2 == std::string::npos && i == title.length() - 1) || i2 - i == 1)) {
			return true;
		} else if(!big_A && equalsIgnoreCase(str, title, i, i2, minlength)) {
			return true;
		}
		if(i2 == std::string::npos) break;
		i = i2 + 1;
	}
	return false;
}
bool name_matches(ExpressionItem *item, const std::string &str) {
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		if(item->getName(i2).case_sensitive) {
			if(str == item->getName(i2).name.substr(0, str.length())) {
				return true;
			}
		} else {
			if(equalsIgnoreCase(str, item->getName(i2).name, 0, str.length(), 0)) {
				return true;
			}
		}
	}
	return false;
}
int name_matches2(ExpressionItem *item, const std::string &str, size_t minlength, size_t *i_match = NULL) {
	if(minlength > 1 && unicode_length(str) == 1) return 0;
	bool b_match = false;
	for(size_t i2 = 1; i2 <= item->countNames(); i2++) {
		if(equalsIgnoreCase(str, item->getName(i2).name, 0, str.length(), 0)) {
			if(!item->getName(i2).case_sensitive && item->getName(i2).name.length() == str.length()) {
				if(i_match) *i_match = i2;
				return 1;
			}
			if(i_match && *i_match == 0) *i_match = i2;
			b_match = true;
		}
	}
	return b_match ? 2 : 0;
}
bool country_matches(Unit *u, const std::string &str, size_t minlength = 0) {
	const std::string &countries = u->countries();
	size_t i = 0;
	while(true) {
		while(true) {
			if(i >= countries.length()) return false;
			if(countries[i] != ' ') break;
			i++;
		}
		size_t i2 = countries.find(',', i);
		if(equalsIgnoreCase(str, countries, i, i2, minlength)) {
			return true;
		}
		if(i2 == std::string::npos) break;
		i = i2 + 1;
	}
	return false;
}
int completion_names_match(std::string name, const std::string &str, size_t minlength = 0, size_t *i_match = NULL) {
	size_t i = 0, n = 0;
	bool b_match = false;
	while(true) {
		size_t i2 = name.find(i == 0 ? " <i>" : "</i>", i);
		if(equalsIgnoreCase(str, name, i, i2, minlength)) {
			if((i2 == std::string::npos && name.length() - i == str.length()) || (i2 != std::string::npos && i2 - i == str.length())) {
				if(i_match) *i_match = n;
				return 1;
			}
			if(i_match && *i_match == 0) *i_match = n + 1;
			b_match = true;
		}
		if(i2 == std::string::npos) break;
		if(i == 0) {
			i = i2 + 4;
		} else {
			i = name.find("<i>", i2);
			if(i == std::string::npos) break;
			i += 3;
		}
		n++;
	}
	if(i_match && *i_match > 0) *i_match -= 1;
	return (b_match ? 2 : 0);
}

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
	doc.setDefaultFont(optionV4.font);
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
	doc.setDefaultFont(optionV4.font);
	doc.setHtml(optionV4.text);

	return QSize(doc.idealWidth(), doc.size().height());
}

struct CompletionData {
	std::string str2, str3, str4;
	Prefix *p2, *p3, *p4;
	MathStructure *current_from_struct;
	MathFunction *current_function;
	int current_function_index;
	Unit *current_from_unit;
	QModelIndex exact_prefix_match;
	bool exact_match_found, exact_prefix_match_found, enable_completion, enable_completion2, editing_to_expression, editing_to_expression1;
	int completion_min, completion_min2;
	int to_type, highest_match;
	Argument *arg;
	CompletionData() : p2(NULL), p3(NULL), p4(NULL), current_from_struct(NULL), current_function(NULL), current_function_index(0), current_from_unit(NULL), exact_match_found(false), exact_prefix_match_found(false), enable_completion(true), enable_completion2(true), editing_to_expression(false), editing_to_expression1(false), completion_min(1), completion_min2(2), to_type(0), highest_match(0), arg(NULL) {
	}
};

ExpressionProxyModel::ExpressionProxyModel(CompletionData *cd, QObject *parent) : QSortFilterProxyModel(parent), cdata(cd) {
	setSortCaseSensitivity(Qt::CaseInsensitive);
	setSortLocaleAware(true);
	setDynamicSortFilter(false);
}
ExpressionProxyModel::~ExpressionProxyModel() {}

bool ExpressionProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const {
	if(cdata->to_type < 2 && str.empty()) return false;
	QModelIndex index = sourceModel()->index(source_row, 0);
	if(!index.isValid()) return false;
	int p_type = index.data(TYPE_ROLE).toInt();
	if(cdata->arg) {
		return p_type > 2 && p_type < 100;
	}
	ExpressionItem *item = NULL;
	Prefix *prefix = NULL;
	void *p = index.data(ITEM_ROLE).value<void*>();
	if(p_type == 1) item = (ExpressionItem*) p;
	else if(p_type == 2) prefix = (Prefix*) p;
	int b_match = false;
	size_t i_match = 0;
	if(item && cdata->to_type < 2) {
		if((cdata->editing_to_expression || !settings->evalops.parse_options.functions_enabled) && item->type() == TYPE_FUNCTION) {}
		else if(item->type() == TYPE_VARIABLE && (!settings->evalops.parse_options.variables_enabled || (cdata->editing_to_expression && (!((Variable*) item)->isKnown() || ((KnownVariable*) item)->unit().empty())))) {}
		else if(!settings->evalops.parse_options.units_enabled && item->type() == TYPE_UNIT) {}
		else {
			CompositeUnit *cu = NULL;
			int exp = 0;
			if(item->type() == TYPE_UNIT && ((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				cu = (CompositeUnit*) item;
				item = cu->get(1, &exp, &prefix);
				if(item && prefix) {
					for(size_t name_i = 1; name_i <= prefix->countNames(); name_i++) {
						const ExpressionName *ename = &prefix->getName(name_i);
						if(!ename->name.empty() && ename->name.length() >= str.length() && (ename->abbreviation || str.length() >= 2)) {
							bool pmatch = true;
							for(size_t i = 0; i < str.length(); i++) {
								if(ename->name[i] != str[i]) {
									pmatch = false;
									break;
								}
							}
							if(pmatch) {
								b_match = 2;
								item = NULL;
								break;
							}
						}
					}
					if(item && exp == 1 && cu->countUnits() == 1 && ((Unit*) item)->useWithPrefixesByDefault()) {
						if(!b_match && cdata->enable_completion2 && title_matches(cu, str, cdata->completion_min2)) {
							b_match = 4;
						}
						item = NULL;
					}
				}
			}
			for(size_t name_i = 1; item && name_i <= item->countNames() && !b_match; name_i++) {
				const ExpressionName *ename = &item->getName(name_i);
				if(ename && (!cu || ename->abbreviation || str.length() >= 3 || str.length() == ename->name.length())) {
					if(item->isHidden() && (item->type() != TYPE_UNIT || !((Unit*) item)->isCurrency()) && ename) {
						b_match = (ename->name == str) ? 1 : 0;
					} else {
						for(size_t icmp = 0; icmp < 4; icmp++) {
							if(icmp == 1 && (item->type() != TYPE_UNIT || cdata->str2.empty() || (cu && !prefix) || (!cu && !((Unit*) item)->useWithPrefixesByDefault()))) break;
							if(cu && prefix) {
								if(icmp == 0 || (icmp == 1 && prefix != cdata->p2) || (icmp == 2 && prefix != cdata->p3) || (icmp == 3 && prefix != cdata->p3)) continue;
							}
							const std::string *cmpstr;
							if(icmp == 0) cmpstr = &str;
							else if(icmp == 1) cmpstr = &cdata->str2;
							else if(icmp == 2) cmpstr = &cdata->str3;
							else cmpstr = &cdata->str4;
							if(cmpstr->empty()) break;
							if(cmpstr->length() <= ename->name.length()) {
								b_match = 2;
								for(size_t i = 0; i < cmpstr->length(); i++) {
									if(ename->name[i] != (*cmpstr)[i]) {
										b_match = false;
										break;
									}
								}
								if(b_match && (!cu || (exp == 1 && cu->countUnits() == 1)) && ((!ename->case_sensitive && equalsIgnoreCase(ename->name, *cmpstr)) || (ename->case_sensitive && ename->name == *cmpstr))) b_match = 1;
								if(b_match) {
									if(icmp > 0 && !cu) {
										if(icmp == 1) prefix = cdata->p2;
										else if(icmp == 2) prefix = cdata->p3;
										else if(icmp == 3) prefix = cdata->p4;
										i_match = str.length() - cmpstr->length();
									} else if(b_match > 1 && !cdata->editing_to_expression && item->isHidden() && str.length() == 1) {
										b_match = 4;
										i_match = name_i;
									}
									break;
								}
							}
						}
					}
				}
			}
			if(item && ((!cu && b_match >= 2) || (exp == 1 && cu->countUnits() == 1 && b_match == 2)) && item->countNames() > 1) {
				for(size_t icmp = 0; icmp < 4 && b_match > 1; icmp++) {
					if(icmp == 1 && (item->type() != TYPE_UNIT || cdata->str2.empty() || (cu && !prefix) || (!cu && !((Unit*) item)->useWithPrefixesByDefault()))) break;
					if(cu && prefix) {
						if(icmp == 0 || (icmp == 1 && prefix != cdata->p2) || (icmp == 2 && prefix != cdata->p3) || (icmp == 3 && prefix != cdata->p3)) continue;
					}
					const std::string *cmpstr;
					if(icmp == 0) cmpstr = &str;
					else if(icmp == 1) cmpstr = &cdata->str2;
					else if(icmp == 2) cmpstr = &cdata->str3;
					else cmpstr = &cdata->str4;
					if(cmpstr->empty()) break;
					for(size_t name_i = 1; name_i <= item->countNames(); name_i++) {
						if(item->getName(name_i).name == *cmpstr) {
							if(!cu) {
								if(icmp == 1) prefix = cdata->p2;
								else if(icmp == 2) prefix = cdata->p3;
								else if(icmp == 3) prefix = cdata->p4;
								else prefix = NULL;
							}
							b_match = 1; break;
						}
					}
				}
			}
			if(item && !b_match && cdata->enable_completion2 && (!item->isHidden() || (item->type() == TYPE_UNIT && str.length() > 1 && ((Unit*) item)->isCurrency()))) {
				int i_cinm = name_matches2(cu ? cu : item, str, cdata->to_type == 1 ? 1 : cdata->completion_min2, &i_match);
				if(i_cinm == 1) {b_match = 1; i_match = 0;}
				else if(i_cinm == 2) b_match = 4;
				else if(title_matches(cu ? cu : item, str, cdata->to_type == 1 ? 1 : cdata->completion_min2)) b_match = 4;
				else if(!cu && item->type() == TYPE_UNIT && ((Unit*) item)->isCurrency() && country_matches((Unit*) item, str, cdata->to_type == 1 ? 1 : cdata->completion_min2)) b_match = 5;
			}
			if(cu) prefix = NULL;
		}
		if(b_match > 1 && (
		(cdata->to_type == 1 && (!item || item->type() != TYPE_UNIT)) || 
		((b_match > 2 || str.length() < 3) && cdata->editing_to_expression && cdata->current_from_struct && !cdata->current_from_struct->isAborted() && item && item->type() == TYPE_UNIT && !contains_related_unit(*cdata->current_from_struct, (Unit*) item) && (!cdata->current_from_struct->isNumber() || !cdata->current_from_struct->number().isReal() || (!prefix && ((Unit*) item)->isSIUnit() && (Unit*) item != CALCULATOR->getRadUnit())))
		)) {
			b_match = 0;
			i_match = 0;
		}
		if(b_match) {
			QString qstr = index.data(Qt::DisplayRole).toString();
			if(!qstr.isEmpty()) {
				if(qstr[0] == '<') {
					size_t i = qstr.indexOf("-) </small>");
					if(i > 2) {
						if(prefix && prefix->longName() == qstr.mid(8, i - 8).toStdString()) {
							prefix = NULL;
						} else {
							qstr = qstr.mid(i + 11);
							if(!prefix) sourceModel()->setData(index, qstr, Qt::DisplayRole);
						}
					}
				}
				if(prefix) {
					if(qstr.isEmpty()) qstr = index.data(Qt::DisplayRole).toString();
					qstr.insert(0, "-) </small>");
					qstr.insert(0, QString::fromStdString(prefix->longName()));
					qstr.insert(0, "<small>(");
					sourceModel()->setData(index, qstr, Qt::DisplayRole);
				}
			}
			if(b_match == 1 && item->type() != TYPE_FUNCTION) {
				if(prefix) {
					cdata->exact_prefix_match = index;
					cdata->exact_prefix_match_found = true;
				} else {
					cdata->exact_match_found = true;
				}
			}
			if(b_match > cdata->highest_match) cdata->highest_match = b_match;
		}
	} else if(item && cdata->to_type == 4) {
		if(item->type() == TYPE_UNIT && item->category() == cdata->current_from_unit->category()) {
			QString qstr = index.data(Qt::DisplayRole).toString();
			if(!qstr.isEmpty() && qstr[0] == '<') {
				int i = qstr.indexOf("-) </small>");
				if(i > 2) {
					qstr = qstr.right(i + 11);
					sourceModel()->setData(index, qstr, Qt::DisplayRole);
				}
			}
			b_match = 2;
		}
	} else if(item && cdata->to_type == 5) {
		if(item->type() == TYPE_UNIT && ((Unit*) item)->isCurrency() && (!item->isHidden() || item == CALCULATOR->getLocalCurrency())) b_match = 2;
	} else if(prefix && cdata->to_type < 2) {
		for(size_t name_i = 1; name_i <= prefix->countNames() && !b_match; name_i++) {
			const std::string *pname = &prefix->getName(name_i).name;
			if(!pname->empty() && str.length() <= pname->length()) {
				b_match = 2;
				for(size_t i = 0; i < str.length(); i++) {
					if(str[i] != (*pname)[i]) {
						b_match = false;
						break;
					}
				}
				if(b_match && *pname == str) b_match = 1;
			}
		}
		if(cdata->to_type == 1 && b_match > 1) b_match = 0;
		if(b_match > cdata->highest_match) cdata->highest_match = b_match;
		else if(b_match == 1 && cdata->highest_match < 2) cdata->highest_match = 2;
		prefix = NULL;
	} else if(p_type >= 100 && cdata->editing_to_expression && cdata->editing_to_expression1) {
		QString qstr = index.data(Qt::DisplayRole).toString();
		if(cdata->to_type >= 2 && str.empty()) b_match = 2;
		else b_match = completion_names_match(qstr.toStdString(), str, cdata->completion_min, &i_match);
		if(b_match > 1) {
			if(cdata->current_from_struct && str.length() < 3) {
				if(p_type >= 100 && p_type < 200) {
					if(cdata->to_type == 5 || cdata->current_from_struct->containsType(STRUCT_UNIT) <= 0) b_match = 0;
				} else if((p_type == 294 || (p_type == 292 && cdata->to_type == 4)) && cdata->current_from_unit) {
					if(cdata->current_from_unit != CALCULATOR->getDegUnit()) b_match = 0;
				} else if(p_type >= 290 && p_type < 300 && (p_type != 292 || cdata->to_type >= 1)) {
					if(!cdata->current_from_struct->isNumber() || (str.empty() && cdata->current_from_struct->isInteger())) b_match = 0;
				} else if(p_type >= 200 && p_type < 290 && (p_type != 200 || cdata->to_type == 1 || cdata->to_type >= 3)) {
					if(!cdata->current_from_struct->isNumber()) b_match = 0;
					else if(str.empty() && p_type >= 202 && !cdata->current_from_struct->isInteger()) b_match = 0;
				} else if(p_type >= 300 && p_type < 400) {
					if(p_type == 300) {
						if(!contains_rational_number(*cdata->current_from_struct)) b_match = 0;
					} else {
						if(!cdata->current_from_struct->isNumber()) b_match = 0;
					}
				} else if(p_type >= 400 && p_type < 500) {
					if(!contains_imaginary_number(*cdata->current_from_struct)) b_match = 0;
				} else if(p_type >= 500 && p_type < 600) {
					if(!cdata->current_from_struct->isDateTime()) b_match = 0;
				} else if(p_type == 600) {
					if(!cdata->current_from_struct->isInteger() && cdata->current_from_struct->containsType(STRUCT_ADDITION) <= 0) b_match = 0;
				} else if(p_type == 601) {
					if(cdata->current_from_struct->containsType(STRUCT_ADDITION) <= 0) b_match = 0;
				}
			}
			if(b_match > cdata->highest_match) cdata->highest_match = b_match;
		}
	}
	if(b_match) {
		sourceModel()->setData(index, b_match, MATCH_ROLE);
		sourceModel()->setData(index, (qulonglong) i_match, IMATCH_ROLE);
		QFont wfont(index.data(Qt::FontRole).value<QFont>());
		if(b_match == 1) wfont.setWeight(QFont::Bold);
		else wfont.setWeight(QFont::Normal);
		sourceModel()->setData(index, wfont, Qt::FontRole);
	}
	return b_match > 0;
}
bool ExpressionProxyModel::lessThan(const QModelIndex &index1, const QModelIndex &index2) const {
	QModelIndex s1 = index1.siblingAtColumn(0);
	QModelIndex s2 = index2.siblingAtColumn(0);
	if(s1.isValid() && s2.isValid()) {
		int i1 = s1.data(MATCH_ROLE).toInt();
		int i2 = s2.data(MATCH_ROLE).toInt();
		if(i1 < i2) return true;
		if(i1 > i2) return false;
	}
	return QSortFilterProxyModel::lessThan(index1, index2);
}
void ExpressionProxyModel::setFilter(std::string sfilter) {
	str = sfilter;
	invalidateFilter();
}

ExpressionEdit::ExpressionEdit(QWidget *parent) : QTextEdit(parent) {
	setAcceptRichText(false);
	completion_blocked = 0;
	cdata = new CompletionData;
	history_index = -1;
	disable_history_arrow_keys = false;
	display_expression_status = true;
	block_display_parse = 0;
	block_text_change = 0;
	dont_change_index = false;
	cursor_has_moved = false;
	expression_has_changed2 = false;
	previous_epos = 0;
	parsed_had_errors = false;
	parsed_had_warnings = false;
	parentheses_highlighted = false;
	setUndoRedoEnabled(true);
	completionModel = new ExpressionProxyModel(cdata, this);
	sourceModel = new QStandardItemModel(this);
	sourceModel->setColumnCount(2);
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
	completionView->setSelectionMode(QAbstractItemView::SingleSelection);
	updateCompletion();
	HTMLDelegate* delegate = new HTMLDelegate();
	completionView->setItemDelegateForColumn(0, delegate);
	completionView->setItemDelegateForColumn(1, delegate);
	completer->setPopup(completionView);
	connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));
	connect(completer, SIGNAL(activated(const QModelIndex&)), this, SLOT(onCompletionActivated(const QModelIndex&)));
}
ExpressionEdit::~ExpressionEdit() {}

#define COMPLETION_APPEND_M(x, y, z, m)		items.clear(); \
						QStandardItem *item = new QStandardItem(x); \
						if(m) { \
							QFont font = completionView->font(); \
							font.setWeight(QFont::Bold); \
							item->setData(font, Qt::FontRole); \
						} else { \
							item->setData(completionView->font(), Qt::FontRole); \
						} \
						item->setData(QVariant::fromValue((void*) NULL), ITEM_ROLE); \
						item->setData(QVariant::fromValue(z), TYPE_ROLE); \
						item->setData(QVariant::fromValue(m), MATCH_ROLE); \
						item->setData(QVariant::fromValue(0), IMATCH_ROLE); \
						items.append(item); \
						item = new QStandardItem(y); \
						item->setData(ifont, Qt::FontRole); \
						items.append(item); \
						sourceModel->appendRow(items);

#define COMPLETION_APPEND(x, y, z, p) 		items.clear(); \
						{QStandardItem *item = new QStandardItem(x); \
						item->setData(completionView->font(), Qt::FontRole); \
						item->setData(QVariant::fromValue((void*) p), ITEM_ROLE); \
						item->setData(QVariant::fromValue(z), TYPE_ROLE); \
						item->setData(QVariant::fromValue(0), MATCH_ROLE); \
						item->setData(QVariant::fromValue(0), IMATCH_ROLE); \
						items.append(item); \
						item = new QStandardItem(y); \
						item->setData(ifont, Qt::FontRole); \
						items.append(item); \
						sourceModel->appendRow(items);}

#define COMPLETION_APPEND_T(x, y, z, p, t) 	items.clear(); \
						QStandardItem *item = new QStandardItem(x); \
						item->setData(completionView->font(), Qt::FontRole); \
						item->setData(QVariant::fromValue((void*) p), ITEM_ROLE); \
						item->setData(QVariant::fromValue(z), TYPE_ROLE); \
						item->setData(QVariant::fromValue(0), MATCH_ROLE); \
						item->setData(QVariant::fromValue(0), IMATCH_ROLE); \
						if(!t.isEmpty()) item->setData("<p>" + t + "</p>", Qt::ToolTipRole);\
						items.append(item); \
						item = new QStandardItem(y); \
						item->setData(ifont, Qt::FontRole); \
						items.append(item); \
						sourceModel->appendRow(items);

#define COMPLETION_APPEND_FLAG(x, y, z, p)	items.clear(); \
						QStandardItem *item = new QStandardItem(x); \
						item->setData(completionView->font(), Qt::FontRole); \
						item->setData(QVariant::fromValue((void*) p), ITEM_ROLE); \
						item->setData(QVariant::fromValue(z), TYPE_ROLE); \
						item->setData(QVariant::fromValue(0), MATCH_ROLE); \
						item->setData(QVariant::fromValue(0), IMATCH_ROLE); \
						items.append(item); \
						item = new QStandardItem(y); \
						item->setData(ifont, Qt::FontRole); \
						item->setData(QIcon(":/data/flags/" + QString::fromStdString(u->referenceName()) + ".png"), Qt::DecorationRole); \
						items.append(item); \
						sourceModel->appendRow(items);
						

void ExpressionEdit::updateCompletion() {
	sourceModel->clear();
	std::string str, title;
	QList<QStandardItem *> items;
	QFont ifont(completionView->font());
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
			COMPLETION_APPEND_T(QString::fromStdString(str), QString::fromStdString(CALCULATOR->functions[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView)), 1, CALCULATOR->functions[i], QString::fromStdString(CALCULATOR->functions[i]->description()))
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
				COMPLETION_APPEND(QString::fromStdString(b ? str : ename_r->name), QString::fromStdString(CALCULATOR->variables[i]->title()), 1, CALCULATOR->variables[i])
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
				COMPLETION_APPEND(QString::fromStdString(b ? str : ename_r->name), QString::fromStdString(title), 1, CALCULATOR->variables[i])
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
				title = u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, completionView);
				if(u->isCurrency()) {
					COMPLETION_APPEND_FLAG(QString::fromStdString(b ? str : ename_r->name), QString::fromStdString(title), 1, u)
				} else {
					if(u->isSIUnit() && !u->category().empty() && title[title.length() - 1] != ')') {
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
					COMPLETION_APPEND(QString::fromStdString(b ? str : ename_r->name), QString::fromStdString(title), 1, u)
				}
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
				COMPLETION_APPEND(QString::fromStdString(str), QString::fromStdString(title), 1, cu)
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
		COMPLETION_APPEND(QString::fromStdString(b ? str : ename_r->name), QString::fromStdString(title), 2, p)
	}
	QString str1, str2;
#define COMPLETION_CONVERT_STRING(x) str1 = tr(x); if(str1 != x) {str1 += " <i>"; str1 += x; str1 += "</i>";}
#define COMPLETION_CONVERT_STRING2(x, y) str1 = tr(x); if(str1 != x) {str1 += " <i>"; str1 += x; str1 += "</i>";} str2 = tr(y); str1 += " <i>"; str1 += str2; str1 += "</i>"; if(str2 != y) {str1 += " <i>"; str1 += y; str1 += "</i>";}
	COMPLETION_CONVERT_STRING2("angle", "phasor")
	COMPLETION_APPEND(str1, tr("Complex angle/phasor notation"), 400, NULL)
	/*COMPLETION_CONVERT_STRING("bases")
	COMPLETION_APPEND(str1, tr("Number bases"), 201, NULL)*/
	COMPLETION_CONVERT_STRING("base")
	COMPLETION_APPEND(str1, tr("Base units"), 101, NULL)
	COMPLETION_CONVERT_STRING("base ")
	COMPLETION_APPEND(str1, tr("Number base"), 200, NULL)
	COMPLETION_CONVERT_STRING("bijective")
	COMPLETION_APPEND(str1, tr("Bijective base-26"), 290, NULL)
	COMPLETION_CONVERT_STRING("binary") str1 += " <i>"; str1 += "bin"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Binary number"), 202, NULL)
	/*COMPLETION_CONVERT_STRING("calendars")
	COMPLETION_APPEND(str1, tr("Calendars"), 500, NULL)*/
	COMPLETION_CONVERT_STRING("cis")
	COMPLETION_APPEND(str1, tr("Complex cis form"), 401, NULL)
	COMPLETION_CONVERT_STRING("decimal") str1 += " <i>"; str1 += "dec"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Decimal number"), 210, NULL)
	COMPLETION_CONVERT_STRING("duodecimal") str1 += " <i>"; str1 += "duo"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Duodecimal number"), 212, NULL)
	COMPLETION_CONVERT_STRING("exponential")
	COMPLETION_APPEND(str1, tr("Complex exponential form"), 402, NULL)
	COMPLETION_CONVERT_STRING("factors")
	COMPLETION_APPEND(str1, tr("Factors"), 600, NULL)
	COMPLETION_CONVERT_STRING("fp16") str1 += " <i>"; str1 += "binary16"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("16-bit floating point binary format"), 310, NULL)
	COMPLETION_CONVERT_STRING("fp32") str1 += " <i>"; str1 += "binary32"; str1 += "</i>"; str1 += " <i>"; str1 += "float"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("32-bit floating point binary format"), 311, NULL)
	COMPLETION_CONVERT_STRING("fp64") str1 += " <i>"; str1 += "binary64"; str1 += "</i>"; str1 += " <i>"; str1 += "double"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("64-bit floating point binary format"), 312, NULL)
	COMPLETION_CONVERT_STRING("fp80");
	COMPLETION_APPEND(str1, tr("80-bit (x86) floating point binary format"), 313, NULL)
	COMPLETION_CONVERT_STRING("fp128") str1 += " <i>"; str1 += "binary128"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("128-bit floating point binary format"), 314, NULL)
	COMPLETION_CONVERT_STRING("fraction")
	COMPLETION_APPEND(str1, tr("Fraction"), 300, NULL)
	COMPLETION_CONVERT_STRING("hexadecimal") str1 += " <i>"; str1 += "hex"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Hexadecimal number"), 216, NULL)
	COMPLETION_CONVERT_STRING("latitude") str1 += " <i>"; str1 += "latitude2"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Latitude"), 294, NULL)
	COMPLETION_CONVERT_STRING("longitude") str1 += " <i>"; str1 += "longitude2"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Longitude"), 294, NULL)
	COMPLETION_CONVERT_STRING("mixed")
	COMPLETION_APPEND(str1, tr("Mixed units"), 102, NULL)
	COMPLETION_CONVERT_STRING("octal") str1 += " <i>"; str1 += "oct"; str1 += "</i>";
	COMPLETION_APPEND(str1, tr("Octal number"), 208, NULL)
	COMPLETION_CONVERT_STRING("optimal")
	COMPLETION_APPEND(str1, tr("Optimal units"), 100, NULL)
	COMPLETION_CONVERT_STRING("partial fraction")
	COMPLETION_APPEND(str1, tr("Expanded partial fraction"), 601, NULL)
	COMPLETION_CONVERT_STRING("polar")
	COMPLETION_APPEND(str1, tr("Complex polar form"), 403, NULL)
	COMPLETION_CONVERT_STRING2("rectangular", "cartesian")
	COMPLETION_APPEND(str1, tr("Complex rectangular form"), 404, NULL)
	COMPLETION_CONVERT_STRING("roman")
	COMPLETION_APPEND(str1, tr("Roman numerals"), 280, NULL)
	COMPLETION_CONVERT_STRING("sexagesimal") str += " <i>"; str += "sexa"; str += "</i>"; str += " <i>"; str += "sexa2"; str += "</i>"; str += " <i>"; str += "sexa3"; str += "</i>";
	COMPLETION_APPEND(str1, tr("Sexagesimal number"), 292, NULL)
	COMPLETION_CONVERT_STRING("time")
	COMPLETION_APPEND(str1, tr("Time format"), 293, NULL)
	COMPLETION_CONVERT_STRING("unicode")
	COMPLETION_APPEND(str1, tr("Unicode"), 281, NULL)
	COMPLETION_CONVERT_STRING("utc")
	COMPLETION_APPEND(str1, tr("UTC time zone"), 501, NULL)
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
void ExpressionEdit::keyReleaseEvent(QKeyEvent *event) {
	disable_history_arrow_keys = false;
	QTextEdit::keyReleaseEvent(event);
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
		}
	}
	if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::KeypadModifier) {
		switch(event->key()) {
			case Qt::Key_Escape: {
				if(CALCULATOR->busy()) {
					CALCULATOR->abort();
				} else if(completionView->isVisible()) {
					hideCompletion();
				} else if(toPlainText().isEmpty()) {
					QApplication::quit();
				} else {
					clear();
				}
				return;
			}
			case Qt::Key_Return: {}
			case Qt::Key_Enter: {
				if(completionView->isVisible() && completionView->currentIndex().isValid()) {
					onCompletionActivated(completionView->currentIndex());
				} else {
					emit returnPressed();
				}
				return;
			}
			case Qt::Key_Up: {
				if(completionView->isVisible() || disable_history_arrow_keys || event->modifiers() == Qt::ShiftModifier) break;
				QTextCursor c = textCursor();
				bool b = (cursor_has_moved && (!c.atStart() || c.hasSelection())) || (!c.atEnd() && !c.atStart());
				if(b || !c.atStart()) {
					QString text = toPlainText();
					if((b && c.atStart()) || text.lastIndexOf("\n", c.position() - 1) >= 0) {
						disable_history_arrow_keys = true;
						break;
					} else {
						QTextCursor c = textCursor();
						c.movePosition(QTextCursor::Start);
						setTextCursor(c);
						disable_history_arrow_keys = true;
						return;
					}
				}
			}
			case Qt::Key_PageUp: {
				if(history_index + 1 < history.size()) {
					if(history_index == -1) current_history = toPlainText();
					history_index++;
					dont_change_index = true;
					blockCompletion();
					if(history_index == -1 && current_history == toPlainText()) history_index = 0;
					if(history_index == -1) setPlainText(current_history);
					else setPlainText(history.at(history_index));
					unblockCompletion();
					QTextCursor c = textCursor();
					c.movePosition(QTextCursor::End);
					setTextCursor(c);
					dont_change_index = false;
				} else {
					break;
				}
				if(event->key() == Qt::Key_Up) cursor_has_moved = false;
				return;
			}
			case Qt::Key_Down: {
				if(completionView->isVisible() || disable_history_arrow_keys) break;
				QTextCursor c = textCursor();
				bool b = (cursor_has_moved && (!c.atEnd() || c.hasSelection())) || (!c.atEnd() && !c.atStart());
				if(b || !c.atEnd()) {
					QString text = toPlainText();
					if((b && c.atEnd()) || text.indexOf("\n", c.position()) >= 0) {
						disable_history_arrow_keys = true;
						break;
					} else {
						QTextCursor c = textCursor();
						c.movePosition(QTextCursor::End);
						setTextCursor(c);
						disable_history_arrow_keys = true;
						return;
					}
				}
			}
			case Qt::Key_PageDown: {
				if(history_index == -1) current_history = toPlainText();
				if(history_index >= -1) history_index--;
				dont_change_index = true;
				blockCompletion();
				if(history_index < 0) {
					if(history_index == -1 && current_history != toPlainText()) setPlainText(current_history);
					else clear();
				} else {
					setPlainText(history.at(history_index));
				}
				QTextCursor c = textCursor();
				c.movePosition(QTextCursor::End);
				setTextCursor(c);
				unblockCompletion();
				if(event->key() == Qt::Key_Up) cursor_has_moved = false;
				return;
			}
			case Qt::Key_Home: {
				QTextCursor c = textCursor();
				c.movePosition(QTextCursor::Start);
				setTextCursor(c);
				return;
			}
			case Qt::Key_End: {
				QTextCursor c = textCursor();
				c.movePosition(QTextCursor::End);
				setTextCursor(c);
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
void ExpressionEdit::blockCompletion() {completionView->hide(); completion_blocked++;}
void ExpressionEdit::unblockCompletion() {completion_blocked--;}
void ExpressionEdit::setStatusText(QString text) {
	if(completionView->isVisible() || text.isEmpty()) {
		QToolTip::hideText();
	} else {
		text.replace("\n", "<br>");
		QToolTip::showText(mapToGlobal(cursorRect().bottomRight()), "<small>" + text + "</small>");
	}
}

bool ExpressionEdit::displayFunctionHint(MathFunction *f, int arg_index) {
	if(!f) return false;
	int iargs = f->maxargs();
	Argument *arg;
	Argument default_arg;
	std::string str, str2, str3;
	const ExpressionName *ename = &f->preferredName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) parent());
	bool last_is_vctr = f->getArgumentDefinition(iargs) && f->getArgumentDefinition(iargs)->type() == ARGUMENT_TYPE_VECTOR;
	if(arg_index > iargs && iargs >= 0 && !last_is_vctr) {
		if(iargs == 1 && f->getArgumentDefinition(1) && f->getArgumentDefinition(1)->handlesVector()) {
			return false;
		}
		setStatusText(tr("Too many arguments for %1().").arg(QString::fromStdString(ename->name)));
		return true;
	}
	str += ename->name;
	if(iargs < 0) {
		iargs = f->minargs() + 1;
		if(arg_index > iargs) arg_index = iargs;
	}
	if(arg_index > iargs && last_is_vctr) arg_index = iargs;
	str += "(";
	int i_reduced = 1;
	if(iargs != 0) {
		for(int i2 = 1; i2 <= iargs; i2++) {
			if(i2 > f->minargs() && arg_index < i2) {
				str += "[";
			}
			if(i2 > 1) {
				str += CALCULATOR->getComma();
				str += " ";
			}
			if(i2 == arg_index) str += "<b>";
			arg = f->getArgumentDefinition(i2);
			if(arg && !arg->name().empty()) {
				str2 = arg->name();
			} else {
				str2 = tr("argument").toStdString();
				str2 += " ";
				str2 += i2s(i2);
			}
			if(i2 == arg_index) {
				if(arg) {
					str3 = arg->printlong();
				} else {
					Argument arg_default;
					str3 = arg_default.printlong();
				}
				if(i_reduced != 2 && settings->printops.use_unicode_signs) {
					gsub(">=", SIGN_GREATER_OR_EQUAL, str3);
					gsub("<=", SIGN_LESS_OR_EQUAL, str3);
					gsub("!=", SIGN_NOT_EQUAL, str3);
				}
				if(!str3.empty()) {
					str2 += ": ";
					str2 += str3;
				}
				gsub("&", "&amp;", str2);
				gsub(">", "&gt;", str2);
				gsub("<", "&lt;", str2);
				str += str2;
				str += "</b>";
			} else {
				gsub("&", "&amp;", str2);
				gsub(">", "&gt;", str2);
				gsub("<", "&lt;", str2);
				str += str2;
				if(i2 > f->minargs() && arg_index < i2) {
					str += "]";
				}
			}
		}
		if(f->maxargs() < 0) {
			str += CALCULATOR->getComma();
			str += " …";
		}
	}
	str += ")";
	setStatusText(QString::fromStdString(str));
	return true;
}
void ExpressionEdit::displayParseStatus() {
	bool prev_func = cdata->current_function;
	cdata->current_function = NULL;
	if(!display_expression_status) return;
	if(block_display_parse) return;
	QString qtext = toPlainText();
	std::string text = qtext.toStdString(), str_f;
	if(text.empty()) {
		setStatusText("");
		prev_parsed_expression = "";
		expression_has_changed2 = false;
		return;
	}
	std::string to_str = CALCULATOR->parseComments(text, settings->evalops.parse_options);
	if(!to_str.empty() && text.empty()) {
		text = CALCULATOR->f_message->referenceName();
		text += "(";
		text += to_str;
		text += ")";
	}
	/*if(text[0] == '/' && text.length() > 1) {
		size_t i = text.find_first_not_of(SPACES, 1);
		if(i != string::npos && text[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, text[i])) {
			setStatusText("qalc command", true, false, false);
			return;
		}
	} else */if(text == "MC") {
		setStatusText(tr("MC (memory clear)"));
		return;
	} else if(text == "MS") {
		setStatusText(tr("MS (memory store)"));
		return;
	} else if(text == "M+") {
		setStatusText(tr("M+ (memory plus)"));
		return;
	} else if(text == "M-" || text == "M−") {
		setStatusText(tr("M− (memory minus)"));
		return;
	}
	std::string parsed_expression, parsed_expression_tooltip;
	remove_duplicate_blanks(text);
	size_t i = text.find_first_of(SPACES LEFT_PARENTHESIS);
	if(i != std::string::npos) {
		str_f = text.substr(0, i);
		if(str_f == "factor" || equalsIgnoreCase(str_f, "factorize") || equalsIgnoreCase(str_f, tr("factorize").toStdString())) {
			text = text.substr(i + 1);
			str_f = tr("factorize").toStdString();
		} else if(equalsIgnoreCase(str_f, "expand") || equalsIgnoreCase(str_f, tr("expand").toStdString())) {
			text = text.substr(i + 1);
			str_f = tr("expand").toStdString();
		} else {
			str_f = "";
		}
	}
	bool last_is_op = last_is_operator(text);
	QTextCursor cursor = textCursor();
	MathStructure mparse, mfunc;
	bool full_parsed = false;
	std::string str_e, str_u, str_w;
	bool had_errors = false, had_warnings = false;
	settings->evalops.parse_options.preserve_format = true;
	//on_display_errors_timeout(NULL);
	//block_error_timeout++;
	if(!cursor.atStart()) {
		settings->evalops.parse_options.unended_function = &mfunc;
		if(cdata->current_from_struct) {cdata->current_from_struct->unref(); cdata->current_from_struct = NULL; cdata->current_from_unit = NULL;}
		if(!cursor.atEnd()) {
			str_e = CALCULATOR->unlocalizeExpression(qtext.left(cursor.position() - 1).toStdString(), settings->evalops.parse_options);
			bool b = CALCULATOR->separateToExpression(str_e, str_u, settings->evalops, false, true);
			b = CALCULATOR->separateWhereExpression(str_e, str_w, settings->evalops) || b;
			if(!b) {
				CALCULATOR->beginTemporaryStopMessages();
				CALCULATOR->parse(&mparse, str_e, settings->evalops.parse_options);
				CALCULATOR->endTemporaryStopMessages();
			}
		} else {
			str_e = CALCULATOR->unlocalizeExpression(text, settings->evalops.parse_options);
			bool b = CALCULATOR->separateToExpression(str_e, str_u, settings->evalops, false, true);
			b = CALCULATOR->separateWhereExpression(str_e, str_w, settings->evalops) || b;
			if(!b) {
				CALCULATOR->parse(&mparse, str_e, settings->evalops.parse_options);
				full_parsed = true;
			}
		}
		settings->evalops.parse_options.unended_function = NULL;
	}
	bool b_func = false;
	if(mfunc.isFunction()) {
		cdata->current_function = mfunc.function();
		if(mfunc.countChildren() == 0) {
			cdata->current_function_index = 1;
			b_func = displayFunctionHint(mfunc.function(), 1);
		} else {
			cdata->current_function_index = mfunc.countChildren();
			b_func = displayFunctionHint(mfunc.function(), mfunc.countChildren());
		}
	}
	if(expression_has_changed2) {
		bool last_is_space = false;
		parsed_expression_tooltip = "";
		if(!full_parsed) {
			str_e = CALCULATOR->unlocalizeExpression(text, settings->evalops.parse_options);
			last_is_space = is_in(SPACES, str_e[str_e.length() - 1]);
			if(CALCULATOR->separateToExpression(str_e, str_u, settings->evalops, false, true) && !str_e.empty()) {
				if(!cdata->current_from_struct) {
					cdata->current_from_struct = new MathStructure;
					EvaluationOptions eo = settings->evalops;
					eo.structuring = STRUCTURING_NONE;
					eo.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
					eo.auto_post_conversion = POST_CONVERSION_NONE;
					eo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					eo.expand = -2;
					if(!CALCULATOR->calculate(cdata->current_from_struct, str_e, 20, eo)) cdata->current_from_struct->setAborted();
					cdata->current_from_unit = CALCULATOR->findMatchingUnit(*cdata->current_from_struct);
				}
			} else if(cdata->current_from_struct) {
				cdata->current_from_struct->unref();
				cdata->current_from_struct = NULL;
				cdata->current_from_unit = NULL;
			}
			CALCULATOR->separateWhereExpression(str_e, str_w, settings->evalops);
			if(!str_e.empty()) CALCULATOR->parse(&mparse, str_e, settings->evalops.parse_options);
		}
		PrintOptions po;
		po.preserve_format = true;
		po.show_ending_zeroes = settings->evalops.parse_options.read_precision != DONT_READ_PRECISION && !CALCULATOR->usesIntervalArithmetic() && settings->evalops.parse_options.base > BASE_CUSTOM;
		po.lower_case_e = settings->printops.lower_case_e;
		po.lower_case_numbers = settings->printops.lower_case_numbers;
		po.base_display = settings->printops.base_display;
		po.twos_complement = settings->printops.twos_complement;
		po.hexadecimal_twos_complement = settings->printops.hexadecimal_twos_complement;
		po.base = settings->evalops.parse_options.base;
		Number nr_base;
		if(po.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
			nr_base = CALCULATOR->customOutputBase();
			CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
		} else if(po.base == BASE_CUSTOM || (po.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26)) {
			po.base = 10;
			po.min_exp = 6;
			po.use_max_decimals = true;
			po.max_decimals = 5;
			po.preserve_format = false;
		}
		po.abbreviate_names = false;
		po.hide_underscore_spaces = true;
		po.use_unicode_signs = settings->printops.use_unicode_signs;
		po.digit_grouping = settings->printops.digit_grouping;
		po.multiplication_sign = settings->printops.multiplication_sign;
		po.division_sign = settings->printops.division_sign;
		po.short_multiplication = false;
		po.excessive_parenthesis = true;
		po.improve_division_multipliers = false;
		po.can_display_unicode_string_function = &can_display_unicode_string_function;
		po.can_display_unicode_string_arg = (void*) parent();
		po.spell_out_logical_operators = settings->printops.spell_out_logical_operators;
		po.restrict_to_parent_precision = false;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		if(str_e.empty()) {
			parsed_expression = "";
		} else {
			CALCULATOR->beginTemporaryStopMessages();
			mparse.format(po);
			parsed_expression = mparse.print(po);
			CALCULATOR->endTemporaryStopMessages();
		}
		if(!str_w.empty()) {
			CALCULATOR->parse(&mparse, str_w, settings->evalops.parse_options);
			parsed_expression += CALCULATOR->localWhereString();
			CALCULATOR->beginTemporaryStopMessages();
			mparse.format(po);
			parsed_expression += mparse.print(po);
			CALCULATOR->endTemporaryStopMessages();
		}
		if(!str_u.empty()) {
			std::string str_u2;
			size_t parse_l = parsed_expression.length();
			bool had_to_conv = false;
			MathStructure *mparse2 = NULL;
			while(true) {
				if(last_is_space) str_u += " ";
				CALCULATOR->separateToExpression(str_u, str_u2, settings->evalops, false, false);
				remove_blank_ends(str_u);
				if(parsed_expression.empty()) {
					parsed_expression += CALCULATOR->localToString(false);
					parsed_expression += " ";
				} else {
					parsed_expression += CALCULATOR->localToString();
				}
				remove_duplicate_blanks(str_u);
				std::string to_str1, to_str2;
				size_t ispace = str_u.find_first_of(SPACES);
				if(ispace != std::string::npos) {
					to_str1 = str_u.substr(0, ispace);
					remove_blank_ends(to_str1);
					to_str2 = str_u.substr(ispace + 1);
					remove_blank_ends(to_str2);
				}
				if(equalsIgnoreCase(str_u, "hex") || equalsIgnoreCase(str_u, "hexadecimal") || equalsIgnoreCase(str_u, tr("hexadecimal").toStdString())) {
					parsed_expression += tr("hexadecimal number").toStdString();
				} else if(equalsIgnoreCase(str_u, "oct") || equalsIgnoreCase(str_u, "octal") || equalsIgnoreCase(str_u, tr("octal").toStdString())) {
					parsed_expression += tr("octal number").toStdString();
				} else if(equalsIgnoreCase(str_u, "dec") || equalsIgnoreCase(str_u, "decimal") || equalsIgnoreCase(str_u, tr("decimal").toStdString())) {
					parsed_expression += tr("decimal number").toStdString();
				} else if(equalsIgnoreCase(str_u, "duo") || equalsIgnoreCase(str_u, "duodecimal") || equalsIgnoreCase(str_u, tr("duodecimal").toStdString())) {
					parsed_expression += tr("duodecimal number").toStdString();
				} else if(equalsIgnoreCase(str_u, "bin") || equalsIgnoreCase(str_u, "binary") || equalsIgnoreCase(str_u, tr("binary").toStdString())) {
					parsed_expression += tr("binary number").toStdString();
				} else if(equalsIgnoreCase(str_u, "roman") || equalsIgnoreCase(str_u, tr("roman").toStdString())) {
					parsed_expression += tr("roman numerals").toStdString();
				} else if(equalsIgnoreCase(str_u, "bijective") || equalsIgnoreCase(str_u, tr("bijective").toStdString())) {
					parsed_expression += tr("bijective base-26").toStdString();
				} else if(equalsIgnoreCase(str_u, "sexa") || equalsIgnoreCase(str_u, "sexa2") || equalsIgnoreCase(str_u, "sexa3") || equalsIgnoreCase(str_u, "sexagesimal") || equalsIgnoreCase(str_u, tr("sexagesimal").toStdString()) || EQUALS_IGNORECASE_AND_LOCAL_NR(str_u, "sexagesimal", tr("sexagesimal"), "2") || EQUALS_IGNORECASE_AND_LOCAL_NR(str_u, "sexagesimal", tr("sexagesimal"), "3")) {
					parsed_expression += tr("sexagesimal number").toStdString();
				} else if(equalsIgnoreCase(str_u, "latitude") || equalsIgnoreCase(str_u, tr("latitude").toStdString()) || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", tr("latitude"), "2")) {
					parsed_expression += tr("latitude").toStdString();
				} else if(equalsIgnoreCase(str_u, "longitude") || equalsIgnoreCase(str_u, tr("longitude").toStdString()) || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", tr("longitude"), "2")) {
					parsed_expression += tr("longitude").toStdString();
				} else if(equalsIgnoreCase(str_u, "fp32") || equalsIgnoreCase(str_u, "binary32") || equalsIgnoreCase(str_u, "float")) {
					parsed_expression += tr("32-bit floating point").toStdString();
				} else if(equalsIgnoreCase(str_u, "fp64") || equalsIgnoreCase(str_u, "binary64") || equalsIgnoreCase(str_u, "double")) {
					parsed_expression += tr("64-bit floating point").toStdString();
				} else if(equalsIgnoreCase(str_u, "fp16") || equalsIgnoreCase(str_u, "binary16")) {
					parsed_expression += tr("16-bit floating point").toStdString();
				} else if(equalsIgnoreCase(str_u, "fp80")) {
					parsed_expression += tr("80-bit (x86) floating point").toStdString();
				} else if(equalsIgnoreCase(str_u, "fp128") || equalsIgnoreCase(str_u, "binary128")) {
					parsed_expression += tr("128-bit floating point").toStdString();
				} else if(equalsIgnoreCase(str_u, "time") || equalsIgnoreCase(str_u, tr("time").toStdString())) {
					parsed_expression += tr("time format").toStdString();
				} else if(equalsIgnoreCase(str_u, "unicode")) {
					parsed_expression += tr("Unicode").toStdString();
				} else if(equalsIgnoreCase(str_u, "bases") || equalsIgnoreCase(str_u, tr("bases").toStdString())) {
					parsed_expression += tr("number bases").toStdString();
				} else if(equalsIgnoreCase(str_u, "calendars") || equalsIgnoreCase(str_u, tr("calendars").toStdString())) {
					parsed_expression += tr("calendars").toStdString();
				} else if(equalsIgnoreCase(str_u, "optimal") || equalsIgnoreCase(str_u, tr("optimal").toStdString())) {
					parsed_expression += tr("optimal unit").toStdString();
				} else if(equalsIgnoreCase(str_u, "base") || equalsIgnoreCase(str_u, tr("base").toStdString())) {
					parsed_expression += tr("base units").toStdString();
				} else if(equalsIgnoreCase(str_u, "mixed") || equalsIgnoreCase(str_u, tr("mixed").toStdString())) {
					parsed_expression += tr("mixed units").toStdString();
				} else if(equalsIgnoreCase(str_u, "fraction") || equalsIgnoreCase(str_u, tr("fraction").toStdString())) {
					parsed_expression += tr("fraction").toStdString();
				} else if(equalsIgnoreCase(str_u, "factors") || equalsIgnoreCase(str_u, tr("factors").toStdString()) || equalsIgnoreCase(str_u, "factor")) {
					parsed_expression += tr("factors").toStdString();
				} else if(equalsIgnoreCase(str_u, "partial fraction") || equalsIgnoreCase(str_u, tr("partial fraction").toStdString())) {
					parsed_expression += tr("expanded partial fractions").toStdString();
				} else if(equalsIgnoreCase(str_u, "rectangular") || equalsIgnoreCase(str_u, "cartesian") || equalsIgnoreCase(str_u, tr("rectangular").toStdString()) || equalsIgnoreCase(str_u, tr("cartesian").toStdString())) {
					parsed_expression += tr("complex rectangular form").toStdString();
				} else if(equalsIgnoreCase(str_u, "exponential") || equalsIgnoreCase(str_u, tr("exponential").toStdString())) {
					parsed_expression += tr("complex exponential form").toStdString();
				} else if(equalsIgnoreCase(str_u, "polar") || equalsIgnoreCase(str_u, tr("polar").toStdString())) {
					parsed_expression += tr("complex polar form").toStdString();
				} else if(str_u == "cis") {
					parsed_expression += tr("complex cis form").toStdString();
				} else if(equalsIgnoreCase(str_u, "angle") || equalsIgnoreCase(str_u, tr("angle").toStdString())) {
					parsed_expression += tr("complex angle notation").toStdString();
				} else if(equalsIgnoreCase(str_u, "phasor") || equalsIgnoreCase(str_u, tr("phasor").toStdString())) {
					parsed_expression += tr("complex phasor notation").toStdString();
				} else if(equalsIgnoreCase(str_u, "utc") || equalsIgnoreCase(str_u, "gmt")) {
					parsed_expression += tr("UTC time zone").toStdString();
				} else if(str_u.length() > 3 && (equalsIgnoreCase(str_u.substr(0, 3), "utc") || equalsIgnoreCase(str_u.substr(0, 3), "gmt"))) {
					str_u = str_u.substr(3);
					parsed_expression += "UTC";
					remove_blanks(str_u);
					bool b_minus = false;
					if(str_u[0] == '+') {
						str_u.erase(0, 1);
					} else if(str_u[0] == '-') {
						b_minus = true;
						str_u.erase(0, 1);
					} else if(str_u.find(SIGN_MINUS) == 0) {
						b_minus = true;
						str_u.erase(0, strlen(SIGN_MINUS));
					}
					unsigned int tzh = 0, tzm = 0;
					int itz = 0;
					if(!str_u.empty() && sscanf(str_u.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
						itz = tzh * 60 + tzm;
					} else {
						had_errors = true;
					}
					if(itz > 0) {
						if(b_minus) parsed_expression += '-';
						else parsed_expression += '+';
						if(itz < 60) {
							parsed_expression += "00";
						} else {
							if(itz < 60 * 10) parsed_expression += '0';
							parsed_expression += i2s(itz / 60);
						}
						if(itz % 60 > 0) {
							parsed_expression += ":";
							if(itz % 60 < 10) parsed_expression += '0';
							parsed_expression += i2s(itz % 60);
						}
					}
				} else if(str_u.length() > 3 && equalsIgnoreCase(str_u.substr(0, 3), "bin") && is_in(NUMBERS, str_u[3])) {
					unsigned int bits = s2i(str_u.substr(3));
					if(bits > 4096) bits = 4096;
					parsed_expression += i2s(bits);
					parsed_expression += "-bit ";
					parsed_expression += tr("binary number").toStdString();
				} else if(str_u.length() > 3 && equalsIgnoreCase(str_u.substr(0, 3), "hex") && is_in(NUMBERS, str_u[3])) {
					unsigned int bits = s2i(str_u.substr(3));
					if(bits > 4096) bits = 4096;
					parsed_expression += i2s(bits);
					parsed_expression += "-bit ";
					parsed_expression += tr("hexadecimal number").toStdString();
				} else if(str_u == "CET") {
					parsed_expression += "UTC";
					parsed_expression += "+01";
				} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, tr("base").toStdString())) {
					parsed_expression += (tr("number base %1").arg(QString::fromStdString(to_str2))).toStdString();
				} else {
					Variable *v = CALCULATOR->getVariable(str_u);
					if(v && !v->isKnown()) v = NULL;
					if(v && CALCULATOR->getUnit(str_u)) v = NULL;
					if(v) {
						mparse = v;
					} else {
						CompositeUnit cu("", "temporary_composite_parse", "", str_u);
						bool b_unit = mparse.containsType(STRUCT_UNIT, false, true, true);
						mparse = cu.generateMathStructure(true);
						mparse.format(po);
						if(!had_to_conv && !mparse.isZero() && !b_unit && !str_e.empty() && str_w.empty()) {
							CALCULATOR->beginTemporaryStopMessages();
							MathStructure to_struct(mparse);
							to_struct.unformat();
							to_struct = CALCULATOR->convertToOptimalUnit(to_struct, settings->evalops, true);
							fix_to_struct_qt(to_struct);
							if(!to_struct.isZero()) {
								mparse2 = new MathStructure();
								CALCULATOR->parse(mparse2, str_e, settings->evalops.parse_options);
								po.preserve_format = false;
								to_struct.format(po);
								po.preserve_format = true;
								if(to_struct.isMultiplication() && to_struct.size() >= 2) {
									if(to_struct[0].isOne()) to_struct.delChild(1, true);
									else if(to_struct[1].isOne()) to_struct.delChild(2, true);
								}
								mparse2->multiply(to_struct);
							}
							CALCULATOR->endTemporaryStopMessages();
						}
					}
					CALCULATOR->beginTemporaryStopMessages();
					parsed_expression += mparse.print(po);
					CALCULATOR->endTemporaryStopMessages();
					if(had_to_conv && mparse2) {
						mparse2->unref();
						mparse2 = NULL;
					}
					had_to_conv = true;
				}
				if(str_u2.empty()) break;
				str_u = str_u2;
			}
			if(mparse2) {
				mparse2->format(po);
				parsed_expression.replace(0, parse_l, mparse2->print(po));
				mparse2->unref();
			}
		}
		if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
		size_t message_n = 0;
		while(CALCULATOR->message()) {
			MessageType mtype = CALCULATOR->message()->type();
			if(mtype == MESSAGE_ERROR || mtype == MESSAGE_WARNING) {
				if(mtype == MESSAGE_ERROR) had_errors = true;
				else had_warnings = true;
				if(message_n > 0) {
					if(message_n == 1) parsed_expression_tooltip = "• " + parsed_expression_tooltip;
					parsed_expression_tooltip += "\n• ";
				}
				parsed_expression_tooltip += CALCULATOR->message()->message();
				message_n++;
			}
			CALCULATOR->nextMessage();
		}
		//block_error_timeout--;
		parsed_had_errors = had_errors; parsed_had_warnings = had_warnings;
		if(!str_f.empty()) {str_f += " "; parsed_expression.insert(0, str_f);}
		gsub("&", "&amp;", parsed_expression);
		gsub(">", "&gt;", parsed_expression);
		gsub("<", "&lt;", parsed_expression);
		if(!last_is_op || (!had_warnings && !had_errors)) {
			if(had_errors || had_warnings) prev_parsed_expression = QString::fromStdString(parsed_expression_tooltip);
			else prev_parsed_expression = QString::fromStdString(parsed_expression);
		}
		if(!b_func) setStatusText(prev_parsed_expression);
		expression_has_changed2 = false;
	} else if(!b_func) {
		CALCULATOR->clearMessages();
		//block_error_timeout--;
		if(prev_func) setStatusText(prev_parsed_expression);
	}
	settings->evalops.parse_options.preserve_format = false;
}


void ExpressionEdit::onTextChanged() {
	if(block_text_change) return;
	if(!dont_change_index) history_index = -1;
	highlightParentheses();
	bool b = completionView->isVisible();
	displayParseStatus();
	if(completion_blocked) hideCompletion();
	else complete();
	qApp->processEvents();
	if(b != completionView->isVisible()) displayParseStatus();
	expression_has_changed2 = true;
}
void ExpressionEdit::complete() {
	if(!cdata->enable_completion) {hideCompletion(); return;}
	setCurrentObject();
	cdata->to_type = 0;
	if(cdata->editing_to_expression && cdata->current_from_struct && cdata->current_from_struct->isDateTime()) cdata->to_type = 3;
	if(current_object_start < 0) {
		if(cdata->editing_to_expression && cdata->current_from_struct && cdata->current_from_unit) {
			cdata->to_type = 4;
		} else if(cdata->editing_to_expression && cdata->editing_to_expression1 && cdata->current_from_struct && cdata->current_from_struct->isNumber()) {
			cdata->to_type = 2;
		} else if(cdata->current_function && cdata->current_function->subtype() == SUBTYPE_DATA_SET && cdata->current_function_index > 1) {
			Argument *arg = cdata->current_function->getArgumentDefinition(cdata->current_function_index);
			if(!arg || arg->type() != ARGUMENT_TYPE_DATA_PROPERTY) {
				hideCompletion();
				return;
			}
		} else if(cdata->to_type < 2) {
			hideCompletion();
			return;
		}
	} else {
		if(current_object_text.length() < (size_t) cdata->completion_min) {hideCompletion(); return;}
	}
	if(cdata->editing_to_expression && cdata->editing_to_expression1 && cdata->current_from_struct) {
		if((cdata->current_from_struct->isUnit() && cdata->current_from_struct->unit()->isCurrency()) || (cdata->current_from_struct->isMultiplication() && cdata->current_from_struct->size() == 2 && (*cdata->current_from_struct)[0].isNumber() && (*cdata->current_from_struct)[1].isUnit() && (*cdata->current_from_struct)[1].unit()->isCurrency())) {
			if(cdata->to_type == 4) cdata->to_type = 5;
			else cdata->to_type = 1;
		}
	}
	if(cdata->to_type < 2 && (current_object_text.empty() || is_in(NUMBERS NOT_IN_NAMES "%", current_object_text[0])) && (!current_object_text.empty() || !cdata->current_function || cdata->current_function->subtype() != SUBTYPE_DATA_SET)) {
		hideCompletion();
		return;
	}
	cdata->exact_prefix_match = QModelIndex();
	cdata->exact_match_found = false;
	cdata->exact_prefix_match_found = false;
	cdata->highest_match = 0;
	cdata->arg = NULL;
	if(cdata->current_function && cdata->current_function->subtype() == SUBTYPE_DATA_SET) {
		cdata->arg = cdata->current_function->getArgumentDefinition(cdata->current_function_index);
		if(cdata->arg && (cdata->arg->type() == ARGUMENT_TYPE_DATA_OBJECT || cdata->arg->type() == ARGUMENT_TYPE_DATA_PROPERTY)) {
			if(cdata->arg->type() == ARGUMENT_TYPE_DATA_OBJECT && (current_object_text.empty() || current_object_text.length() < (size_t) cdata->completion_min)) {hideCompletion(); return;}
			if(cdata->current_function_index == 1) {
				for(size_t i = 1; i <= cdata->current_function->countNames(); i++) {
					if(current_object_text.find(cdata->current_function->getName(i).name) != std::string::npos) {
						cdata->arg = NULL;
						break;
					}
				}
			}
		} else {
			cdata->arg = NULL;
		}
		if(cdata->arg) {
			DataSet *o = NULL;
			if(cdata->arg->type() == ARGUMENT_TYPE_DATA_OBJECT) o = ((DataObjectArgument*) cdata->arg)->dataSet();
			else if(cdata->arg->type() == ARGUMENT_TYPE_DATA_PROPERTY) o = ((DataPropertyArgument*) cdata->arg)->dataSet();
			if(o) {
				QFont ifont(completionView->font());
				ifont.setStyle(QFont::StyleItalic);
				QList<QStandardItem *> items;
				for(int i = 0; i < sourceModel->rowCount();) {
					QModelIndex index = sourceModel->index(i, 0);
					int p_type = index.data(TYPE_ROLE).toInt();
					if(p_type > 2 && p_type < 100) {
						sourceModel->removeRow(i);
					} else {
						i++;
					}
				}
				DataPropertyIter it;
				DataProperty *dp = o->getFirstProperty(&it);
				std::vector<DataObject*> found_objects;
				while(dp) {
					if(cdata->arg->type() == ARGUMENT_TYPE_DATA_OBJECT) {
						if(dp->isKey() && dp->propertyType() == PROPERTY_STRING) {
							DataObjectIter it2;
							DataObject *obj = o->getFirstObject(&it2);
							while(obj) {
								const std::string &name = obj->getProperty(dp);
								int b_match = 0;
								if(equalsIgnoreCase(current_object_text, name, 0, current_object_text.length(), 0)) b_match = name.length() == current_object_text.length() ? 1 : 2;
								for(size_t i = 0; b_match && i < found_objects.size(); i++) {
									if(found_objects[i] == obj) b_match = 0;
								}
								if(b_match) {
									found_objects.push_back(obj);
									DataPropertyIter it3;
									DataProperty *dp2 = o->getFirstProperty(&it3);
									std::string names = name;
									std::string title;
									while(dp2) {
										if(title.empty() && dp2->hasName("name")) {
											title = dp2->getDisplayString(obj->getProperty(dp2));
										}
										if(dp2 != dp && dp2->isKey()) {
											names += " <i>";
											names += dp2->getDisplayString(obj->getProperty(dp2));
											names += "</i>";
										}
										dp2 = o->getNextProperty(&it3);
									}
									COMPLETION_APPEND_M(QString::fromStdString(names), title.empty() ? tr("Data object") : QString::fromStdString(title), 4, b_match)
								}
								obj = o->getNextObject(&it2);
							}
						}
					} else {
						int b_match = 0;
						size_t i_match = 0;
						if(current_object_text.empty()) {
							b_match = 2;
							i_match = 1;
						} else {
							for(size_t i = 1; i <= dp->countNames(); i++) {
								const std::string &name = dp->getName(i);
								if((i_match == 0 || name.length() == current_object_text.length()) && equalsIgnoreCase(current_object_text, name, 0, current_object_text.length(), 0)) {
									b_match = name.length() == current_object_text.length() ? 1 : 2;
									i_match = i;
									if(b_match == 1) break;
								}
							}
						}
						if(b_match) {
							std::string names = dp->getName(i_match);
							for(size_t i = 1; i <= dp->countNames(); i++) {
								if(i != i_match) {
									names += " <i>";
									names += dp->getName(i);
									names += "</i>";
								}
							}
							i_match = 0;
							COMPLETION_APPEND_M(QString::fromStdString(names), QString::fromStdString(dp->title()), 3, b_match)
							if(b_match > cdata->highest_match) cdata->highest_match = b_match;
						}
					}
					dp = o->getNextProperty(&it);
				}
			} else {
				cdata->arg = NULL;
			}
		}
	}
	cdata->p2 = NULL; cdata->p3 = NULL; cdata->p4 = NULL;
	cdata->str2 = ""; cdata->str3 = ""; cdata->str4 = "";
	if(!cdata->arg && current_object_text.length() > (size_t) cdata->completion_min) {
		for(size_t pi = 1; ; pi++) {
			Prefix *prefix = CALCULATOR->getPrefix(pi);
			if(!prefix) break;
			for(size_t name_i = 1; name_i <= prefix->countNames(); name_i++) {
				const std::string *pname = &prefix->getName(name_i).name;
				if(!pname->empty() && pname->length() < current_object_text.length() - cdata->completion_min + 1) {
					bool pmatch = true;
					for(size_t i = 0; i < pname->length(); i++) {
						if((*pname)[i] != current_object_text[i]) {
							pmatch = false;
							break;
						}
					}
					if(pmatch) {
						if(cdata->str2.empty()) {cdata->p2 = prefix; cdata->str2 = current_object_text.substr(pname->length());}
						else if(cdata->str3.empty()) {cdata->p3 = prefix; cdata->str3 = current_object_text.substr(pname->length());}
						else if(cdata->str4.empty()) {cdata->p4 = prefix; cdata->str4 = current_object_text.substr(pname->length());}
					}
				}
			}
		}
	}
	completionModel->setFilter(current_object_text);
	completionModel->sort(1);
	if(completionModel->rowCount() > 0 && cdata->highest_match != 1) {
		completionView->resizeColumnsToContents();
		completionView->resizeRowsToContents();
		QRect rect = cursorRect();
		rect.setWidth(completionView->sizeHint().width());
		completer->complete(rect);
		completionView->clearSelection();
		completionView->setCurrentIndex(QModelIndex());
	} else {
		hideCompletion();
	}
}

void ExpressionEdit::onCursorPositionChanged() {
	if(block_text_change) return;
	cursor_has_moved = true;
	int epos = toPlainText().length() - textCursor().position();
	if(epos == previous_epos) return;
	previous_epos = epos;
	completionView->hide();
	highlightParentheses();
	displayParseStatus();
}
void ExpressionEdit::highlightParentheses() {
	if(textCursor().hasSelection()) return;
	if(parentheses_highlighted) {
		block_text_change++;
		QTextCursor cur = textCursor();
		cur.select(QTextCursor::Document);
		cur.setCharFormat(QTextCharFormat());
		block_text_change--;
		parentheses_highlighted = false;
	}
	int pos = textCursor().position(), ipar2;
	QString text = toPlainText();
	bool b = text.at(pos) == ')';
	if(!b && pos > 0 && text.at(pos - 1) == ')') {
		pos--;
		b = true;
	}
	if(b) {
		ipar2 = pos;
		int pars = 1;
		while(ipar2 > 0) {
			ipar2--;
			if(text.at(ipar2) == ')') pars++;
			else if(text.at(ipar2) == '(') pars--;
			if(pars == 0) break;
		}
		b = (pars == 0);
	} else {
		b = text.at(pos) == '(';
		if(!b && pos > 0 && text.at(pos - 1) == '(') {
			pos--;
			b = true;
		}
		if(b) {
			ipar2 = pos;
			int pars = 1;
			while(ipar2 + 1 < text.length()) {
				ipar2++;
				if(text.at(ipar2) == '(') pars++;
				else if(text.at(ipar2) == ')') pars--;
				if(pars == 0) break;
			}
			b = (pars == 0);
		}
	}
	if(b) {
		block_text_change++;
		QTextCharFormat format;
		if(settings->color == 1) format.setForeground(QColor(0, 128, 0));
		else format.setForeground(QColor(0, 255, 0));
		QTextCursor cur = textCursor();
		cur.setPosition(pos, QTextCursor::MoveAnchor);
		cur.setPosition(pos + 1, QTextCursor::KeepAnchor);
		cur.setCharFormat(format);
		cur.setPosition(ipar2, QTextCursor::MoveAnchor);
		cur.setPosition(ipar2 + 1, QTextCursor::KeepAnchor);
		cur.setCharFormat(format);
		setCurrentCharFormat(QTextCharFormat());
		block_text_change--;
		parentheses_highlighted = true;
	}
}
void ExpressionEdit::onCompletionActivated(const QModelIndex &index_pre) {
	if(!index_pre.isValid()) return;
	QModelIndex index = index_pre.siblingAtColumn(0);
	if(!index.isValid()) return;
	std::string str;
	ExpressionItem *item = NULL;
	Prefix *prefix = NULL;
	int p_type = 0;
	int exp = 1;
	void *p = NULL;
	const ExpressionName *ename = NULL, *ename_r = NULL, *ename_r2;
	int i_type = 0;
	size_t i_match = 0;
	p_type = index.data(TYPE_ROLE).toInt();
	p = index.data(ITEM_ROLE).value<void*>();
	i_match = index.data(IMATCH_ROLE).toInt();
	i_type = index.data(TYPE_ROLE).toULongLong();
	if(i_type == 3) return;
	if(p_type == 1) item = (ExpressionItem*) p;
	else if(p_type == 2) prefix = (Prefix*) p;
	else if(p_type >= 100) p_type = 0;
	if(item && item->type() == TYPE_UNIT && ((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT && (((CompositeUnit*) item)->countUnits() > 1 || !((CompositeUnit*) item)->get(1, &exp, &prefix) || exp != 1)) {
		str = ((Unit*) item)->print(false, true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this);
	} else if(item) {
		CompositeUnit *cu = NULL;
		if(item->type() == TYPE_UNIT && ((Unit*) item)->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			cu = (CompositeUnit*) item;
			item = cu->get(1);
		}
		if(i_type > 2) {
			if(i_match > 0) ename = &item->getName(i_match);
			else ename = &item->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
			if(!ename) return;
			if(cu && prefix) {
				str = prefix->preferredInputName(ename->abbreviation, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this).name;
				str += ename->name;
			} else {
				str = ename->name;
			}
		} else if(cu && prefix) {
			ename_r = &prefix->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
			if(settings->printops.abbreviate_names && ename_r->abbreviation) ename_r2 = &prefix->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
			else ename_r2 = NULL;
			if(ename_r2 == ename_r) ename_r2 = NULL;
			const ExpressionName *ename_i;
			size_t l = 0;
			for(size_t name_i = 0; name_i <= (ename_r2 ? prefix->countNames() + 1 : prefix->countNames()) && l != current_object_text.length(); name_i++) {
				if(name_i == 0) {
					ename_i = ename_r;
				} else if(name_i == 1 && ename_r2) {
					ename_i = ename_r2;
				} else {
					ename_i = &prefix->getName(ename_r2 ? name_i - 1 : name_i);
					if(!ename_i || ename_i == ename_r || ename_i == ename_r2 || (ename_i->name.length() <= l && ename_i->name.length() != current_object_text.length()) || ename_i->plural || (ename_i->unicode && (!settings->printops.use_unicode_signs || !can_display_unicode_string_function(ename_i->name.c_str(), (void*) this)))) {
						ename_i = NULL;
					}
				}
				if(ename_i) {
					if(!((Unit*)item)->useWithPrefixesByDefault() || ename_i->name.length() >= current_object_text.length()) {
						for(size_t i = 0; i < current_object_text.length() && i < ename_i->name.length(); i++) {
							if(ename_i->name[i] != current_object_text[i]) {
								if(i_type != 1 || !equalsIgnoreCase(ename_i->name, current_object_text)) {
									ename_i = NULL;
								}
								break;
							}
						}
					} else {
						ename_i = NULL;
					}
				}
				if(ename_i) {
					l = ename_i->name.length();
					ename = ename_i;
				}
			}
			for(size_t name_i = 1; name_i <= prefix->countNames() && l != current_object_text.length(); name_i++) {
				ename_i = &prefix->getName(name_i);
				if(!ename_i || ename_i == ename_r || ename_i == ename_r2 || (ename_i->name.length() <= l && ename_i->name.length() != current_object_text.length()) || (!ename_i->plural && !(ename_i->unicode && (!settings->printops.use_unicode_signs || !can_display_unicode_string_function(ename_i->name.c_str(), (void*) this))))) {
					ename_i = NULL;
				}
				if(ename_i) {
					if(!((Unit*)item)->useWithPrefixesByDefault() || ename_i->name.length() >= current_object_text.length()) {
						for(size_t i = 0; i < current_object_text.length() && i < ename_i->name.length(); i++) {
							if(ename_i->name[i] != current_object_text[i] && (ename_i->name[i] < 'A' || ename_i->name[i] > 'Z' || ename_i->name[i] != current_object_text[i] + 32) && (ename_i->name[i] < 'a' || ename_i->name[i] > '<' || ename_i->name[i] != current_object_text[i] - 32)) {
								if(i_type != 1 || !equalsIgnoreCase(ename_i->name, current_object_text)) {
									ename_i = NULL;
								}
								break;
							}
						}
					} else {
						ename_i = NULL;
					}
				}
				if(ename_i) {
					l = ename_i->name.length();
					ename = ename_i;
				}
			}
			if(ename && ename->completion_only) {
				ename = &prefix->preferredInputName(ename->abbreviation, settings->printops.use_unicode_signs, ename->plural, false, &can_display_unicode_string_function, (void*) this);
			}
			if(!ename) ename = ename_r;
			if(!ename) return;
			str = ename->name;
			str += item->preferredInputName(settings->printops.abbreviate_names && ename->abbreviation, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this).name;
		} else {
			std::string str2 = current_object_text.substr(i_match);
			ename_r = &item->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
			if(settings->printops.abbreviate_names && ename_r->abbreviation) ename_r2 = &item->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
			else ename_r2 = NULL;
			if(ename_r2 == ename_r) ename_r2 = NULL;
			for(size_t name_i = 0; name_i <= (ename_r2 ? item->countNames() + 1 : item->countNames()) && !ename; name_i++) {
				if(name_i == 0) {
					ename = ename_r;
				} else if(name_i == 1 && ename_r2) {
					ename = ename_r2;
				} else {
					ename = &item->getName(ename_r2 ? name_i - 1 : name_i);
					if(!ename || ename == ename_r || ename == ename_r2 || ename->plural || (ename->unicode && (!settings->printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) this)))) {
						ename = NULL;
					}
				}
				if(ename) {
					if(str2.length() <= ename->name.length()) {
						for(size_t i = 0; i < str2.length(); i++) {
							if(ename->name[i] != str2[i]) {
								if(i_type != 1 || !equalsIgnoreCase(ename->name, str2)) {
									ename = NULL;
								}
								break;
							}
						}
					} else {
						ename = NULL;
					}
				}
			}
			for(size_t name_i = 1; name_i <= item->countNames() && !ename; name_i++) {
				ename = &item->getName(name_i);
				if(!ename || ename == ename_r || ename == ename_r2 || (!ename->plural && !(ename->unicode && (!settings->printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) this))))) {
					ename = NULL;
				}
				if(ename) {
					if(str2.length() <= ename->name.length()) {
						for(size_t i = 0; i < str2.length(); i++) {
							if(ename->name[i] != str2[i] && (ename->name[i] < 'A' || ename->name[i] > 'Z' || ename->name[i] != str2[i] + 32) && (ename->name[i] < 'a' || ename->name[i] > '<' || ename->name[i] != str2[i] - 32)) {
								if(i_type != 1 || !equalsIgnoreCase(ename->name, str2)) {
									ename = NULL;
								}
								break;
							}
						}
					} else {
						ename = NULL;
					}
				}
			}
			if(ename && ename->completion_only) {
				ename = &item->preferredInputName(ename->abbreviation, settings->printops.use_unicode_signs, ename->plural, false, &can_display_unicode_string_function, (void*) this);
			}
			if(!ename) ename = ename_r;
			if(!ename) return;
			str = ename->name;
		}
	} else if(prefix) {
		ename_r = &prefix->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
		if(settings->printops.abbreviate_names && ename_r->abbreviation) ename_r2 = &prefix->preferredInputName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) this);
		else ename_r2 = NULL;
		if(ename_r2 == ename_r) ename_r2 = NULL;
		for(size_t name_i = 0; name_i <= (ename_r2 ? prefix->countNames() + 1 : prefix->countNames()) && !ename; name_i++) {
			if(name_i == 0) {
				ename = ename_r;
			} else if(name_i == 1 && ename_r2) {
				ename = ename_r2;
			} else {
				ename = &prefix->getName(ename_r2 ? name_i - 1 : name_i);
				if(!ename || ename == ename_r || ename == ename_r2 || ename->plural || (ename->unicode && (!settings->printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) this)))) {
					ename = NULL;
				}
			}
			if(ename) {
				if(current_object_text.length() <= ename->name.length()) {
					for(size_t i = 0; i < current_object_text.length(); i++) {
						if(ename->name[i] != current_object_text[i]) {
							if(i_type != 1 || !equalsIgnoreCase(ename->name, current_object_text)) {
								ename = NULL;
							}
							break;
						}
					}
				} else {
					ename = NULL;
				}
			}
		}
		for(size_t name_i = 1; name_i <= prefix->countNames() && !ename; name_i++) {
			ename = &prefix->getName(name_i);
			if(!ename || ename == ename_r || ename == ename_r2 || (!ename->plural && !(ename->unicode && (!settings->printops.use_unicode_signs || !can_display_unicode_string_function(ename->name.c_str(), (void*) this))))) {
				ename = NULL;
			}
			if(ename) {
				if(current_object_text.length() <= ename->name.length()) {
					for(size_t i = 0; i < current_object_text.length(); i++) {
						if(ename->name[i] != current_object_text[i] && (ename->name[i] < 'A' || ename->name[i] > 'Z' || ename->name[i] != current_object_text[i] + 32) && (ename->name[i] < 'a' || ename->name[i] > '<' || ename->name[i] != current_object_text[i] - 32)) {
							if(i_type != 1 || !equalsIgnoreCase(ename->name, current_object_text)) {
								ename = NULL;
							}
							break;
						}
					}
				} else {
					ename = NULL;
				}
			}
		}
		if(ename && (ename->completion_only || (settings->printops.use_unicode_signs && ename->name == "u"))) {
			ename = &prefix->preferredInputName(ename->abbreviation, settings->printops.use_unicode_signs, ename->plural, false, &can_display_unicode_string_function, (void*) this);
		}
		if(!ename) ename = ename_r;
		if(!ename) return;
		str = ename->name;
	} else {
		str = index.data(Qt::DisplayRole).toString().toStdString();
		size_t i = 0;
		size_t i2 = str.find(" <i>");
		while(i_match > 0) {
			if(i == 0) i = i2 + 4;
			else i = i2 + 8;
			if(i >= str.length()) break;
			i2 = str.find("</i>", i);
			if(i2 == std::string::npos) break;
			i_match--;
			if(i == std::string::npos) break;
		}
		if(i2 == std::string::npos) i2 = str.length();
		if(i == std::string::npos) i = 0;
		str = str.substr(i, i2 - i);
	}
	blockCompletion();
	if(current_object_start >= 0) {
		QTextCursor c = textCursor();
		c.setPosition(current_object_start);
		c.setPosition(current_object_end, QTextCursor::KeepAnchor);
		setTextCursor(c);
	}
	int i_move = 0;
	QString text = toPlainText();
	if(item && item->type() == TYPE_FUNCTION) {
		if(text.length() > current_object_end && text[current_object_end] == '(') {
			i_move = 1;
		} else {
			str += "()";
			i_move = -1;
		}
	}
	insertPlainText(QString::fromStdString(str));
	if(i_move != 0) {
		QTextCursor c = textCursor();
		c.setPosition(c.position() + i_move);
		setTextCursor(c);
	}
	hideCompletion();
	unblockCompletion();
	if(!item && !prefix && cdata->editing_to_expression && current_object_end == text.length()) {
		if(str[str.length() - 1] != ' ') emit returnPressed();
	}
}
void ExpressionEdit::hideCompletion() {
	completionView->hide();
}
void ExpressionEdit::addToHistory() {
	history.removeAll(toPlainText());
	history.insert(0, toPlainText());
	history_index = 0;
	cursor_has_moved = false;
}
void ExpressionEdit::setCurrentObject() {
	current_object_start = -1;
	current_object_end = -1;
	current_object_text = "";
	cdata->editing_to_expression = false;
	cdata->editing_to_expression1 = false;
	std::string str;
	size_t l_to = 0, pos = 0, pos2 = 0;
	if(textCursor().position() > 0) {
		QString str = toPlainText();
		current_object_start = textCursor().position();
		current_object_end = current_object_start;
		current_object_text = str.left(current_object_start).toStdString();
		l_to = current_object_text.length();
		pos = current_object_text.length();
		pos2 = pos;
		//if(current_object_text[0] == '/') return;
		for(size_t i = 0; i < l_to; i++) {
			if(current_object_text[i] == '#') {
				current_object_start = -1;
				current_object_end = -1;
				current_object_text = "";
				break;
			}
		}
	}
	if(current_object_start >= 0) {
		if(CALCULATOR->hasToExpression(current_object_text, true, settings->evalops)) {
			cdata->editing_to_expression = true;
			std::string str_to, str_from = current_object_text;
			bool b_space = is_in(SPACES, str_from[str_from.length() - 1]);
			bool b_first = true;
			do {
				CALCULATOR->separateToExpression(str_from, str_to, settings->evalops, true, true);
				if(b_first && str_from.empty()) {
					cdata->current_from_struct = settings->current_result;
					if(cdata->current_from_struct) {
						cdata->current_from_struct->ref();
						cdata->current_from_unit = CALCULATOR->findMatchingUnit(*cdata->current_from_struct);
					}
				}
				b_first = false;
				str_from = str_to;
				if(!str_to.empty() && b_space) str_from += " ";
			} while(CALCULATOR->hasToExpression(str_from, true, settings->evalops));
			l_to = str_to.length();
		}
		bool non_number_before = false;
		size_t l;
		while(pos2 > 0 && l_to > 0) {
			l = 1;
			while(pos2 - l > 0 && (unsigned char) current_object_text[pos2 - l] >= 0x80 && (unsigned char) current_object_text[pos2 - l] < 0xC0) l++;
			pos2 -= l;
			l_to -= l;
			current_object_start--;
			if(!CALCULATOR->utf8_pos_is_valid_in_name((char*) current_object_text.c_str() + sizeof(char) * pos2)) {
				pos2 += l;
				current_object_start++;
				break;
			} else if(l == 1 && is_in(NUMBERS, current_object_text[pos2])) {
				if(non_number_before) {
					pos2++;
					current_object_start++;
					break;
				}
			} else {
				non_number_before = true;
			}
		}
		
		cdata->editing_to_expression1 = (l_to == 0);
		if(pos2 > pos) {
			current_object_start = -1;
			current_object_end = -1;
			current_object_text = "";
		} else {
			std::string str2 = toPlainText().toStdString();
			while(pos < str2.length()) {
				if(!CALCULATOR->utf8_pos_is_valid_in_name((char*) str2.c_str() + sizeof(char) * pos)) {
					break;
				}
				if((unsigned char) str2[pos] >= 0xC0) pos += 2;
				else if((unsigned char) str2[pos] >= 0xE0) pos += 3;
				else if((unsigned char) str2[pos] >= 0xF0) pos += 4;
				else pos++;
				current_object_end++;
			}
			if(pos2 >= pos) {
				current_object_start = -1;
				current_object_end = -1;
				current_object_text = "";
			} else {
				current_object_text = str2.substr(pos2, pos - pos2);
			}
		}
	}
}
