/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextBrowser>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functionsdialog.h"
#include "functioneditdialog.h"

FunctionsDialog::FunctionsDialog(QWidget *parent) : QDialog(parent) {
	QVBoxLayout *topbox = new QVBoxLayout(this);
	setWindowTitle(tr("Functions"));
	QHBoxLayout *hbox = new QHBoxLayout();
	topbox->addLayout(hbox);
	QSplitter *vsplitter = new QSplitter(Qt::Vertical, this);
	hbox->addWidget(vsplitter, 1);
	QSplitter *hsplitter = new QSplitter(Qt::Horizontal, this);
	categoriesView = new QTreeWidget(this);
	categoriesView->setSelectionMode(QAbstractItemView::SingleSelection);
	categoriesView->setRootIsDecorated(false);
	categoriesView->headerItem()->setText(0, tr("Category"));
	hsplitter->addWidget(categoriesView);
	functionsView = new QTreeView(this);
	functionsView->setSelectionMode(QAbstractItemView::SingleSelection);
	functionsView->setRootIsDecorated(false);
	functionsModel = new ItemProxyModel(this);
	sourceModel = new QStandardItemModel(this);
	functionsModel->setSourceModel(sourceModel);
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Function")));
	functionsView->setModel(functionsModel);
	selected_item = NULL;
	hsplitter->addWidget(functionsView);
	vsplitter->addWidget(hsplitter);
	descriptionView = new QTextBrowser(this);
	vsplitter->addWidget(descriptionView);
	vsplitter->setStretchFactor(0, 3);
	vsplitter->setStretchFactor(1, 2);
	hsplitter->setStretchFactor(0, 2);
	hsplitter->setStretchFactor(1, 3);
	QVBoxLayout *box = new QVBoxLayout();
	newButton = new QPushButton(tr("New"), this); box->addWidget(newButton); connect(newButton, SIGNAL(clicked()), this, SLOT(newClicked()));
	editButton = new QPushButton(tr("Edit"), this); box->addWidget(editButton); connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
	delButton = new QPushButton(tr("Delete"), this); box->addWidget(delButton); connect(delButton, SIGNAL(clicked()), this, SLOT(delClicked()));
	deactivateButton = new QPushButton(tr("Deactivate"), this); box->addWidget(deactivateButton); connect(deactivateButton, SIGNAL(clicked()), this, SLOT(deactivateClicked()));
	box->addSpacing(24);
	insertButton = new QPushButton(tr("Calculate"), this); box->addWidget(insertButton); connect(insertButton, SIGNAL(clicked()), this, SLOT(insertClicked()));
	applyButton = new QPushButton(tr("Apply"), this); box->addWidget(applyButton); connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
	box->addStretch(1);
	hbox->addLayout(box, 0);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	topbox->addWidget(buttonBox);
	selected_category = "All";
	updateFunctions();
	functionsModel->setFilter("All");
	functionsModel->sort(0);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(categoriesView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(functionsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedFunctionChanged(const QModelIndex&, const QModelIndex&)));
	resize(900, 800);
}
FunctionsDialog::~FunctionsDialog() {}

void FunctionsDialog::newClicked() {
	UserFunction *f = FunctionEditDialog::newFunction(this);
	if(f) {
		selected_item = f;
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true)));
		item->setData(QVariant::fromValue((void*) f), Qt::UserRole);
		sourceModel->appendRow(item);
		functionsView->selectionModel()->clear();
		functionsView->selectionModel()->setCurrentIndex(functionsModel->mapFromSource(item->index()), QItemSelectionModel::SelectCurrent);
		selectedCategoryChanged(categoriesView->currentItem(), NULL);
		emit itemsChanged();
	}
}
void FunctionsDialog::editClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f && FunctionEditDialog::editFunction(this, f)) {
		sourceModel->removeRow(functionsModel->mapToSource(functionsView->selectionModel()->currentIndex()).row());
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true)));
		item->setData(QVariant::fromValue((void*) f), Qt::UserRole);
		sourceModel->appendRow(item);
		selected_item = f;
		functionsView->selectionModel()->clear();
		functionsView->selectionModel()->setCurrentIndex(functionsModel->mapFromSource(item->index()), QItemSelectionModel::SelectCurrent);
		selectedCategoryChanged(categoriesView->currentItem(), NULL);
		emit itemsChanged();
	}
}
void FunctionsDialog::delClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f && f->isLocal()) {
		sourceModel->removeRow(functionsModel->mapToSource(functionsView->selectionModel()->currentIndex()).row());
		selected_item = NULL;
		f->destroy();
		emit itemsChanged();
	}
}
void FunctionsDialog::applyClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	emit applyFunctionRequest(f);
}
void FunctionsDialog::insertClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f) {
		emit insertFunctionRequest(f, this);
	}
}
void FunctionsDialog::deactivateClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f) {
		f->setActive(!f->isActive());
		selectedCategoryChanged(categoriesView->currentItem(), NULL);
		emit itemsChanged();
	}
}
void FunctionsDialog::selectedFunctionChanged(const QModelIndex &index, const QModelIndex&) {
	if(index.isValid()) {
		MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
		if(CALCULATOR->stillHasFunction(f)) {
			selected_item = f;
			Argument *arg;
			Argument default_arg;
			std::string str;
			const ExpressionName *ename = &f->preferredName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) descriptionView);
			str = "<b><i>";
			str += ename->name;
			str += "</b>";
			int iargs = f->maxargs();
			if(iargs < 0) {
				iargs = f->minargs() + 1;
			}
			str += "(";
			if(iargs != 0) {
				for(int i2 = 1; i2 <= iargs; i2++) {
					if(i2 > f->minargs()) {
						str += "[";
					}
					if(i2 > 1) {
						str += CALCULATOR->getComma();
						str += " ";
					}
					arg = f->getArgumentDefinition(i2);
					if(arg && !arg->name().empty()) {
						str += arg->name();
					} else {
						str += tr("argument").toStdString();
						str += " ";
						str += i2s(i2);
					}
					if(i2 > f->minargs()) {
						str += "]";
					}
				}
				if(f->maxargs() < 0) {
					str += CALCULATOR->getComma();
					str += " …";
				}
			}
			str += ")";
			for(size_t i2 = 1; i2 <= f->countNames(); i2++) {
				if(&f->getName(i2) != ename) {
					str += "<br>";
					str += f->getName(i2).name;
				}
			}
			str += "</i><br>";
			if(f->subtype() == SUBTYPE_DATA_SET) {
				str += "<br>";
				str += tr("Retrieves data from the %1 data set for a given object and property. If \"info\" is typed as property, a dialog window will pop up with all properties of the object.").arg(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) descriptionView))).toStdString();
				str += "<br>";
			}
			if(!f->description().empty()) {
				str += "<br>";
				str += f->description();
				str += "<br>";
			}
			if(!f->example(true).empty()) {
				str += "<br>";
				str += tr("Example:").toStdString();
				str += " ";
				str += f->example(false, ename->name);
				str += "<br>";
			}
			if(f->subtype() == SUBTYPE_DATA_SET && !((DataSet*) f)->copyright().empty()) {
				str += "<br>";
				str += ((DataSet*) f)->copyright();
				str += "<br>";
			}
			if(iargs) {
				str += "<br><b>";
				str += tr("Arguments").toStdString();
				str += "</b><br>";
				for(int i2 = 1; i2 <= iargs; i2++) {
					arg = f->getArgumentDefinition(i2);
					if(arg && !arg->name().empty()) {
						str += arg->name();
					} else {
						str += i2s(i2);
					}
					str += ": <i>";
					if(arg) {
						str += arg->printlong();
					} else {
						str += default_arg.printlong();
					}
					if(i2 > f->minargs()) {
						str += " (";
						str += tr("optional", "optional argument").toStdString();
						if(!f->getDefaultValue(i2).empty() && f->getDefaultValue(i2) != "\"\"") {
							str += ", ";
							str += tr("default: ", "argument default").toStdString();
							ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
							str += CALCULATOR->localizeExpression(f->getDefaultValue(i2), pa);
						}
						str += ")";
					}
					str += "</i><br>";
				}
			}
			if(!f->condition().empty()) {
				str += "<br>";
				str += tr("Requirement").toStdString();
				str += ": ";
				str += f->printCondition();
				str += "<br>";
			}
			if(f->subtype() == SUBTYPE_DATA_SET) {
				DataSet *ds = (DataSet*) f;
				str += "<br><b>";
				str += tr("Properties").toStdString();
				str += "</b><br>";
				DataPropertyIter it;
				DataProperty *dp = ds->getFirstProperty(&it);
				while(dp) {
					if(!dp->isHidden()) {
						if(!dp->title(false).empty()) {
							str += dp->title();
							str += ": ";
						}
						for(size_t i = 1; i <= dp->countNames(); i++) {
							if(i > 1) str += ", ";
							str += dp->getName(i);
						}
						if(dp->isKey()) {
							str += " (";
							//indicating that the property is a data set key
							str += tr("key").toStdString();
							str += ")";
						}
						str += "<br>";
						if(!dp->description().empty()) {
							str += "<i>";
							str += dp->description();
							str += "</i><br>";
						}
					}
					dp = ds->getNextProperty(&it);
				}
			}
			if(settings->printops.use_unicode_signs) {
				gsub(">=", SIGN_GREATER_OR_EQUAL, str);
				gsub("<=", SIGN_LESS_OR_EQUAL, str);
				gsub("!=", SIGN_NOT_EQUAL, str);
			}
			if(f->isActive() != (deactivateButton->text() == tr("Deactivate"))) {
				deactivateButton->setMinimumWidth(deactivateButton->width());
				if(f->isActive()) {
					deactivateButton->setText(tr("Deactivate"));
				} else {
					deactivateButton->setText(tr("Activate"));
				}
			}
			editButton->setEnabled(!f->isBuiltin());
			insertButton->setEnabled(f->isActive());
			delButton->setEnabled(f->isLocal());
			deactivateButton->setEnabled(true);
			applyButton->setEnabled(f->isActive() && (f->minargs() <= 1 || settings->rpn_mode));
			descriptionView->setHtml(QString::fromStdString(str));
		}
		return;
	}
	editButton->setEnabled(false);
	insertButton->setEnabled(false);
	delButton->setEnabled(false);
	deactivateButton->setEnabled(false);
	applyButton->setEnabled(false);
	descriptionView->clear();
	selected_item = NULL;
}
void FunctionsDialog::selectedCategoryChanged(QTreeWidgetItem *iter, QTreeWidgetItem*) {
	if(!iter) selected_category = "";
	else selected_category = iter->text(1).toStdString();
	functionsModel->setFilter(selected_category);
	functionsModel->sort(0);
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(index.isValid()) {
		selectedFunctionChanged(index, QModelIndex());
		functionsView->scrollTo(index);
	}
}

struct tree_struct {
	std::string item;
	std::list<tree_struct> items;
	std::list<tree_struct>::iterator it;
	std::list<tree_struct>::reverse_iterator rit;

	tree_struct *parent;
	void sort() {
	items.sort();
	for(std::list<tree_struct>::iterator it = items.begin(); it != items.end(); ++it) {
			it->sort();
		}
	}
	bool operator < (const tree_struct &s1) const {
		return string_is_less(item, s1.item);
	}
};
void FunctionsDialog::updateFunctions() {

	size_t cat_i, cat_i_prev;
	bool b;
	std::string str, cat, cat_sub;
	tree_struct function_cats;
	function_cats.parent = NULL;
	bool has_inactive = false, has_uncat = false;
	std::list<tree_struct>::iterator it;

	sourceModel->clear();
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Function")));

	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		MathFunction *f = CALCULATOR->functions[i];
		if(!f->isActive()) {
			has_inactive = true;
		} else {
			tree_struct *item = &function_cats;
			if(!f->category().empty()) {
				cat = f->category();
				cat_i = cat.find("/"); cat_i_prev = 0;
				b = false;
				while(true) {
					if(cat_i == std::string::npos) {
						cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
					} else {
						cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
					}
					b = false;
					for(it = item->items.begin(); it != item->items.end(); ++it) {
						if(cat_sub == it->item) {
							item = &*it;
							b = true;
							break;
						}
					}
					if(!b) {
						tree_struct cat;
						item->items.push_back(cat);
						it = item->items.end();
						--it;
						it->parent = item;
						item = &*it;
						item->item = cat_sub;
					}
					if(cat_i == std::string::npos) {
						break;
					}
					cat_i_prev = cat_i + 1;
					cat_i = cat.find("/", cat_i_prev);
				}
			} else if(!f->isLocal()) {
				has_uncat = true;
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true)));
		item->setData(QVariant::fromValue((void*) f), Qt::UserRole);
		sourceModel->appendRow(item);
		if(f == selected_item) functionsView->selectionModel()->setCurrentIndex(functionsModel->mapFromSource(item->index()), QItemSelectionModel::SelectCurrent);
	}
	function_cats.sort();

	categoriesView->clear();
	QTreeWidgetItem *iter, *iter2, *iter3;
	QStringList l;
	l << tr("All", "All functions"); l << "All";
	iter3 = new QTreeWidgetItem(categoriesView, l);
	tree_struct *item, *item2;
	function_cats.it = function_cats.items.begin();
	if(function_cats.it != function_cats.items.end()) {
		item = &*function_cats.it;
		++function_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	while(item) {
		str += "/";
		str += item->item;
		l.clear(); l << QString::fromStdString(item->item); l << QString::fromStdString(str);
		iter = new QTreeWidgetItem(iter2, l);
		if(str == selected_category) {
			iter->setExpanded(true);
			iter->setSelected(true);
		}
		while(item && item->it == item->items.end()) {
			size_t str_i = str.rfind("/");
			if(str_i == std::string::npos) {
				str = "";
			} else {
				str = str.substr(0, str_i);
			}
			item = item->parent;
			iter2 = iter->parent();
			iter = iter2;
		}
		if(item) {
			item2 = &*item->it;
			if(item->it == item->items.begin()) iter2 = iter;
			++item->it;
			item = item2;
			item->it = item->items.begin();
		}
	}
	if(has_uncat) {
		//add "Uncategorized" category if there are functions without category
		l.clear(); l << tr("Uncategorized"); l << "Uncategorized";
		iter = new QTreeWidgetItem(iter3, l);
		if(selected_category == "Uncategorized") {
			iter->setSelected(true);
		}
	}
	l.clear(); l << tr("User functions"); l << "User functions";
	iter = new QTreeWidgetItem(iter3, l);
	if(selected_category == "User functions") {
		iter->setSelected(true);
	}
	if(has_inactive) {
		//add "Inactive" category if there are inactive functions
		l.clear(); l << tr("Inactive"); l << "Inactive";
		iter = new QTreeWidgetItem(categoriesView, l);
		if(selected_category == "Inactive") {
			iter->setSelected(true);
		}
	}
	if(categoriesView->selectedItems().isEmpty()) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_category = "All";
		iter3->setExpanded(true);
		iter3->setSelected(true);
	}
}

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
	if(cat == "All") return item->isActive();
	if(cat == "Inactive") return !item->isActive();
	if(cat == "Uncategorized") return item->isActive() && item->category().empty() && !item->isLocal();
	if(cat == "User functions") return item->isActive() && item->isLocal();
	if(!item->isActive()) return false;
	if(!subcat.empty()) {
		size_t l1 = subcat.length(), l2;
		l2 = item->category().length();
		return (l2 == l1 || (l2 > l1 && item->category()[l1] == '/')) && item->category().substr(0, l1) == subcat;
	} else {
		return item->category() == cat;
	}
}
void ItemProxyModel::setFilter(std::string sfilter) {
	cat = sfilter;
	if(cat[0] == '/') subcat = cat.substr(1, cat.length() - 1);
	else subcat = "";
	invalidateFilter();
}

