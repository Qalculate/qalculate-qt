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
#include <QGridLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "datasetsdialog.h"

DataSetsDialog::DataSetsDialog(QWidget *parent) : QDialog(parent) {
	selected_dataset = NULL;
	selected_object = NULL;
	QVBoxLayout *topbox = new QVBoxLayout(this);
	setWindowTitle(tr("Data Sets"));
	hsplitter = new QSplitter(Qt::Horizontal, this);
	topbox->addWidget(hsplitter, 1);
	vsplitter_l = new QSplitter(Qt::Vertical, this);
	vsplitter_r = new QSplitter(Qt::Vertical, this);
	hsplitter->addWidget(vsplitter_l);
	hsplitter->addWidget(vsplitter_r);
	QWidget *w = new QWidget(this);
	QVBoxLayout *box = new QVBoxLayout(w);
	vsplitter_l->addWidget(w);
	box->addWidget(new QLabel(tr("Data sets:")));
	datasetsView = new QTreeWidget(this);
	datasetsView->setSelectionMode(QAbstractItemView::SingleSelection);
	datasetsView->setRootIsDecorated(false);
	datasetsView->header()->hide();
	datasetsView->setColumnCount(1);
	box->addWidget(datasetsView);
	w = new QWidget(this);
	box = new QVBoxLayout(w);
	vsplitter_l->addWidget(w);
	box->addWidget(new QLabel(tr("Objects:")));
	objectsView = new QTreeWidget(this);
	objectsView->setSelectionMode(QAbstractItemView::SingleSelection);
	objectsView->setRootIsDecorated(false);
	objectsView->setAllColumnsShowFocus(true);
	objectsView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	objectsView->header()->hide();
	objectsView->setColumnCount(1);
	box->addWidget(objectsView);
	w = new QWidget(this);
	box = new QVBoxLayout(w);
	vsplitter_r->addWidget(w);
	box->addWidget(new QLabel(tr("Data set description:")));
	descriptionView = new QTextEdit(this);
	descriptionView->setReadOnly(true);
	box->addWidget(descriptionView);
	w = new QWidget(this);
	box = new QVBoxLayout(w);
	vsplitter_r->addWidget(w);
	box->addWidget(new QLabel(tr("Object attributes:")));
	propertiesView = new QTreeWidget(this);
	propertiesView->setSelectionMode(QAbstractItemView::NoSelection);
	propertiesView->setRootIsDecorated(false);
	propertiesView->setAllColumnsShowFocus(true);
	propertiesView->setColumnCount(3);
	propertiesView->header()->setStretchLastSection(false);
	propertiesView->headerItem()->setText(0, QString());
	propertiesView->headerItem()->setText(1, QString());
	propertiesView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	propertiesView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
	propertiesView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	propertiesView->header()->hide();
	box->addWidget(propertiesView);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	topbox->addWidget(buttonBox);
	datasetsView->setFocus();
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(datasetsView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedDatasetChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(objectsView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedObjectChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(datasetsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(datasetDoubleClicked(QTreeWidgetItem*, int)));
	connect(objectsView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(objectDoubleClicked(QTreeWidgetItem*, int)));
	connect(propertiesView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(propertyDoubleClicked(QTreeWidgetItem*, int)));
	connect(propertiesView, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(propertyClicked(QTreeWidgetItem*, int)));
	connect(vsplitter_l, SIGNAL(splitterMoved(int, int)), this, SLOT(vsplitterlMoved(int, int)));
	connect(vsplitter_r, SIGNAL(splitterMoved(int, int)), this, SLOT(vsplitterrMoved(int, int)));
	updateDatasets();
	if(!settings->datasets_geometry.isEmpty()) restoreGeometry(settings->datasets_geometry);
	else resize(1000, 700);
	vsplitter_l->setStretchFactor(0, 2);
	vsplitter_l->setStretchFactor(1, 3);
	vsplitter_r->setStretchFactor(0, 2);
	vsplitter_r->setStretchFactor(1, 3);
	hsplitter->setStretchFactor(0, 2);
	hsplitter->setStretchFactor(1, 3);
	if(!settings->datasets_vsplitter_state.isEmpty()) {
		vsplitter_l->restoreState(settings->datasets_vsplitter_state);
		vsplitter_r->restoreState(settings->datasets_vsplitter_state);
	}
	if(!settings->datasets_hsplitter_state.isEmpty()) hsplitter->restoreState(settings->datasets_hsplitter_state);
}
DataSetsDialog::~DataSetsDialog() {}

void DataSetsDialog::vsplitterlMoved(int, int) {
	vsplitter_r->setSizes(vsplitter_l->sizes());
}
void DataSetsDialog::vsplitterrMoved(int, int) {
	vsplitter_l->setSizes(vsplitter_r->sizes());
}
void DataSetsDialog::selectedDatasetChanged(QTreeWidgetItem *item, QTreeWidgetItem*) {
	objectsView->clear();
	if(!item) {
		objectsView->setColumnCount(1);
		descriptionView->clear();
		selected_dataset = NULL;
		return;
	}
	DataSet *ds = (DataSet*) item->data(0, Qt::UserRole).value<void*>();
	selected_dataset = ds;
	DataObjectIter it;
	DataPropertyIter pit;
	DataProperty *dp;
	DataObject *o = ds->getFirstObject(&it);
	dp = ds->getFirstProperty(&pit);
	int n = 0;
	while(dp) {
		if(!dp->isHidden() && dp->isKey()) n++;
		dp = ds->getNextProperty(&pit);
	}
	if(n == 0) n = 1;
	objectsView->setColumnCount(n);
	for(int i = 0; i < n; i++) objectsView->headerItem()->setText(i, QString());
	n = 0;
	if(o) selectedObjectChanged(NULL, NULL);
	while(o) {
		dp = ds->getFirstProperty(&pit);
		QTreeWidgetItem *item = new QTreeWidgetItem(objectsView);
		for(int i = 0; dp; i++) {
			if(!dp->isHidden() && dp->isKey()) item->setText(i, QString::fromStdString(o->getPropertyDisplayString(dp)));
			dp = ds->getNextProperty(&pit);
		}
		item->setData(0, Qt::UserRole, QVariant::fromValue((void*) o));
		n++;
		if(o == selected_object || (n == 1 && !selected_object)) {
			item->setSelected(true);
			selectedObjectChanged(item, NULL);
		}
		o = ds->getNextObject(&it);
	}
	if(n == 1) objectsView->sortItems(0, Qt::AscendingOrder);
	QString str;
	if(!ds->description().empty()) {
		str = QString::fromStdString(ds->description());
		str += "<br>";
		str += "<br>";
	}
	str += "<b>";
	str += tr("Properties");
	str += "</b>";
	str += "<br>";
	dp = ds->getFirstProperty(&pit);
	while(dp) {
		if(!dp->isHidden()) {
			if(!dp->title(false).empty()) {
				str += QString::fromStdString(dp->title());
				str += ": ";
			}
			for(size_t i = 1; i <= dp->countNames(); i++) {
				if(i > 1) str += ", ";
				str += QString::fromStdString(dp->getName(i));
			}
			if(dp->isKey()) {
				str += " (";
				str += tr("key");
				str += ")";
			}
			str += "<br>";
			if(!dp->description().empty()) {
				str += "<i>";
				str += QString::fromStdString(dp->description());
				str += "</i>";
				str += "<br>";
			}
		}
		dp = ds->getNextProperty(&pit);
	}
	str += "<br>";
	str += "<b>";
	str += tr("Data Retrieval Function");
	str += "</b>";
	Argument *arg;
	Argument default_arg;
	const ExpressionName *ename = &ds->preferredName(false, true, false, false, &can_display_unicode_string_function, (void*) descriptionView);
	str += "<br>";
	str += "<i><b>";
	str += QString::fromStdString(ename->name);
	str += "</b>";
	int iargs = ds->maxargs();
	if(iargs < 0) {
		iargs = ds->minargs() + 1;
	}
	str += "(";
	if(iargs != 0) {
		for(int i2 = 1; i2 <= iargs; i2++) {
			if(i2 > ds->minargs()) {
				str += "[";
			}
			if(i2 > 1) {
				str += QString::fromStdString(CALCULATOR->getComma());
				str += " ";
			}
			arg = ds->getArgumentDefinition(i2);
			if(arg && !arg->name().empty()) {
				str += QString::fromStdString(arg->name());
			} else {
				str += tr("argument");
				str += " ";
				str += QString::number(i2);
			}
			if(i2 > ds->minargs()) {
				str += "]";
			}
		}
		if(ds->maxargs() < 0) {
			str += QString::fromStdString(CALCULATOR->getComma());
			str += " …";
		}
	}
	str += ")";
	for(size_t i2 = 1; i2 <= ds->countNames(); i2++) {
		if(&ds->getName(i2) != ename) {
			str += "<br>";
			str += QString::fromStdString(ds->getName(i2).name);
		}
	}
	str += "</i>";
	str += "<br>";
	if(!ds->copyright().empty()) {
		str += "<br>";
		str += QString::fromStdString(ds->copyright());
		str += "<br>";
	}
	descriptionView->setHtml(str);
}
void DataSetsDialog::selectedObjectChanged(QTreeWidgetItem *item, QTreeWidgetItem*) {
	propertiesView->clear();
	propertiesView->setColumnCount(3);
	if(!item) {
		selected_object = NULL;
		return;
	}
	DataObject *o = (DataObject*) item->data(0, Qt::UserRole).value<void*>();
	selected_object = o;
	DataSet *ds = o->parentSet();
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	QFont bold_font(font());
	bold_font.setWeight(QFont::Bold);
	while(dp) {
		if(!dp->isHidden()) {
			QTreeWidgetItem *item = new QTreeWidgetItem(propertiesView);
			item->setText(0, QString::fromStdString(dp->title()));
			item->setText(1, QString::fromStdString(o->getPropertyDisplayString(dp)));
			item->setData(0, Qt::UserRole, QVariant::fromValue((void*) dp));
			item->setFont(0, bold_font);
			item->setText(2, QString());
			item->setIcon(2, LOAD_ICON("edit-paste"));
			item->setTextAlignment(2, Qt::AlignRight);
		}
		dp = ds->getNextProperty(&it);
	}
}
void DataSetsDialog::datasetDoubleClicked(QTreeWidgetItem*, int) {}
void DataSetsDialog::objectDoubleClicked(QTreeWidgetItem*, int) {}
void DataSetsDialog::propertyDoubleClicked(QTreeWidgetItem *item, int) {
	if(!item || !selected_object) return;
	emit insertPropertyRequest(selected_object, (DataProperty*) item->data(0, Qt::UserRole).value<void*>());
}
void DataSetsDialog::propertyClicked(QTreeWidgetItem *item, int c) {
	if(c != 2 || !item || !selected_object) return;
	emit insertPropertyRequest(selected_object, (DataProperty*) item->data(0, Qt::UserRole).value<void*>());
}
void DataSetsDialog::updateDatasets() {
	DataSet *ds;
	for(size_t i = 1; ; i++) {
		ds = CALCULATOR->getDataSet(i);
		if(!ds) break;
		QTreeWidgetItem *item = new QTreeWidgetItem(datasetsView, QStringList(QString::fromStdString(ds->title())));
		item->setData(0, Qt::UserRole, QVariant::fromValue((void*) ds));
		if((!ds && i == 1) || ds == selected_dataset) {
			item->setSelected(true);
		}
	}
}
void DataSetsDialog::closeEvent(QCloseEvent *e) {
	settings->datasets_geometry = saveGeometry();
	settings->datasets_vsplitter_state = vsplitter_l->saveState();
	settings->datasets_hsplitter_state = hsplitter->saveState();
	QDialog::closeEvent(e);
}
void DataSetsDialog::reject() {
	settings->datasets_geometry = saveGeometry();
	settings->datasets_vsplitter_state = vsplitter_l->saveState();
	settings->datasets_hsplitter_state = hsplitter->saveState();
	QDialog::reject();
}

