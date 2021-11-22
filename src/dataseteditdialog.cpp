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
#include <QTreeWidget>
#include <QLabel>
#include <QLineEdit>
#include <QAction>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QHeaderView>
#include <QCheckBox>
#include <QMessageBox>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functioneditdialog.h"
#include "dataseteditdialog.h"

DataPropertyEditDialog::DataPropertyEditDialog(QWidget *parent) : QDialog(parent) {
	o_property = NULL;
	name_edited = false;
	namesEditDialog = NULL;
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	connect(nameEdit->addAction(LOAD_ICON("configure"), QLineEdit::TrailingPosition), SIGNAL(triggered()), this, SLOT(editNames()));
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
		nameEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Descriptive name:"), this), 1, 0);
	titleEdit = new QLineEdit(this);
	grid->addWidget(titleEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 2, 0, 1, 2);
	descriptionEdit = new SmallTextEdit(3, this);
	grid->addWidget(descriptionEdit, 3, 0, 1, 2);
	grid->addWidget(new QLabel(tr("Value type:"), this), 4, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setEditable(false);
	typeCombo->addItem(tr("Expression"), PROPERTY_EXPRESSION);
	typeCombo->addItem(tr("Number"), PROPERTY_NUMBER);
	typeCombo->addItem(tr("Text"), PROPERTY_STRING);
	typeCombo->setCurrentIndex(0);
	grid->addWidget(typeCombo, 4, 1);
	grid->addWidget(new QLabel(tr("Unit expression:"), this), 5, 0);
	unitEdit = new MathLineEdit(this, true);
	grid->addWidget(unitEdit, 5, 1);
	keyBox = new QCheckBox(tr("Use as key"), this);
	grid->addWidget(keyBox, 6, 0, 1, 2);
	caseBox = new QCheckBox(tr("Case sensitive value"), this);
	grid->addWidget(caseBox, 7, 0, 1, 2);
	approxBox = new QCheckBox(tr("Approximate value"), this);
	grid->addWidget(approxBox, 8, 0, 1, 2);
	bracketsBox = new QCheckBox(tr("Value uses brackets"), this);
	grid->addWidget(bracketsBox, 9, 0, 1, 2);
	bracketsBox->hide();
	hideBox = new QCheckBox(tr("Hide"), this);
	grid->addWidget(hideBox, 10, 0, 1, 2);
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	nameEdit->setFocus();
	typeChanged(0);
	connect(typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(onPropertyChanged()));
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onPropertyChanged()));
	connect(descriptionEdit, SIGNAL(textChanged()), this, SLOT(onPropertyChanged()));
	connect(titleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onPropertyChanged()));
	connect(unitEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onPropertyChanged()));
	connect(hideBox, SIGNAL(clicked()), this, SLOT(onPropertyChanged()));
	connect(caseBox, SIGNAL(clicked()), this, SLOT(onPropertyChanged()));
	connect(keyBox, SIGNAL(clicked()), this, SLOT(onPropertyChanged()));
	connect(approxBox, SIGNAL(clicked()), this, SLOT(onPropertyChanged()));
	connect(bracketsBox, SIGNAL(clicked()), this, SLOT(onPropertyChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
DataPropertyEditDialog::~DataPropertyEditDialog() {}

void DataPropertyEditDialog::typeChanged(int i) {
	keyBox->setEnabled(i != 0);
	caseBox->setEnabled(i == 2);
	approxBox->setEnabled(i != 2);
	bracketsBox->setEnabled(i != 2);
	unitEdit->setEnabled(i != 2);
}
void DataPropertyEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(-1, this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_property, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
	onPropertyChanged();
}
void DataPropertyEditDialog::onPropertyChanged() {
	okButton->setEnabled(!nameEdit->isReadOnly() && !nameEdit->text().trimmed().isEmpty());
}
DataProperty *DataPropertyEditDialog::createProperty(DataSet *ds) {
	DataProperty *dp = new DataProperty(ds);
	modifyProperty(dp);
	return dp;
}
bool DataPropertyEditDialog::modifyProperty(DataProperty *dp) {
	dp->setUserModified(true);
	dp->setTitle(titleEdit->text().trimmed().toStdString());
	dp->setUnit(settings->unlocalizeExpression(unitEdit->text().trimmed().toStdString()));
	dp->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	dp->setHidden(hideBox->isChecked());
	dp->setKey(keyBox->isChecked());
	dp->setApproximate(approxBox->isChecked());
	dp->setCaseSensitive(caseBox->isChecked());
	dp->setUsesBrackets(bracketsBox->isChecked());
	dp->setPropertyType((PropertyType) typeCombo->currentData().toInt());
	if(namesEditDialog) namesEditDialog->modifyNames(dp, nameEdit->text());
	else NamesEditDialog::modifyName(dp, nameEdit->text());
	return true;
}
void DataPropertyEditDialog::setProperty(DataProperty *dp) {
	o_property = dp;
	name_edited = false;
	bool read_only = dp->parentSet() && !dp->parentSet()->isLocal();
	nameEdit->setText(QString::fromStdString(dp->getName()));
	if(namesEditDialog) namesEditDialog->setNames(dp, nameEdit->text());
	unitEdit->setText(QString::fromStdString(settings->localizeExpression(dp->getUnitString(), true)));
	descriptionEdit->blockSignals(true);
	descriptionEdit->setPlainText(QString::fromStdString(dp->description()));
	descriptionEdit->blockSignals(false);
	typeCombo->setCurrentIndex(typeCombo->findData(dp->propertyType()));
	titleEdit->setText(QString::fromStdString(dp->title(false)));
	hideBox->setChecked(dp->isHidden());
	keyBox->setChecked(dp->isKey());
	approxBox->setChecked(dp->isApproximate());
	caseBox->setChecked(dp->isCaseSensitive());
	bracketsBox->setChecked(dp->usesBrackets());
	descriptionEdit->setReadOnly(read_only);
	titleEdit->setReadOnly(read_only);
	okButton->setEnabled(false);
	nameEdit->setReadOnly(read_only);
	unitEdit->setReadOnly(read_only);
}
bool DataPropertyEditDialog::editProperty(QWidget *parent, DataProperty *dp) {
	DataPropertyEditDialog *d = new DataPropertyEditDialog(parent);
	d->setWindowTitle(tr("Edit Data Property"));
	d->setProperty(dp);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyProperty(dp)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
DataProperty *DataPropertyEditDialog::newProperty(QWidget *parent, DataSet *ds) {
	DataPropertyEditDialog *d = new DataPropertyEditDialog(parent);
	d->setWindowTitle(tr("Edit Data Property"));
	DataProperty *dp = NULL;
	while(d->exec() == QDialog::Accepted) {
		dp = d->createProperty(ds);
		if(dp) break;
	}
	d->deleteLater();
	return dp;
}


DataSetEditDialog::DataSetEditDialog(QWidget *parent) : QDialog(parent) {
	o_dataset = NULL;
	selected_property = NULL;
	name_edited = false;
	file_edited = false;
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
	grid->addWidget(new QLabel(tr("Descriptive name:"), this), 0, 0);
	titleEdit = new QLineEdit(this);
	grid->addWidget(titleEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Data file:"), this), 1, 0);
	fileEdit = new QLineEdit(this);
	grid->addWidget(fileEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 2, 0, 1, 2);
	descriptionEdit = new SmallTextEdit(3, this);
	grid->addWidget(descriptionEdit, 3, 0, 1, 2);
	grid->addWidget(new QLabel(tr("Copyright:"), this), 4, 0, 1, 2);
	copyrightEdit = new SmallTextEdit(3, this);
	grid->addWidget(copyrightEdit, 5, 0, 1, 2);
	grid = new QGridLayout(w2);
	propertiesView = new QTreeWidget(this);
	propertiesView->setSelectionMode(QAbstractItemView::SingleSelection);
	propertiesView->setRootIsDecorated(false);
	propertiesView->setAllColumnsShowFocus(true);
	propertiesView->setColumnCount(3);
	propertiesView->header()->setStretchLastSection(false);
	propertiesView->headerItem()->setText(0, tr("Title"));
	propertiesView->headerItem()->setText(1, tr("Name"));
	propertiesView->headerItem()->setText(2, tr("Type"));
	propertiesView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	propertiesView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	propertiesView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	grid->addWidget(propertiesView, 0, 0);
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addStretch(1);
	addButton = new QPushButton(tr("Add"), this);
	hbox->addWidget(addButton);
	editButton = new QPushButton(tr("Edit"), this);
	editButton->setEnabled(false);
	hbox->addWidget(editButton);
	delButton = new QPushButton(tr("Remove"), this);
	delButton->setEnabled(false);
	hbox->addWidget(delButton);
	grid->addLayout(hbox, 1, 0);
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
	grid->addWidget(new QLabel(tr("Object argument name:"), this), 1, 0);
	arg1Edit = new QLineEdit(this);
	grid->addWidget(arg1Edit, 1, 1);
	grid->addWidget(new QLabel(tr("Property argument name:"), this), 2, 0);
	arg2Edit = new QLineEdit(this);
	grid->addWidget(arg2Edit, 2, 1);
	grid->addWidget(new QLabel(tr("Default property:"), this), 3, 0, Qt::AlignTop);
	default2Edit = new QLineEdit(this);
	default2Edit->setText("info");
	grid->addWidget(default2Edit, 3, 1, Qt::AlignTop);
	grid->setRowStretch(3, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	titleEdit->setFocus();
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(fileEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onFileEdited(const QString&)));
	connect(titleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onTitleEdited(const QString&)));
	connect(descriptionEdit, SIGNAL(textChanged()), this, SLOT(onDatasetChanged()));
	connect(copyrightEdit, SIGNAL(textChanged()), this, SLOT(onDatasetChanged()));
	connect(arg1Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onDatasetChanged()));
	connect(arg2Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onDatasetChanged()));
	connect(default2Edit, SIGNAL(textEdited(const QString&)), this, SLOT(onDatasetChanged()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addProperty()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(editProperty()));
	connect(delButton, SIGNAL(clicked()), this, SLOT(delProperty()));
	connect(propertiesView, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(selectedPropertyChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(propertiesView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(editProperty()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
DataSetEditDialog::~DataSetEditDialog() {
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i]) delete tmp_props[i];
	}
	tmp_props.clear();
	tmp_props_orig.clear();
}

void DataSetEditDialog::selectedPropertyChanged(QTreeWidgetItem *item, QTreeWidgetItem*) {
	if(!item) {
		editButton->setEnabled(false);
		delButton->setEnabled(false);
		selected_property = NULL;
		return;
	}
	selected_property = (DataProperty*) item->data(0, Qt::UserRole).value<void*>();
	editButton->setEnabled(true);
	delButton->setEnabled(!nameEdit->isReadOnly());
}
void DataSetEditDialog::setPropertyItemText(QTreeWidgetItem *item, DataProperty *dp) {
	QString str;
	switch(dp->propertyType()) {
		case PROPERTY_STRING: {
			str += tr("text");
			break;
		}
		case PROPERTY_NUMBER: {
			if(dp->isApproximate()) {
				str += tr("approximate");
				str += " ";
			}
			str += tr("number");
			break;
		}
		case PROPERTY_EXPRESSION: {
			if(dp->isApproximate()) {
				str += tr("approximate");
				str += " ";
			}
			str += tr("expression");
			break;
		}
	}
	if(dp->isKey()) {
		str += " (";
		str += tr("key");
		str += ")";
	}
	item->setText(0, QString::fromStdString(dp->title(false)));
	item->setText(1, QString::fromStdString(dp->getName()));
	item->setText(2, str);
}
void DataSetEditDialog::addProperty() {
	DataProperty *dp = DataPropertyEditDialog::newProperty(this, o_dataset);
	if(dp) {
		tmp_props.push_back(dp);
		tmp_props_orig.push_back(NULL);
		QTreeWidgetItem *item = new QTreeWidgetItem(propertiesView);
		setPropertyItemText(item, dp);
		item->setData(0, Qt::UserRole, QVariant::fromValue((void*) dp));
		propertiesView->clearSelection();
		propertiesView->setCurrentItem(item);
		item->setSelected(true);
		onDatasetChanged();
	}
}
void DataSetEditDialog::editProperty() {
	if(!selected_property) return;
	DataProperty *dp = selected_property;
	if(DataPropertyEditDialog::editProperty(this, dp)) {
		QTreeWidgetItem *item = propertiesView->currentItem();
		setPropertyItemText(item, dp);
		onDatasetChanged();
	}
}
void DataSetEditDialog::delProperty() {
	if(!selected_property || !selected_property->isUserModified()) return;
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i] == selected_property) {
			delete tmp_props[i];
			if(tmp_props_orig[i]) {
				tmp_props[i] = NULL;
			} else {
				tmp_props.erase(tmp_props.begin() + i);
				tmp_props_orig.erase(tmp_props_orig.begin() + i);
			}
			QTreeWidgetItem *item = propertiesView->currentItem();
			if(item) delete item;
			onDatasetChanged();
			break;
		}
	}
}
void DataSetEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(TYPE_FUNCTION, this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_dataset, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	if(!file_edited) fileEdit->setText(nameEdit->text());
	name_edited = false;
	onDatasetChanged();
}
void DataSetEditDialog::onFileEdited(const QString&) {
	file_edited = true;
	onDatasetChanged();
}
void DataSetEditDialog::onTitleEdited(const QString &str) {
	if(!file_edited) {
		QString sfile = str.simplified().toLower();
		sfile.replace(" ", "_");
		fileEdit->setText(sfile);
	}
	if(!name_edited && !namesEditDialog) {
		QString sname = str.simplified().toLower();
		sname.replace(" ", "_");
		if(!sname.trimmed().isEmpty() && !CALCULATOR->functionNameIsValid(sname.trimmed().toStdString())) {
			sname = QString::fromStdString(CALCULATOR->convertToValidFunctionName(sname.trimmed().toStdString()));
		}
		nameEdit->setText(sname);
	}
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
	okButton->setEnabled(!nameEdit->isReadOnly() && !nameEdit->text().trimmed().isEmpty() && !fileEdit->text().trimmed().isEmpty());
}
DataSet *DataSetEditDialog::createDataset(MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	MathFunction *func = NULL;
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString())) {
		func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
		if(name_edited && (!func || func->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			*replaced_item = func;
		}
	}
	DataSet *ds_atom = CALCULATOR->getDataSet("atom");
	DataSet *ds = new DataSet(ds_atom ? ds_atom->category() : tr("Data Sets").toStdString(), "", fileEdit->text().trimmed().toStdString(), titleEdit->text().trimmed().toStdString(), descriptionEdit->toPlainText().trimmed().toStdString(), true);
	Argument *arg = ds->getArgumentDefinition(1);
	if(arg) {
		if(arg1Edit->text().trimmed().isEmpty()) arg->setName(tr("Object").toStdString());
		else arg->setName(arg1Edit->text().trimmed().toStdString());
	}
	arg = ds->getArgumentDefinition(1);
	if(arg) {
		if(arg2Edit->text().trimmed().isEmpty()) arg->setName(tr("Property").toStdString());
		else arg->setName(arg2Edit->text().trimmed().toStdString());
	}
	ds->setDefaultProperty(default2Edit->text().trimmed().toStdString());
	ds->setCopyright(copyrightEdit->toPlainText().trimmed().toStdString());
	for(size_t i = 0; i < tmp_props.size();) {
		if(!tmp_props[i]) {
			if(tmp_props_orig[i]) ds->delProperty(tmp_props_orig[i]);
			i++;
		} else if(tmp_props[i]->isUserModified()) {
			if(tmp_props_orig[i]) {
				tmp_props_orig[i]->set(*tmp_props[i]);
				i++;
			} else {
				ds->addProperty(tmp_props[i]);
				tmp_props.erase(tmp_props.begin() + i);
			}
		} else {
			i++;
		}
	}
	if(namesEditDialog) namesEditDialog->modifyNames(ds, nameEdit->text());
	else NamesEditDialog::modifyName(ds, nameEdit->text());
	CALCULATOR->addDataSet(ds);
	ds->loadObjects();
	ds->setObjectsLoaded(true);
	return ds;
}
bool DataSetEditDialog::modifyDataset(DataSet *ds, MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString(), ds)) {
		MathFunction *func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
		if(name_edited && (!func || func->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			*replaced_item = func;
		}
	}
	if(ds->isLocal()) ds->setDefaultDataFile(fileEdit->text().trimmed().toStdString());
	ds->setTitle(titleEdit->text().trimmed().toStdString());
	ds->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	Argument *arg = ds->getArgumentDefinition(1);
	if(arg) {
		if(arg1Edit->text().trimmed().isEmpty()) arg->setName(tr("Object").toStdString());
		else arg->setName(arg1Edit->text().trimmed().toStdString());
	}
	arg = ds->getArgumentDefinition(1);
	if(arg) {
		if(arg2Edit->text().trimmed().isEmpty()) arg->setName(tr("Property").toStdString());
		else arg->setName(arg2Edit->text().trimmed().toStdString());
	}
	ds->setDefaultProperty(default2Edit->text().trimmed().toStdString());
	ds->setCopyright(copyrightEdit->toPlainText().trimmed().toStdString());
	for(size_t i = 0; i < tmp_props.size();) {
		if(!tmp_props[i]) {
			if(tmp_props_orig[i]) ds->delProperty(tmp_props_orig[i]);
			i++;
		} else if(tmp_props[i]->isUserModified()) {
			if(tmp_props_orig[i]) {
				tmp_props_orig[i]->set(*tmp_props[i]);
				i++;
			} else {
				ds->addProperty(tmp_props[i]);
				tmp_props.erase(tmp_props.begin() + i);
			}
		} else {
			i++;
		}
	}
	if(namesEditDialog) namesEditDialog->modifyNames(ds, nameEdit->text());
	else NamesEditDialog::modifyName(ds, nameEdit->text());
	return true;
}
void DataSetEditDialog::setDataset(DataSet *ds) {
	o_dataset = ds;
	name_edited = false;
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i]) delete tmp_props[i];
	}
	tmp_props.clear();
	tmp_props_orig.clear();
	bool read_only = !ds->isLocal();
	nameEdit->setText(QString::fromStdString(ds->getName(1).name));
	if(namesEditDialog) namesEditDialog->setNames(ds, nameEdit->text());
	descriptionEdit->blockSignals(true);
	descriptionEdit->setPlainText(QString::fromStdString(ds->description()));
	descriptionEdit->blockSignals(false);
	copyrightEdit->blockSignals(true);
	copyrightEdit->setPlainText(QString::fromStdString(ds->copyright()));
	copyrightEdit->blockSignals(false);
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	while(dp) {
		tmp_props.push_back(new DataProperty(*dp));
		tmp_props_orig.push_back(dp);
		dp = ds->getNextProperty(&it);
	}
	titleEdit->setText(QString::fromStdString(ds->title(false)));
	fileEdit->setText(QString::fromStdString(ds->defaultDataFile()));
	default2Edit->setText(QString::fromStdString(ds->defaultProperty()));
	Argument *arg = ds->getArgumentDefinition(1);
	if(arg) arg1Edit->setText(QString::fromStdString(arg->name()));
	arg = ds->getArgumentDefinition(2);
	if(arg) arg2Edit->setText(QString::fromStdString(arg->name()));
	descriptionEdit->setReadOnly(read_only);
	copyrightEdit->setReadOnly(read_only);
	arg1Edit->setReadOnly(read_only);
	arg2Edit->setReadOnly(read_only);
	default2Edit->setReadOnly(read_only);
	fileEdit->setReadOnly(read_only);
	file_edited = true;
	titleEdit->setReadOnly(read_only);
	okButton->setEnabled(false);
	nameEdit->setReadOnly(read_only);
	addButton->setEnabled(!read_only);
	editButton->setEnabled(false);
	delButton->setEnabled(false);
	selected_property = NULL;
	propertiesView->clear();
	propertiesView->setColumnCount(3);
	propertiesView->headerItem()->setText(0, tr("Title"));
	propertiesView->headerItem()->setText(1, tr("Name"));
	propertiesView->headerItem()->setText(2, tr("Type"));
	for(size_t i = 0; i < tmp_props.size(); i++) {
		DataProperty *dp = tmp_props[i];
		if(dp) {
			QTreeWidgetItem *item = new QTreeWidgetItem(propertiesView);
			setPropertyItemText(item, dp);
			item->setData(0, Qt::UserRole, QVariant::fromValue((void*) dp));
			if(i == 0) item->setSelected(true);
		}
	}
}
bool DataSetEditDialog::editDataset(QWidget *parent, DataSet *ds, MathFunction **replaced_item) {
	DataSetEditDialog *d = new DataSetEditDialog(parent);
	d->setWindowTitle(tr("Edit Data Set"));
	d->setDataset(ds);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyDataset(ds, replaced_item)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
DataSet *DataSetEditDialog::newDataset(QWidget *parent, MathFunction **replaced_item) {
	DataSetEditDialog *d = new DataSetEditDialog(parent);
	d->setWindowTitle(tr("New Data Set"));
	DataSet *ds = NULL;
	while(d->exec() == QDialog::Accepted) {
		ds = d->createDataset(replaced_item);
		if(ds) break;
	}
	d->deleteLater();
	return ds;
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

