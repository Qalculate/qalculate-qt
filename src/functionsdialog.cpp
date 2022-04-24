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
#include <QCheckBox>
#include <QLineEdit>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functionsdialog.h"
#include "itemproxymodel.h"
#include "functioneditdialog.h"
#include "dataseteditdialog.h"

FunctionsDialog::FunctionsDialog(QWidget *parent) : QDialog(parent, Qt::Window) {
	QVBoxLayout *topbox = new QVBoxLayout(this);
	setWindowTitle(tr("Functions"));
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
	categoriesView->installEventFilter(this);
	hsplitter->addWidget(categoriesView);
	QWidget *w = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout(w);
	vbox->setSpacing(0);
	vbox->setContentsMargins(0, 0, 0, 0);
	functionsView = new QTreeView(this);
	functionsView->setSelectionMode(QAbstractItemView::SingleSelection);
	functionsView->setRootIsDecorated(false);
	functionsModel = new ItemProxyModel(this);
	functionsView->installEventFilter(this);
	sourceModel = new QStandardItemModel(this);
	functionsModel->setSourceModel(sourceModel);
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Function")));
	functionsView->setModel(functionsModel);
	selected_item = NULL;
	vbox->addWidget(functionsView, 1);
	searchEdit = new QLineEdit(this);
	searchEdit->addAction(LOAD_ICON("edit-find"), QLineEdit::LeadingPosition);
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
	searchEdit->setTextMargins(22, 0, 0, 0);
#	endif
#endif
	searchEdit->installEventFilter(this);
	vbox->addWidget(searchEdit, 0);
	hsplitter->addWidget(w);
	vsplitter->addWidget(hsplitter);
	descriptionView = new QTextEdit(this);
	descriptionView->setReadOnly(true);
	vsplitter->addWidget(descriptionView);
	vsplitter->setStretchFactor(0, 2);
	vsplitter->setStretchFactor(1, 1);
	hsplitter->setStretchFactor(0, 2);
	hsplitter->setStretchFactor(1, 3);
	QVBoxLayout *box = new QVBoxLayout();
	newButton = new QPushButton(tr("New…"), this); box->addWidget(newButton); connect(newButton, SIGNAL(clicked()), this, SLOT(newClicked()));
	editButton = new QPushButton(tr("Edit…"), this); box->addWidget(editButton); connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
	deactivateButton = new QPushButton(tr("Deactivate"), this); box->addWidget(deactivateButton); connect(deactivateButton, SIGNAL(clicked()), this, SLOT(deactivateClicked()));
	delButton = new QPushButton(tr("Delete"), this); box->addWidget(delButton); connect(delButton, SIGNAL(clicked()), this, SLOT(delClicked()));
	box->addSpacing(24);
	calculateButton = new QPushButton(tr("Calculate…"), this); box->addWidget(calculateButton); connect(calculateButton, SIGNAL(clicked()), this, SLOT(calculateClicked()));
	calculateButton->setDefault(true);
	applyButton = new QPushButton(tr("Apply"), this); box->addWidget(applyButton); connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
	insertButton = new QPushButton(tr("Insert"), this); box->addWidget(insertButton); connect(insertButton, SIGNAL(clicked()), this, SLOT(insertClicked()));
	box->addSpacing(24);
	favouriteButton = new QCheckBox(tr("Favorite"), this); box->addWidget(favouriteButton); connect(favouriteButton, SIGNAL(clicked()), this, SLOT(favouriteClicked()));
	box->addStretch(1);
	hbox->addLayout(box, 0);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	topbox->addWidget(buttonBox);
	selected_category = "All";
	updateFunctions();
	functionsView->setFocus();
	connect(searchEdit, SIGNAL(textChanged(const QString&)), this, SLOT(searchChanged(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(categoriesView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(functionsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedFunctionChanged(const QModelIndex&, const QModelIndex&)));
	connect(functionsView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(calculateClicked()));
	if(!settings->functions_geometry.isEmpty()) restoreGeometry(settings->functions_geometry);
	else resize(900, 800);
	if(!settings->functions_vsplitter_state.isEmpty()) vsplitter->restoreState(settings->functions_vsplitter_state);
	if(!settings->functions_hsplitter_state.isEmpty()) hsplitter->restoreState(settings->functions_hsplitter_state);
}
FunctionsDialog::~FunctionsDialog() {}

bool FunctionsDialog::eventFilter(QObject *o, QEvent *e) {
	if(e->type() == QEvent::KeyPress) {
		QKeyEvent *event = static_cast<QKeyEvent*>(e);
		if(o == searchEdit) {
			if(event->key() == Qt::Key_Down || event->key() == Qt::Key_Up || event->key() == Qt::Key_PageDown || event->key() == Qt::Key_PageUp) {
				functionsView->setFocus();
				QKeyEvent *eventCopy = new QKeyEvent(event->type(), event->key(), event->modifiers(), event->text(), event->isAutoRepeat(), event->count());
				QApplication::postEvent(functionsView, eventCopy);
				return true;
			}
		} else if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::GroupSwitchModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
			if(!event->text().isEmpty() && event->text()[0].isLetterOrNumber()) {
				searchEdit->setFocus();
				searchEdit->setText(event->text());
				return true;
			}
		}
	}
	return QDialog::eventFilter(o, e);
}
void FunctionsDialog::keyPressEvent(QKeyEvent *event) {
	if(event->matches(QKeySequence::Find)) {
		searchEdit->setFocus();
		return;
	}
	if(event->key() == Qt::Key_Escape && searchEdit->hasFocus()) {
		searchEdit->clear();
		functionsView->setFocus();
		return;
	}
	if(event->key() == Qt::Key_Return && functionsView->hasFocus()) {
		QModelIndex index = functionsView->selectionModel()->currentIndex();
		if(index.isValid()) {
			calculateClicked();
			return;
		}
	}
	QDialog::keyPressEvent(event);
}
void FunctionsDialog::closeEvent(QCloseEvent *e) {
	settings->functions_geometry = saveGeometry();
	settings->functions_vsplitter_state = vsplitter->saveState();
	settings->functions_hsplitter_state = hsplitter->saveState();
	QDialog::closeEvent(e);
}
void FunctionsDialog::reject() {
	settings->functions_geometry = saveGeometry();
	settings->functions_vsplitter_state = vsplitter->saveState();
	settings->functions_hsplitter_state = hsplitter->saveState();
	QDialog::reject();
}
void FunctionsDialog::searchChanged(const QString &str) {
	functionsModel->setSecondaryFilter(str.toStdString());
	functionsView->selectionModel()->setCurrentIndex(functionsModel->index(0, 0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	if(str.isEmpty()) functionsView->setFocus();
}
void FunctionsDialog::newClicked() {
	MathFunction *replaced_item = NULL;
	UserFunction *f = FunctionEditDialog::newFunction(this, &replaced_item);
	if(f) {
		if(replaced_item && (replaced_item == f || !CALCULATOR->hasFunction(replaced_item))) {
			QModelIndexList list = sourceModel->match(sourceModel->index(0, 0), Qt::UserRole, QVariant::fromValue((void*) replaced_item), 1, Qt::MatchExactly);
			if(!list.isEmpty()) sourceModel->removeRow(list[0].row());
		} else if(replaced_item && !replaced_item->isActive()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("Inactive", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(list.isEmpty()) {QStringList l; l << tr("Inactive"); l << "Inactive"; new QTreeWidgetItem(categoriesView, l);}
		}
		selected_item = f;
		if(f->category().empty()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("Uncategorized", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(list.isEmpty()) {QStringList l; l << tr("Uncategorized"); l << "Uncategorized"; new QTreeWidgetItem(categoriesView->topLevelItem(2), l);}
		} else if(f->category() != CALCULATOR->temporaryCategory()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems(QString("/") + QString::fromStdString(f->category()), Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(list.isEmpty()) {
				if(selected_category != "All") selected_category = "User items";
				updateFunctions();
				emit itemsChanged();
				return;
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) functionsView)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) f), Qt::UserRole);
		sourceModel->appendRow(item);
		if(selected_category != "All" && selected_category != "User items" && selected_category != std::string("/") + f->category() && (selected_category != "Uncategorized" || !f->category().empty())) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("User items", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(!list.isEmpty()) {
				categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			}
		} else {
			functionsModel->invalidate();
		}
		sourceModel->sort(0);
		QModelIndex index = functionsModel->mapFromSource(item->index());
		if(index.isValid()) {
			functionsView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			functionsView->scrollTo(index);
		}
		if(f != replaced_item && !f->isHidden()) settings->favourite_functions.push_back(f);
		emit itemsChanged();
	}
}
void FunctionsDialog::editClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *replaced_item = NULL;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f && ((f->subtype() != SUBTYPE_DATA_SET && FunctionEditDialog::editFunction(this, f, &replaced_item)) || (f->subtype() == SUBTYPE_DATA_SET && DataSetEditDialog::editDataset(this, (DataSet*) f, &replaced_item)))) {
		sourceModel->removeRow(functionsModel->mapToSource(functionsView->selectionModel()->currentIndex()).row());
		if(replaced_item && !CALCULATOR->hasFunction(replaced_item)) {
			QModelIndexList list = sourceModel->match(sourceModel->index(0, 0), Qt::UserRole, QVariant::fromValue((void*) replaced_item), 1, Qt::MatchExactly);
			if(!list.isEmpty()) sourceModel->removeRow(list[0].row());
		} else if(replaced_item && !replaced_item->isActive()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("Inactive", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(list.isEmpty()) {QStringList l; l << tr("Inactive"); l << "Inactive"; new QTreeWidgetItem(categoriesView, l);}
		}
		selected_item = f;
		if(f->category().empty()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("Uncategorized", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(list.isEmpty()) {QStringList l; l << tr("Uncategorized"); l << "Uncategorized"; new QTreeWidgetItem(categoriesView->topLevelItem(2), l);}
		} else if(f->category() != CALCULATOR->temporaryCategory()) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems(QString("/") + QString::fromStdString(f->category()), Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(list.isEmpty()) {
				if(selected_category != "All") selected_category = "User items";
				updateFunctions();
				emit itemsChanged();
				return;
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) functionsView)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) f), Qt::UserRole);
		sourceModel->appendRow(item);
		if(selected_category != "All" && selected_category != "User items" && selected_category != std::string("/") + f->category() && (selected_category != "Uncategorized" || !f->category().empty())) {
			QList<QTreeWidgetItem*> list = categoriesView->findItems("User items", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
			if(!list.isEmpty()) {
				categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			}
		} else {
			functionsModel->invalidate();
		}
		sourceModel->sort(0);
		QModelIndex index = functionsModel->mapFromSource(item->index());
		if(index.isValid()) {
			functionsView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			functionsView->scrollTo(index);
		}
		emit itemsChanged();
	}
}
void FunctionsDialog::favouriteClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(!f) return;
	if(favouriteButton->isChecked()) {
		settings->favourite_functions.push_back(f);
	} else {
		for(size_t i = 0; i < settings->favourite_functions.size(); i++) {
			if(settings->favourite_functions[i] == f) {
				settings->favourite_functions.erase(settings->favourite_functions.begin() + i);
				break;
			}
		}
		if(selected_category == "Favorites") {
			if(settings->favourite_functions.empty()) {
				QList<QTreeWidgetItem*> list = categoriesView->findItems("All", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
				if(!list.isEmpty()) categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			} else {
				selected_item = NULL;
				functionsModel->invalidate();
				selectedFunctionChanged(functionsView->selectionModel()->currentIndex(), functionsView->selectionModel()->currentIndex());
			}
		}
	}
	settings->favourite_functions_changed = true;
	emit itemsChanged();
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
void FunctionsDialog::calculateClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f) {
		emit calculateFunctionRequest(f);
	}
}
void FunctionsDialog::insertClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f) {
		emit insertFunctionRequest(f);
	}
}
void FunctionsDialog::deactivateClicked() {
	QModelIndex index = functionsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	MathFunction *f = (MathFunction*) index.data(Qt::UserRole).value<void*>();
	if(f) {
		f->setActive(!f->isActive());
		QList<QTreeWidgetItem*> list = categoriesView->findItems(f->isActive() ? "All" : "Inactive", Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
		if(!list.isEmpty()) {
			categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
		} else if(!f->isActive()) {
			QStringList l; l << tr("Inactive"); l << "Inactive";
			categoriesView->setCurrentItem(new QTreeWidgetItem(categoriesView, l), 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
		}
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
			const ExpressionName *ename = &f->preferredName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) descriptionView);
			str = "<b><i>";
			str += ename->formattedName(TYPE_FUNCTION, true, true);
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
					str += f->getName(i2).formattedName(TYPE_FUNCTION, true, true);
				}
			}
			str += "</i><br>";
			if(f->subtype() == SUBTYPE_DATA_SET) {
				str += "<br>";
				str += tr("Retrieves data from the %1 data set for a given object and property. If \"info\" is typed as property, a dialog window will pop up with all properties of the object.").arg(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) descriptionView))).toHtmlEscaped().toStdString();
				str += "<br>";
			}
			if(!f->description().empty()) {
				str += "<br>";
				str += to_html_escaped(f->description());
				str += "<br>";
			}
			if(!f->example(true).empty()) {
				str += "<br>";
				str += tr("Example:").toStdString();
				str += " ";
				str += to_html_escaped(f->example(false, ename->formattedName(TYPE_FUNCTION, true)));
				str += "<br>";
			}
			if(f->subtype() == SUBTYPE_DATA_SET && !((DataSet*) f)->copyright().empty()) {
				str += "<br>";
				str += to_html_escaped(((DataSet*) f)->copyright());
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
						str += to_html_escaped(arg->printlong());
					} else {
						str += default_arg.printlong();
					}
					if(i2 > f->minargs()) {
						str += " (";
						str += tr("optional", "optional argument").toStdString();
						if(!f->getDefaultValue(i2).empty() && f->getDefaultValue(i2) != "\"\"") {
							str += ", ";
							str += tr("default:", "argument default").toStdString();
							str += " ";
							ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
							std::string str2 = CALCULATOR->localizeExpression(f->getDefaultValue(i2), pa);
							gsub("*", settings->multiplicationSign(), str2);
							gsub("/", settings->divisionSign(false), str2);
							gsub("-", SIGN_MINUS, str2);
							str += str2;
						}
						str += ")";
					}
					str += "</i><br>";
				}
			}
			if(!f->condition().empty()) {
				str += "<br>";
				str += tr("Requirement:", "Required condition for function").toStdString();
				str += " ";
				str += to_html_escaped(f->printCondition());
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
							str += tr("%1:").arg(QString::fromStdString(dp->title())).toStdString();
							str += " ";
						}
						for(size_t i = 1; i <= dp->countNames(); i++) {
							if(i > 1) str += ", ";
							str += dp->getName(i);
						}
						if(dp->isKey()) {
							str += " (";
							//: indicating that the property is a data set key
							str += tr("key").toStdString();
							str += ")";
						}
						str += "<br>";
						if(!dp->description().empty()) {
							str += "<i>";
							str += to_html_escaped(dp->description());
							str += "</i><br>";
						}
					}
					dp = ds->getNextProperty(&it);
				}
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
			calculateButton->setEnabled(f->isActive());
			delButton->setEnabled(f->isLocal());
			favouriteButton->setEnabled(true);
			favouriteButton->setChecked(false);
			for(size_t i = 0; i < settings->favourite_functions.size(); i++) {
				if(settings->favourite_functions[i] == f) {
					favouriteButton->setChecked(true);
					break;
				}
			}
			deactivateButton->setEnabled(true);
			applyButton->setEnabled(f->isActive() && (f->minargs() <= 1 || settings->rpn_mode));
			descriptionView->setHtml(QString::fromStdString(str));
			return;
		}
	}
	editButton->setEnabled(false);
	insertButton->setEnabled(false);
	calculateButton->setEnabled(false);
	delButton->setEnabled(false);
	deactivateButton->setEnabled(false);
	applyButton->setEnabled(false);
	favouriteButton->setEnabled(false);
	favouriteButton->setChecked(false);
	descriptionView->clear();
	selected_item = NULL;
}
void FunctionsDialog::selectedCategoryChanged(QTreeWidgetItem *iter, QTreeWidgetItem*) {
	if(!iter) selected_category = "";
	else selected_category = iter->text(1).toStdString();
	searchEdit->clear();
	functionsModel->setFilter(selected_category);
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
	QStandardItem *item_sel = NULL;

	functionsView->selectionModel()->blockSignals(true);
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
			} else {
				has_uncat = true;
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) functionsView)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) f), Qt::UserRole);
		sourceModel->appendRow(item);
		if(f == selected_item) item_sel = item;
	}
	functionsView->selectionModel()->blockSignals(false);
	sourceModel->sort(0);
	function_cats.sort();

	categoriesView->blockSignals(true);
	categoriesView->clear();
	categoriesView->blockSignals(false);
	QTreeWidgetItem *iter, *iter2, *iter3;
	QStringList l;
	l.clear(); l << tr("Favorites"); l << "Favorites";
	iter = new QTreeWidgetItem(categoriesView, l);
	if(selected_category == "Favorites") {
		iter->setSelected(true);
	}
	l.clear(); l << tr("User functions"); l << "User items";
	iter = new QTreeWidgetItem(categoriesView, l);
	if(selected_category == "User items") {
		iter->setSelected(true);
	}
	l.clear(); l << tr("All", "All functions"); l << "All";
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
	if(has_inactive) {
		//add "Inactive" category if there are inactive functions
		l.clear(); l << tr("Inactive"); l << "Inactive";
		iter = new QTreeWidgetItem(categoriesView, l);
		if(selected_category == "Inactive") {
			iter->setSelected(true);
		}
	}
	iter3->setExpanded(true);
	if(categoriesView->selectedItems().isEmpty()) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_category = "All";
		iter3->setSelected(true);
	}
	searchEdit->blockSignals(true);
	searchEdit->clear();
	searchEdit->blockSignals(false);
	functionsModel->setFilter(selected_category);
	QModelIndex index;
	if(item_sel) index = functionsModel->mapFromSource(item_sel->index());
	if(!index.isValid()) index = functionsModel->index(0, 0);
	if(index.isValid()) {
		functionsView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
		functionsView->scrollTo(index);
	}
	selectedFunctionChanged(index, QModelIndex());
}
void FunctionsDialog::setSearch(const QString &str) {
	searchEdit->setText(str);
}
void FunctionsDialog::selectCategory(std::string str) {
	QList<QTreeWidgetItem*> list = categoriesView->findItems((str.empty() || str == "All") ? "All" : "/" + QString::fromStdString(str), Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
	if(!list.isEmpty()) {
		categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
}

