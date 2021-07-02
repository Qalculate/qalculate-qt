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
#include <QTextEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "variablesdialog.h"
#include "itemproxymodel.h"
#include "unknowneditdialog.h"
#include "variableeditdialog.h"
#include "csvdialog.h"

VariablesDialog::VariablesDialog(QWidget *parent) : QDialog(parent) {
	QVBoxLayout *topbox = new QVBoxLayout(this);
	setWindowTitle(tr("Variables"));
	QHBoxLayout *hbox = new QHBoxLayout();
	topbox->addLayout(hbox);
	vsplitter = new QSplitter(Qt::Vertical, this);
	hbox->addWidget(vsplitter, 1);
	hsplitter = new QSplitter(Qt::Horizontal, this);
	categoriesView = new QTreeWidget(this);
	categoriesView->setSelectionMode(QAbstractItemView::SingleSelection);
	categoriesView->setRootIsDecorated(false);
	categoriesView->headerItem()->setText(0, tr("Category"));
	categoriesView->setColumnCount(2);
	categoriesView->setColumnHidden(1, true);
	hsplitter->addWidget(categoriesView);
	QWidget *w = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout(w);
	vbox->setSpacing(0);
	vbox->setContentsMargins(0, 0, 0, 0);
	variablesView = new QTreeView(this);
	variablesView->setSelectionMode(QAbstractItemView::SingleSelection);
	variablesView->setRootIsDecorated(false);
	variablesModel = new ItemProxyModel(this);
	sourceModel = new QStandardItemModel(this);
	variablesModel->setSourceModel(sourceModel);
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Variable")));
	variablesView->setModel(variablesModel);
	selected_item = NULL;
	vbox->addWidget(variablesView, 1);
	searchEdit = new QLineEdit(this);
	searchEdit->addAction(LOAD_ICON("edit-find"), QLineEdit::LeadingPosition);
	vbox->addWidget(searchEdit, 0);
	hsplitter->addWidget(w);
	vsplitter->addWidget(hsplitter);
	descriptionView = new QTextEdit(this);
	descriptionView->setReadOnly(true);
	vsplitter->addWidget(descriptionView);
	vsplitter->setStretchFactor(0, 3);
	vsplitter->setStretchFactor(1, 1);
	hsplitter->setStretchFactor(0, 2);
	hsplitter->setStretchFactor(1, 3);
	QVBoxLayout *box = new QVBoxLayout();
	newButton = new QPushButton(tr("New"), this); box->addWidget(newButton);
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Variable/Constant…"), this, SLOT(newVariable()));
	menu->addAction(tr("Unknown Variable…"), this, SLOT(newUnknown()));
	menu->addAction(tr("Matrix…"), this, SLOT(newMatrix()));
	newButton->setMenu(menu);
	editButton = new QPushButton(tr("Edit…"), this); box->addWidget(editButton); connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
	exportButton = new QPushButton(tr("Export…"), this); box->addWidget(exportButton); connect(exportButton, SIGNAL(clicked()), this, SLOT(exportClicked()));
	deactivateButton = new QPushButton(tr("Deactivate"), this); box->addWidget(deactivateButton); connect(deactivateButton, SIGNAL(clicked()), this, SLOT(deactivateClicked()));
	delButton = new QPushButton(tr("Delete"), this); box->addWidget(delButton); connect(delButton, SIGNAL(clicked()), this, SLOT(delClicked()));
	box->addSpacing(24);
	insertButton = new QPushButton(tr("Insert"), this); box->addWidget(insertButton); connect(insertButton, SIGNAL(clicked()), this, SLOT(insertClicked()));
	insertButton->setDefault(true);
	box->addStretch(1);
	hbox->addLayout(box, 0);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	topbox->addWidget(buttonBox);
	selected_category = "All";
	updateVariables();
	variablesModel->setFilter("All");
	connect(searchEdit, SIGNAL(textEdited(const QString&)), this, SLOT(searchChanged(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(categoriesView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(variablesView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedVariableChanged(const QModelIndex&, const QModelIndex&)));
	connect(variablesView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(insertClicked()));
	selectedVariableChanged(QModelIndex(), QModelIndex());
	if(!settings->variables_geometry.isEmpty()) restoreGeometry(settings->variables_geometry);
	else resize(900, 700);
	if(!settings->variables_vsplitter_state.isEmpty()) vsplitter->restoreState(settings->variables_vsplitter_state);
	if(!settings->variables_hsplitter_state.isEmpty()) hsplitter->restoreState(settings->variables_hsplitter_state);
}
VariablesDialog::~VariablesDialog() {}

void VariablesDialog::keyPressEvent(QKeyEvent *event) {
	if(event->matches(QKeySequence::Find)) {
		searchEdit->setFocus();
		return;
	}
	if(event->key() == Qt::Key_Escape && searchEdit->hasFocus()) {
		searchEdit->clear();
		variablesView->setFocus();
		return;
	}
	if(event->key() == Qt::Key_Return && variablesView->hasFocus()) {
		QModelIndex index = variablesView->selectionModel()->currentIndex();
		if(index.isValid()) {
			insertClicked();
			return;
		}
	}
	QDialog::keyPressEvent(event);
}
void VariablesDialog::closeEvent(QCloseEvent *e) {
	settings->variables_geometry = saveGeometry();
	settings->variables_vsplitter_state = vsplitter->saveState();
	settings->variables_hsplitter_state = hsplitter->saveState();
	QDialog::closeEvent(e);
}
void VariablesDialog::reject() {
	settings->variables_geometry = saveGeometry();
	settings->variables_vsplitter_state = vsplitter->saveState();
	settings->variables_hsplitter_state = hsplitter->saveState();
	QDialog::reject();
}
void VariablesDialog::searchChanged(const QString &str) {
	variablesModel->setSecondaryFilter(str.toStdString());
	variablesView->selectionModel()->setCurrentIndex(variablesModel->index(0, 0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
}
void VariablesDialog::newMatrix() {newVariable(2);}
void VariablesDialog::newVariable() {newVariable(0);}
void VariablesDialog::newVariable(int type) {
	ExpressionItem *replaced_item = NULL;
	Variable *v;
	if(type == 2) v = VariableEditDialog::newMatrix(this, &replaced_item);
	else if(type == 1) v = UnknownEditDialog::newVariable(this, &replaced_item);
	else v = VariableEditDialog::newVariable(this, NULL, QString(), &replaced_item);
	if(v) {
		if(replaced_item && (replaced_item == v || !item_in_calculator(replaced_item))) {
			QModelIndexList list = sourceModel->match(sourceModel->index(0, 0), Qt::UserRole, QVariant::fromValue((void*) replaced_item), 1, Qt::MatchExactly);
			if(!list.isEmpty()) sourceModel->removeRow(list[0].row());
		}
		selected_item = v;
		QStandardItem *item = new QStandardItem(QString::fromStdString(v->title(true)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) v), Qt::UserRole);
		sourceModel->appendRow(item);
		if(selected_category != "All" && selected_category != "User items" && selected_category != std::string("/") + v->category()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("User items", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(!list.isEmpty()) {
				categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			}
		} else {
			variablesModel->invalidate();
		}
		sourceModel->sort(0);
		QModelIndex index = variablesModel->mapFromSource(item->index());
		if(index.isValid()) {
			variablesView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			variablesView->scrollTo(index);
		}
		emit itemsChanged();
	}
}
void VariablesDialog::newUnknown() {newVariable(1);}
void VariablesDialog::variableRemoved(Variable *v) {
	QModelIndexList list = sourceModel->match(sourceModel->index(0, 0), Qt::UserRole, QVariant::fromValue((void*) v), 1, Qt::MatchExactly);
	if(!list.isEmpty()) sourceModel->removeRow(list[0].row());
}
void VariablesDialog::editClicked() {
	QModelIndex index = variablesView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Variable *v = (Variable*) index.data(Qt::UserRole).value<void*>();
	if(!v) return;
	bool b = false;
	ExpressionItem *replaced_item = NULL;
	if(v->isKnown()) {
		b = VariableEditDialog::editVariable(this, (KnownVariable*) v, &replaced_item);
	} else {
		b = UnknownEditDialog::editVariable(this, (UnknownVariable*) v, &replaced_item);
	}
	if(b) {
		sourceModel->removeRow(variablesModel->mapToSource(variablesView->selectionModel()->currentIndex()).row());
		if(replaced_item) {
			if(!CALCULATOR->stillHasUnit((Unit*) replaced_item)) {
				emit unitRemoved((Unit*) replaced_item);
			} else if(!CALCULATOR->stillHasVariable((Variable*) replaced_item) || (replaced_item->type() == TYPE_VARIABLE && !CALCULATOR->hasVariable((Variable*) replaced_item))) {
				QModelIndexList list = sourceModel->match(sourceModel->index(0, 0), Qt::UserRole, QVariant::fromValue((void*) replaced_item), 1, Qt::MatchExactly);
				if(!list.isEmpty()) sourceModel->removeRow(list[0].row());
			} else if(replaced_item->type() == TYPE_UNIT && !CALCULATOR->hasUnit((Unit*) replaced_item)) {
				emit unitRemoved((Unit*) replaced_item);
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(v->title(true)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) v), Qt::UserRole);
		sourceModel->appendRow(item);
		selected_item = v;
		if(selected_category != "All" && selected_category != "User items" && selected_category != std::string("/") + v->category()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("User items", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(!list.isEmpty()) {
				categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			}
		} else {
			variablesModel->invalidate();
		}
		sourceModel->sort(0);
		QModelIndex index = variablesModel->mapFromSource(item->index());
		if(index.isValid()) {
			variablesView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			variablesView->scrollTo(index);
		}
		emit itemsChanged();
	}
}
void VariablesDialog::delClicked() {
	QModelIndex index = variablesView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Variable *v = (Variable*) index.data(Qt::UserRole).value<void*>();
	if(v && v->isLocal()) {
		sourceModel->removeRow(variablesModel->mapToSource(variablesView->selectionModel()->currentIndex()).row());
		selected_item = NULL;
		v->destroy();
		emit itemsChanged();
	}
}
void VariablesDialog::insertClicked() {
	QModelIndex index = variablesView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Variable *v = (Variable*) index.data(Qt::UserRole).value<void*>();
	if(v) {
		emit insertVariableRequest(v);
	}
}
void VariablesDialog::exportClicked() {
	QModelIndex index = variablesView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Variable *v = (Variable*) index.data(Qt::UserRole).value<void*>();
	if(v && v->isKnown()) CSVDialog::exportCSVFile(this, NULL, (KnownVariable*) v);
}
void VariablesDialog::deactivateClicked() {
	QModelIndex index = variablesView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Variable *v = (Variable*) index.data(Qt::UserRole).value<void*>();
	if(v) {
		v->setActive(!v->isActive());
		variablesModel->invalidate();
		emit itemsChanged();
	}
}
void VariablesDialog::selectedVariableChanged(const QModelIndex &index, const QModelIndex&) {
	if(index.isValid()) {
		Variable *v = (Variable*) index.data(Qt::UserRole).value<void*>();
		if(CALCULATOR->stillHasVariable(v)) {
			selected_item = v;
			std::string str;
			const ExpressionName *ename = &v->preferredName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) descriptionView);
			str = "<b>";
			str += ename->name;
			str += "</b>";
			for(size_t i2 = 1; i2 <= v->countNames(); i2++) {
				if(&v->getName(i2) != ename) {
					str += ", ";
					str += v->getName(i2).name;
				}
			}
			str += "<br><br>";
			if(v->isKnown()) {
				bool is_approximate = false;
				if(((KnownVariable*) v)->get().isMatrix() && ((KnownVariable*) v)->get().columns() * ((KnownVariable*) v)->get().rows() > 16) {
					str += tr("a matrix").toStdString();
				} else if(((KnownVariable*) v)->get().isVector() && ((KnownVariable*) v)->get().size() > 10) {
					str += tr("a vector").toStdString();
				} else {
					PrintOptions po = settings->printops;
					po.can_display_unicode_string_arg = (void*) descriptionView;
					po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
					po.base = 10;
					po.number_fraction_format = FRACTION_DECIMAL_EXACT;
					po.allow_non_usable = true;
					po.is_approximate = &is_approximate;
					if(v->isApproximate() || is_approximate) str += SIGN_ALMOST_EQUAL " ";
					else str += "= ";
					str += CALCULATOR->print(((KnownVariable*) v)->get(), 1000, po, true, settings->colorize_result ? settings->color : 0, TAG_TYPE_HTML);
				}
			} else {
				if(((UnknownVariable*) v)->assumptions()) {
					QString value;
					if(((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_BOOLEAN) {
						switch(((UnknownVariable*) v)->assumptions()->sign()) {
							case ASSUMPTION_SIGN_POSITIVE: {value = tr("positive"); break;}
							case ASSUMPTION_SIGN_NONPOSITIVE: {value = tr("non-positive"); break;}
							case ASSUMPTION_SIGN_NEGATIVE: {value = tr("negative"); break;}
							case ASSUMPTION_SIGN_NONNEGATIVE: {value = tr("non-negative"); break;}
							case ASSUMPTION_SIGN_NONZERO: {value = tr("non-zero"); break;}
							default: {}
						}
					}
					if(!value.isEmpty() && ((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_NONE) value += " ";
					switch(((UnknownVariable*) v)->assumptions()->type()) {
						case ASSUMPTION_TYPE_INTEGER: {value += tr("integer"); break;}
						case ASSUMPTION_TYPE_BOOLEAN: {value += tr("boolean"); break;}
						case ASSUMPTION_TYPE_RATIONAL: {value += tr("rational"); break;}
						case ASSUMPTION_TYPE_REAL: {value += tr("real"); break;}
						case ASSUMPTION_TYPE_COMPLEX: {value += tr("complex"); break;}
						case ASSUMPTION_TYPE_NUMBER: {value += tr("number"); break;}
						case ASSUMPTION_TYPE_NONMATRIX: {value += tr("not matrix"); break;}
						default: {}
					}
					if(value.isEmpty()) value = tr("unknown");
					str += value.toStdString();
				} else {
					str += tr("Default assumptions").toStdString();
				}
			}
			if(!v->description().empty()) {
				str += "<br><br>";
				str += v->description();
			}
			if(v->isActive() != (deactivateButton->text() == tr("Deactivate"))) {
				deactivateButton->setMinimumWidth(deactivateButton->width());
				if(v->isActive()) {
					deactivateButton->setText(tr("Deactivate"));
				} else {
					deactivateButton->setText(tr("Activate"));
				}
			}
			editButton->setEnabled(!v->isBuiltin());
			exportButton->setEnabled(v->isKnown() && ((KnownVariable*) v)->get().isVector());
			insertButton->setEnabled(v->isActive());
			delButton->setEnabled(v->isLocal());
			deactivateButton->setEnabled(!settings->isAnswerVariable(v) && v != settings->v_memory);
			descriptionView->setHtml(QString::fromStdString(str));
			return;
		}
	}
	editButton->setEnabled(false);
	insertButton->setEnabled(false);
	exportButton->setEnabled(false);
	delButton->setEnabled(false);
	deactivateButton->setEnabled(false);
	descriptionView->clear();
	selected_item = NULL;
}
void VariablesDialog::selectedCategoryChanged(QTreeWidgetItem *iter, QTreeWidgetItem*) {
	if(!iter) selected_category = "";
	else selected_category = iter->text(1).toStdString();
	searchEdit->clear();
	variablesModel->setFilter(selected_category);
	QModelIndex index = variablesView->selectionModel()->currentIndex();
	if(index.isValid()) {
		selectedVariableChanged(index, QModelIndex());
		variablesView->scrollTo(index);
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
void VariablesDialog::updateVariables() {

	size_t cat_i, cat_i_prev;
	bool b;
	std::string str, cat, cat_sub;
	tree_struct variable_cats;
	variable_cats.parent = NULL;
	bool has_inactive = false, has_uncat = false;
	std::list<tree_struct>::iterator it;

	sourceModel->clear();
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Variable")));

	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		Variable *v = CALCULATOR->variables[i];
		if(!v->isActive()) {
			has_inactive = true;
		} else {
			tree_struct *item = &variable_cats;
			if(!v->category().empty()) {
				cat = v->category();
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
			} else if(!v->isLocal()) {
				has_uncat = true;
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(v->title(true)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) v), Qt::UserRole);
		sourceModel->appendRow(item);
		if(v == selected_item) variablesView->selectionModel()->setCurrentIndex(variablesModel->mapFromSource(item->index()), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
	sourceModel->sort(0);
	variable_cats.sort();

	categoriesView->clear();
	QTreeWidgetItem *iter, *iter2, *iter3;
	QStringList l;
	l << tr("All", "All variables"); l << "All";
	iter3 = new QTreeWidgetItem(categoriesView, l);
	tree_struct *item, *item2;
	variable_cats.it = variable_cats.items.begin();
	if(variable_cats.it != variable_cats.items.end()) {
		item = &*variable_cats.it;
		++variable_cats.it;
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
		//add "Uncategorized" category if there are variables without category
		l.clear(); l << tr("Uncategorized"); l << "Uncategorized";
		iter = new QTreeWidgetItem(iter3, l);
		if(selected_category == "Uncategorized") {
			iter->setSelected(true);
		}
	}
	l.clear(); l << tr("User variables"); l << "User items";
	iter = new QTreeWidgetItem(iter3, l);
	if(selected_category == "User items") {
		iter->setSelected(true);
	}
	if(has_inactive) {
		//add "Inactive" category if there are inactive variables
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
void VariablesDialog::setSearch(const QString &str) {
	searchEdit->setText(str);
	searchChanged(str);
}
void VariablesDialog::selectCategory(std::string str) {
	QList<QTreeWidgetItem*> list = categoriesView->findItems((str.empty() || str == "All") ? "All" : "/" + QString::fromStdString(str), Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
	if(!list.isEmpty()) {
		categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
}

