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
#include <QLabel>
#include <QComboBox>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "unitsdialog.h"
#include "itemproxymodel.h"
//#include "uniteditdialog.h"

UnitsDialog::UnitsDialog(QWidget *parent) : QDialog(parent) {
	last_from = true;
	QVBoxLayout *topbox = new QVBoxLayout(this);
	setWindowTitle(tr("Units"));
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
	unitsView = new QTreeView(this);
	unitsView->setSelectionMode(QAbstractItemView::SingleSelection);
	unitsView->setRootIsDecorated(false);
	unitsModel = new ItemProxyModel(this);
	sourceModel = new QStandardItemModel(this);
	unitsModel->setSourceModel(sourceModel);
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Unit")));
	unitsView->setModel(unitsModel);
	selected_item = NULL;
	vbox->addWidget(unitsView, 1);
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
	newButton = new QPushButton(tr("New…"), this); box->addWidget(newButton); connect(newButton, SIGNAL(clicked()), this, SLOT(newClicked()));
	newButton->setEnabled(false);
	editButton = new QPushButton(tr("Edit…"), this); box->addWidget(editButton); connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
	deactivateButton = new QPushButton(tr("Deactivate"), this); box->addWidget(deactivateButton); connect(deactivateButton, SIGNAL(clicked()), this, SLOT(deactivateClicked()));
	delButton = new QPushButton(tr("Delete"), this); box->addWidget(delButton); connect(delButton, SIGNAL(clicked()), this, SLOT(delClicked()));
	box->addSpacing(24);
	convertButton = new QPushButton(tr("Convert"), this); box->addWidget(convertButton); connect(convertButton, SIGNAL(clicked()), this, SLOT(convertClicked()));
	insertButton = new QPushButton(tr("Insert"), this); box->addWidget(insertButton); connect(insertButton, SIGNAL(clicked()), this, SLOT(insertClicked()));
	insertButton->setDefault(true);
	box->addStretch(1);
	hbox->addLayout(box, 0);
	QGridLayout *grid = new QGridLayout();
	equalsLabel = new QLabel("=", this);
	equalsLabel->setAlignment(Qt::AlignRight);
	QFontMetrics fm(equalsLabel->font());
	equalsLabel->setMinimumWidth(fm.boundingRect(SIGN_ALMOST_EQUAL).width());
	equalsLabel->setMinimumHeight(fm.boundingRect(SIGN_ALMOST_EQUAL).height());
	grid->addWidget(equalsLabel, 1, 0);
	fromEdit = new MathLineEdit(this);
	fromEdit->setText("1");
	fromEdit->setAlignment(Qt::AlignRight);
	grid->addWidget(fromEdit, 0, 1);
	toEdit = new MathLineEdit(this);
	toEdit->setText("1");
	toEdit->setAlignment(Qt::AlignRight);
	grid->addWidget(toEdit, 1, 1);
	fromLabel = new QLabel(this);
	grid->addWidget(fromLabel, 0, 2);
	toCombo = new QComboBox(this);
	toModel = new ItemProxyModel(this);
	toSourceModel = new QStandardItemModel(this);
	toSourceModel->setColumnCount(1);
	toModel->setSourceModel(toSourceModel);
	toCombo->setModel(toModel);
	grid->addWidget(toCombo, 1, 2);
	topbox->addLayout(grid);
	topbox->addSpacing(topbox->spacing() * 2);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	topbox->addWidget(buttonBox);
	selected_category = "All";
	updateUnits();
	unitsModel->setFilter("All");
	toModel->setFilter("All");
	connect(searchEdit, SIGNAL(textEdited(const QString&)), this, SLOT(searchChanged(const QString&)));
	connect(fromEdit, SIGNAL(textEdited(const QString&)), this, SLOT(fromChanged()));
	connect(toEdit, SIGNAL(textEdited(const QString&)), this, SLOT(toChanged()));
	connect(toCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(toUnitChanged()));
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(categoriesView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedCategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(unitsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedUnitChanged(const QModelIndex&, const QModelIndex&)));
	connect(unitsView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(onUnitActivated(const QModelIndex&)));
	selectedUnitChanged(QModelIndex(), QModelIndex());
	if(!settings->units_geometry.isEmpty()) restoreGeometry(settings->units_geometry);
	else resize(900, 800);
	if(!settings->units_vsplitter_state.isEmpty()) vsplitter->restoreState(settings->units_vsplitter_state);
	if(!settings->units_hsplitter_state.isEmpty()) hsplitter->restoreState(settings->units_hsplitter_state);
}
UnitsDialog::~UnitsDialog() {}

void UnitsDialog::convert(bool from) {
	QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(!index.isValid() || toCombo->currentIndex() < 0) {toEdit->clear(); return;}
	last_from = from;
	Unit *from_u, *to_u;
	std::string str;
	if(from) {
		str = fromEdit->text().trimmed().toStdString();
		if(str == CALCULATOR->timedOutString()) return;
		from_u = (Unit*) index.data(Qt::UserRole).value<void*>();
		to_u = (Unit*) toCombo->currentData(Qt::UserRole).value<void*>();
		if(from_u == to_u || str.empty()) {toEdit->setText(fromEdit->text().trimmed()); return;}
	} else {
		str = toEdit->text().trimmed().toStdString();
		if(str == CALCULATOR->timedOutString()) return;
		to_u = (Unit*) index.data(Qt::UserRole).value<void*>();
		from_u = (Unit*) toCombo->currentData(Qt::UserRole).value<void*>();
		if(from_u == to_u || str.empty()) {fromEdit->setText(toEdit->text().trimmed()); return;}
	}
	bool b = false;
	EvaluationOptions eo;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	eo.parse_options = settings->evalops.parse_options;
	eo.parse_options.base = 10;
	if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	PrintOptions po;
	po.is_approximate = &b;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
	CALCULATOR->resetExchangeRatesUsed();
	CALCULATOR->beginTemporaryStopMessages();
	MathStructure v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(str, eo.parse_options), from_u, to_u, 1500, eo);
	if(!v_mstruct.isAborted() && settings->checkExchangeRates(this)) v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(str, eo.parse_options), from_u, to_u, 1500, eo);
	if(v_mstruct.isAborted()) {
		str = CALCULATOR->timedOutString();
	} else {
		str = CALCULATOR->print(v_mstruct, 300, po);
	}
	if(from) toEdit->setText(QString::fromStdString(str));
	else fromEdit->setText(QString::fromStdString(str));
	b = b || v_mstruct.isApproximate();
	if(b) equalsLabel->setText(SIGN_ALMOST_EQUAL);
	else equalsLabel->setText("=");
	CALCULATOR->endTemporaryStopMessages();
}
void UnitsDialog::fromChanged() {
	convert(true);
}
void UnitsDialog::toChanged() {
	convert(false);
}
void UnitsDialog::toUnitChanged() {
	convert(last_from);
}
void UnitsDialog::fromUnitChanged() {
	convert(last_from);
}
void UnitsDialog::keyPressEvent(QKeyEvent *event) {
	if(event->matches(QKeySequence::Find)) {
		searchEdit->setFocus();
		return;
	}
	if(event->key() == Qt::Key_Escape && searchEdit->hasFocus()) {
		searchEdit->clear();
		unitsView->setFocus();
		return;
	}
	if(event->key() == Qt::Key_Escape && (fromEdit->hasFocus() || toEdit->hasFocus())) {
		fromEdit->clear();
		toEdit->clear();
		return;
	}
	if(event->key() == Qt::Key_Return && unitsView->hasFocus()) {
		QModelIndex index = unitsView->selectionModel()->currentIndex();
		if(index.isValid()) {
			onUnitActivated(index);
			return;
		}
	}
	QDialog::keyPressEvent(event);
}
void UnitsDialog::onUnitActivated(const QModelIndex &index) {
	if(!index.isValid()) return;
	Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
	if(u) emit unitActivated(u);
}
void UnitsDialog::closeEvent(QCloseEvent *e) {
	settings->units_geometry = saveGeometry();
	settings->units_vsplitter_state = vsplitter->saveState();
	settings->units_hsplitter_state = hsplitter->saveState();
	QDialog::closeEvent(e);
}
void UnitsDialog::reject() {
	settings->units_geometry = saveGeometry();
	settings->units_vsplitter_state = vsplitter->saveState();
	settings->units_hsplitter_state = hsplitter->saveState();
	QDialog::reject();
}
void UnitsDialog::searchChanged(const QString &str) {
	unitsModel->setSecondaryFilter(str.toStdString());
	unitsView->selectionModel()->setCurrentIndex(unitsModel->index(0, 0), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
}
void UnitsDialog::newClicked() {
	/*Unit *u = UnitEditDialog::newUnit(this);
	if(u) {
		selected_item = u;
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) u), Qt::UserRole);
		sourceModel->appendRow(item);
		unitsModel->invalidate();
		QModelIndex index = unitsModel->mapFromSource(item->index());
		if(index.isValid()) {
			unitsView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			unitsView->scrollTo(index);
		}
		emit itemsChanged();
	}*/
}
void UnitsDialog::unitRemoved(Unit *u) {
	QModelIndexList list = sourceModel->match(sourceModel->index(0, 0), Qt::UserRole, QVariant::fromValue((void*) u), 1, Qt::MatchExactly);
	if(!list.isEmpty()) sourceModel->removeRow(list[0].row());
}
void UnitsDialog::editClicked() {
	/*QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
	if(u && UnitEditDialog::editUnit(this, u)) {
		sourceModel->removeRow(unitsModel->mapToSource(unitsView->selectionModel()->currentIndex()).row());
		QStandardItem *item = new QStandardItem(QString::fromStdString(f->title(true)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) u), Qt::UserRole);
		sourceModel->appendRow(item);
		selected_item = u;
		unitsModel->invalidate();
		QModelIndex index = unitsModel->mapFromSource(item->index());
		if(index.isValid()) {
			unitsView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
			unitsView->scrollTo(index);
		}
		emit itemsChanged();
	}*/
}
void UnitsDialog::delClicked() {
	QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
	if(u && u->isLocal()) {
		sourceModel->removeRow(unitsModel->mapToSource(unitsView->selectionModel()->currentIndex()).row());
		selected_item = NULL;
		u->destroy();
		emit itemsChanged();
	}
}
void UnitsDialog::convertClicked() {
	QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
	if(u) {
		emit convertToUnitRequest(u);
	}
}
void UnitsDialog::insertClicked() {
	QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
	if(u) {
		emit insertUnitRequest(u);
	}
}
void UnitsDialog::deactivateClicked() {
	QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(!index.isValid()) return;
	Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
	if(u) {
		u->setActive(!u->isActive());
		unitsModel->invalidate();
		emit itemsChanged();
	}
}
void UnitsDialog::selectedUnitChanged(const QModelIndex &index, const QModelIndex&) {
	if(index.isValid()) {
		Unit *u = (Unit*) index.data(Qt::UserRole).value<void*>();
		if(CALCULATOR->stillHasUnit(u)) {
			selected_item = u;
			std::string str;
			const ExpressionName *ename = &u->preferredName(false, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) descriptionView);
			str = "<b>";
			str += ename->name;
			str += "</b>";
			for(size_t i2 = 1; i2 <= u->countNames(); i2++) {
				if(&u->getName(i2) != ename) {
					str += ", ";
					str += u->getName(i2).name;
				}
			}
			str += "<br><br>";
			bool is_approximate = false;
			PrintOptions po = settings->printops;
			po.can_display_unicode_string_arg = (void*) descriptionView;
			po.is_approximate = &is_approximate;
			po.allow_non_usable = true;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			po.base = 10;
			po.number_fraction_format = FRACTION_DECIMAL_EXACT;
			po.use_unit_prefixes = false;
			if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
				AliasUnit *au = (AliasUnit*) u;
				MathStructure m(1, 1, 0), mexp(1, 1, 0);
				if(au->hasNonlinearExpression()) {
					m.set("x");
					if(au->expression().find("\\y") != std::string::npos) mexp.set("y");
					str += "<i>x</i> ";
					str += u->preferredDisplayName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) descriptionView).name;
					str += " ";
					if(au->expression().find("\\y") != std::string::npos) str += "<sup><i>y</i></sup>";
				}
				au->convertToFirstBaseUnit(m, mexp);
				m.multiply(au->firstBaseUnit());
				if(!mexp.isOne()) m.last() ^= mexp;
				if(m.isApproximate() || is_approximate) str += SIGN_ALMOST_EQUAL " ";
				else str += "= ";
				m.format(po);
				str += m.print(po, true, false, TAG_TYPE_HTML);
				if(au->hasNonlinearExpression() && !au->inverseExpression().empty()) {
					str += "<br>";
					m.set("x");
					if(au->inverseExpression().find("\\y") != std::string::npos) mexp.set("y");
					else mexp.set(1, 1, 0);
					str += "<i>x</i> ";
					str += au->firstBaseUnit()->preferredDisplayName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) descriptionView).name;
					str += " ";
					if(au->inverseExpression().find("\\y") != std::string::npos) str += "<sup><i>y</i></sup>";
					au->convertFromFirstBaseUnit(m, mexp);
					m.multiply(au);
					if(!mexp.isOne()) m.last() ^= mexp;
					if(m.isApproximate() || is_approximate) str += SIGN_ALMOST_EQUAL " ";
					else str += "= ";
					m.format(po);
					str += m.print(po, true, false, TAG_TYPE_HTML);
				}
			} else if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				str += "= ";
				MathStructure m(((CompositeUnit*) u)->generateMathStructure());
				m.format(po);
				str += m.print(po, true, false, TAG_TYPE_HTML);
			}
			if(!u->description().empty()) {
				if(u->subtype() != SUBTYPE_BASE_UNIT) str += "<br>";
				str += "<br>";
				str += u->description();
			}
			if(u->isActive() != (deactivateButton->text() == tr("Deactivate"))) {
				deactivateButton->setMinimumWidth(deactivateButton->width());
				if(u->isActive()) {
					deactivateButton->setText(tr("Deactivate"));
				} else {
					deactivateButton->setText(tr("Activate"));
				}
			}
			//editButton->setEnabled(!u->isBuiltin());
			insertButton->setEnabled(u->isActive());
			convertButton->setEnabled(u->isActive());
			delButton->setEnabled(u->isLocal());
			deactivateButton->setEnabled(true);
			descriptionView->setHtml(QString::fromStdString(str));
			std::string to_filter;
			if(u->category().empty()) {
				if(u->isLocal()) to_filter = "Uncategorized";
				else to_filter =  "User items";
			} else {
				to_filter = "/"; to_filter += u->category();
			}
			bool change_index = false;
			if(u->baseUnit()->referenceName() == "m" && u->baseExponent() == 3) {
				size_t i = to_filter.find("/", 1);
				if(i != std::string::npos) to_filter = to_filter.substr(0, i);
			}
			if(to_filter != toModel->currentFilter()) {
				toModel->setFilter(to_filter);
				toModel->sort(0);
				change_index = toCombo->count() > 0;
			} else if(toCombo->currentIndex() >= 0 && toCombo->currentData(Qt::UserRole).value<void*>() == u) {
				change_index = true;
			}
			if(change_index) {
				if(toCombo->itemData(0, Qt::UserRole).value<void*>() == u && toCombo->count() >= 2) toCombo->setCurrentIndex(1);
				else toCombo->setCurrentIndex(0);
			}
			fromEdit->setEnabled(u->isActive());
			toEdit->setEnabled(u->isActive());
			toCombo->setEnabled(u->isActive());
			QString qstr;
			if(u->isCurrency()) qstr = QString::fromStdString(u->referenceName());
			else qstr = QString::fromStdString(u->print(true, true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) fromLabel));
			if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				qstr.replace("^-1", "⁻" SIGN_POWER_1);
				qstr.replace("^-2", "⁻" SIGN_POWER_2);
				qstr.replace("^-3", "⁻" SIGN_POWER_3);
				qstr.replace("^-4", "⁻" SIGN_POWER_4);
				qstr.replace("^4", SIGN_POWER_4);
				qstr.remove("_unit");
			}
			fromLabel->setText(qstr);
			fromUnitChanged();
			return;
		}
	}
	fromUnitChanged();
	fromEdit->setEnabled(false);
	toEdit->setEnabled(false);
	toCombo->setEnabled(false);
	fromLabel->setText(QString());
	editButton->setEnabled(false);
	insertButton->setEnabled(false);
	convertButton->setEnabled(false);
	delButton->setEnabled(false);
	deactivateButton->setEnabled(false);
	descriptionView->clear();
	selected_item = NULL;
}
void UnitsDialog::selectedCategoryChanged(QTreeWidgetItem *iter, QTreeWidgetItem*) {
	if(!iter) selected_category = "";
	else selected_category = iter->text(1).toStdString();
	searchEdit->clear();
	unitsModel->setFilter(selected_category);
	QModelIndex index = unitsView->selectionModel()->currentIndex();
	if(index.isValid()) {
		selectedUnitChanged(index, QModelIndex());
		unitsView->scrollTo(index);
	} else if(selected_category != "All" && selected_category != "Inactive" && selected_category != toModel->currentFilter()) {
		toModel->setFilter(selected_category);
		toModel->sort(0);
		if(toCombo->count()) toCombo->setCurrentIndex(0);
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
void UnitsDialog::updateUnits() {

	size_t cat_i, cat_i_prev;
	bool b;
	std::string str, cat, cat_sub;
	QString qstr;
	tree_struct unit_cats;
	unit_cats.parent = NULL;
	bool has_inactive = false, has_uncat = false;
	std::list<tree_struct>::iterator it;

	sourceModel->clear();
	sourceModel->setColumnCount(1);
	sourceModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Unit")));

	toSourceModel->clear();
	toSourceModel->setColumnCount(1);

	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		Unit *u = CALCULATOR->units[i];
		if(!u->isActive()) {
			has_inactive = true;
		} else {
			tree_struct *item = &unit_cats;
			if(!u->category().empty()) {
				cat = u->category();
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
			} else if(!u->isLocal()) {
				has_uncat = true;
			}
		}
		QStandardItem *item = new QStandardItem(QString::fromStdString(u->title(true)));
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) u), Qt::UserRole);
		sourceModel->appendRow(item);
		if(u == selected_item) unitsView->selectionModel()->setCurrentIndex(unitsModel->mapFromSource(item->index()), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
		
		if(u->isCurrency()) qstr = QString::fromStdString(u->referenceName());
		else qstr = QString::fromStdString(u->print(true, true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) toCombo));
		if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			qstr.replace("^-1", "⁻" SIGN_POWER_1);
			qstr.replace("^-2", "⁻" SIGN_POWER_2);
			qstr.replace("^-3", "⁻" SIGN_POWER_3);
			qstr.replace("^-4", "⁻" SIGN_POWER_4);
			qstr.replace("^4", SIGN_POWER_4);
			qstr.remove("_unit");
		}
		item = new QStandardItem(qstr);
		item->setEditable(false);
		item->setData(QVariant::fromValue((void*) u), Qt::UserRole);
		toSourceModel->appendRow(item);
	}
	sourceModel->sort(0);
	toSourceModel->sort(0);
	unit_cats.sort();

	categoriesView->clear();
	QTreeWidgetItem *iter, *iter2, *iter3;
	QStringList l;
	l << tr("All", "All units"); l << "All";
	iter3 = new QTreeWidgetItem(categoriesView, l);
	tree_struct *item, *item2;
	unit_cats.it = unit_cats.items.begin();
	if(unit_cats.it != unit_cats.items.end()) {
		item = &*unit_cats.it;
		++unit_cats.it;
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
		//add "Uncategorized" category if there are units without category
		l.clear(); l << tr("Uncategorized"); l << "Uncategorized";
		iter = new QTreeWidgetItem(iter3, l);
		if(selected_category == "Uncategorized") {
			iter->setSelected(true);
		}
	}
	l.clear(); l << tr("User units"); l << "User items";
	iter = new QTreeWidgetItem(iter3, l);
	if(selected_category == "User items") {
		iter->setSelected(true);
	}
	if(has_inactive) {
		//add "Inactive" category if there are inactive units
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
void UnitsDialog::setSearch(const QString &str) {
	searchEdit->setText(str);
	searchChanged(str);
}
void UnitsDialog::selectCategory(std::string str) {
	QList<QTreeWidgetItem*> list = categoriesView->findItems((str.empty() || str == "All") ? "All" : "/" + QString::fromStdString(str), Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap, 1);
	if(!list.isEmpty()) {
		categoriesView->setCurrentItem(list[0], 0, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
	}
}


