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
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QAction>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functioneditdialog.h"
#include "dataseteditdialog.h"

DataSetEditDialog::DataSetEditDialog(QWidget *parent) : QDialog(parent) {
	o_dataset = NULL;
	name_edited = false;
	namesEditDialog = NULL;
	QVBoxLayout *box = new QVBoxLayout(this);
	tabs = new QTabWidget(this);
	tabs->setUsesScrollButtons(false);
	box->addWidget(tabs);
	QWidget *w1 = new QWidget(this);
	QWidget *w2 = new QWidget(this);
	QWidget *w3 = new QWidget(this);
	tabs->addTab(w1, tr("General"));
	tabs->addTab(w2, tr("Properties"));
	tabs->addTab(w3, tr("Function"));
	QGridLayout *grid = new QGridLayout(w1);
	grid->addWidget(new QLabel(tr("Title:"), this), 0, 0);
	titleEdit = new QLineEdit(this);
	grid->addWidget(titleEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Data file:"), this), 1, 0);
	fileEdit = new QLineEdit(this);
	grid->addWidget(fileEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 2, 0, 1, 2);
	descriptionEdit = new SmallTextEdit(3, this);
	grid->addWidget(descriptionEdit, 5, 0, 1, 2);
	grid->addWidget(new QLabel(tr("Copyright:"), this), 4, 0, 1, 2);
	copyrightEdit = new SmallTextEdit(3, this);
	grid->addWidget(copyrightEdit, 5, 0, 1, 2);
	grid = new QGridLayout(w2);
	grid = new QGridLayout(w3);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	connect(nameEdit->addAction(LOAD_ICON("configure"), QLineEdit::TrailingPosition), SIGNAL(triggered()), this, SLOT(editNames()));
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
			nameEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	grid->addWidget(nameEdit, 0, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	nameEdit->setFocus();
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(descriptionEdit, SIGNAL(textChanged()), this, SLOT(onDatasetChanged()));
	connect(copyrightEdit, SIGNAL(textChanged()), this, SLOT(onDatasetChanged()));
	connect(titleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onDatasetChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
DataSetEditDialog::~DataSetEditDialog() {}

void DataSetEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(TYPE_FUNCTION, this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_dataset, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
	onDatasetChanged();
}
void DataSetEditDialog::onNameEdited(const QString &str) {
	if(!str.trimmed().isEmpty() && !CALCULATOR->functionNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidFunctionName(str.trimmed().toStdString())));
	}
	onDatasetChanged();
	name_edited = true;
}
void DataSetEditDialog::onDatasetChanged() {
	okButton->setEnabled(!nameEdit->isReadOnly() && !nameEdit->text().trimmed().isEmpty());
}

DataObjectEditDialog::DataObjectEditDialog(DataSet *o, QWidget *parent) : QDialog(parent), ds(o) {
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	int r = 0;
	while(dp) {
		grid->addWidget(new QLabel(tr("%1:").arg(QString::fromStdString(dp->title())), this), r, 0);
		QLineEdit *w;
		if(dp->propertyType() == PROPERTY_EXPRESSION) w = new MathLineEdit(this);
		else w = new QLineEdit(this);
		valueEdit << w;
		grid->addWidget(w, r, 1);
		connect(w, SIGNAL(textEdited(const QString&)), this, SLOT(onObjectChanged()));
		grid->addWidget(new QLabel(QString::fromStdString(settings->localizeExpression(dp->getUnitString(), true)), this), r, 2);
		QComboBox *combo = NULL;
		if(dp->propertyType() != PROPERTY_STRING) {
			combo = new QComboBox(this);
			combo->setEditable(false);
			combo->addItem(tr("Default"));
			combo->addItem(tr("Approximate"));
			combo->addItem(tr("Exact"));
			combo->setCurrentIndex(0);
			grid->addWidget(combo, r, 3);
			connect(combo, SIGNAL(activated(int)), this, SLOT(onObjectChanged()));
		}
		approxCombo << combo;
		r++;
		dp = ds->getNextProperty(&it);
	}
	if(!valueEdit.isEmpty()) valueEdit.at(0)->setFocus();
	grid->setColumnStretch(1, 1);
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
DataObjectEditDialog::~DataObjectEditDialog() {}

void DataObjectEditDialog::onObjectChanged() {
	okButton->setEnabled(!valueEdit.isEmpty() && !valueEdit.at(0)->isReadOnly());
}
void DataObjectEditDialog::setObject(DataObject *obj) {
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	int r = 0;
	while(dp) {
		QLineEdit *w = valueEdit.at(r);
		QComboBox *combo = approxCombo.at(r);
		int iapprox = -1;
		w->setReadOnly(!obj->isUserModified());
		w->setText(QString::fromStdString(settings->localizeExpression(obj->getProperty(dp, &iapprox))));
		if(combo) {
			combo->setCurrentIndex(iapprox < 0 ? 0 : (iapprox > 0 ? 2 : 1));
		}
		r++;
		dp = ds->getNextProperty(&it);
	}
	okButton->setEnabled(false);
}
DataObject *DataObjectEditDialog::createObject() {
	DataPropertyIter it;
	DataObject *obj = new DataObject(ds);
	ds->addObject(obj);
	DataProperty *dp = ds->getFirstProperty(&it);
	int r = 0;
	while(dp) {
		if(valueEdit.at(r)->text().trimmed().isEmpty()) {
			obj->eraseProperty(dp);
		} else {
			obj->setProperty(dp, settings->unlocalizeExpression(valueEdit.at(r)->text().trimmed().toStdString()), approxCombo.at(r) ? approxCombo.at(r)->currentIndex() - 1 : -1);
		}
		r++;
		dp = ds->getNextProperty(&it);
	}
	obj->setUserModified();
	return obj;
}
bool DataObjectEditDialog::modifyObject(DataObject *obj) {
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	int r = 0;
	while(dp) {
		if(valueEdit.at(r)->text().trimmed().isEmpty()) {
			obj->eraseProperty(dp);
		} else {
			obj->setProperty(dp, settings->unlocalizeExpression(valueEdit.at(r)->text().trimmed().toStdString()), approxCombo.at(r) ? approxCombo.at(r)->currentIndex() - 1 : -1);
		}
		r++;
		dp = ds->getNextProperty(&it);
	}
	obj->setUserModified();
	return true;
}

bool DataObjectEditDialog::editObject(QWidget *parent, DataObject *obj) {
	DataObjectEditDialog *d = new DataObjectEditDialog(obj->parentSet(), parent);
	d->setWindowTitle(tr("Edit Data Object"));
	d->setObject(obj);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyObject(obj)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
DataObject *DataObjectEditDialog::newObject(QWidget *parent, DataSet *o) {
	DataObjectEditDialog *d = new DataObjectEditDialog(o, parent);
	d->setWindowTitle(tr("New Data Object"));
	DataObject *obj = NULL;
	while(d->exec() == QDialog::Accepted) {
		obj = d->createObject();
		if(obj) break;
	}
	d->deleteLater();
	return obj;
}

