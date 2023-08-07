/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QAction>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "unknowneditdialog.h"
#include "functioneditdialog.h"

UnknownEditDialog::UnknownEditDialog(QWidget *parent) : QDialog(parent) {
	o_variable = NULL;
	name_edited = false;
	namesEditDialog = NULL;
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	connect(nameEdit->addAction(LOAD_ICON("configure"), QLineEdit::TrailingPosition), SIGNAL(triggered()), this, SLOT(editNames()));
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
		nameEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	grid->addWidget(nameEdit, 0, 1);
	customBox = new QCheckBox(tr("Custom assumptions"), this);
	customBox->setChecked(true);
	grid->addWidget(customBox, 1, 0, 1, 2);
	grid->addWidget(new QLabel(tr("Type:"), this), 2, 0);
	typeCombo = new QComboBox(this);
	typeCombo->addItem("Number", ASSUMPTION_TYPE_NUMBER);
	typeCombo->addItem("Real number", ASSUMPTION_TYPE_REAL);
	typeCombo->addItem("Rational number", ASSUMPTION_TYPE_RATIONAL);
	typeCombo->addItem("Integer", ASSUMPTION_TYPE_INTEGER);
	typeCombo->addItem("Boolean", ASSUMPTION_TYPE_BOOLEAN);
	grid->addWidget(typeCombo, 2, 1);
	grid->addWidget(new QLabel(tr("Sign:"), this), 3, 0);
	signCombo = new QComboBox(this);
	signCombo->addItem("Unknown", ASSUMPTION_SIGN_UNKNOWN);
	signCombo->addItem("Non-zero", ASSUMPTION_SIGN_NONZERO);
	signCombo->addItem("Positive", ASSUMPTION_SIGN_POSITIVE);
	signCombo->addItem("Non-negative", ASSUMPTION_SIGN_NONNEGATIVE);
	signCombo->addItem("Negative", ASSUMPTION_SIGN_NEGATIVE);
	signCombo->addItem("Non-positive", ASSUMPTION_SIGN_NONPOSITIVE);
	grid->addWidget(signCombo, 3, 1);
	temporaryBox = new QCheckBox(tr("Temporary"), this);
	temporaryBox->setChecked(false);
	grid->addWidget(temporaryBox, 4, 0, 1, 2, Qt::AlignRight);
	typeCombo->setCurrentIndex(typeCombo->findData(CALCULATOR->defaultAssumptions()->type()));
	signCombo->setCurrentIndex(signCombo->findData(CALCULATOR->defaultAssumptions()->sign()));
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeChanged(int)));
	connect(signCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onSignChanged(int)));
	connect(customBox, SIGNAL(toggled(bool)), this, SLOT(onCustomToggled(bool)));
	connect(temporaryBox, SIGNAL(clicked()), this, SLOT(onVariableChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
UnknownEditDialog::~UnknownEditDialog() {}

void UnknownEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(TYPE_VARIABLE, this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_variable, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
	onVariableChanged();
}
void UnknownEditDialog::onTypeChanged(int i) {
	int t = typeCombo->itemData(i).toInt();
	int s = signCombo->currentData().toInt();
	if((t == ASSUMPTION_TYPE_NUMBER && s != ASSUMPTION_SIGN_NONZERO && s != ASSUMPTION_SIGN_UNKNOWN) || (t == ASSUMPTION_TYPE_BOOLEAN && s != ASSUMPTION_SIGN_UNKNOWN)) {
		signCombo->blockSignals(true);
		signCombo->setCurrentIndex(signCombo->findData(ASSUMPTION_SIGN_UNKNOWN));
		signCombo->blockSignals(false);
	}
	onVariableChanged();
}
void UnknownEditDialog::onSignChanged(int i) {
	int t = typeCombo->currentData().toInt();
	int s = signCombo->itemData(i).toInt();
	if((t == ASSUMPTION_TYPE_NUMBER && s != ASSUMPTION_SIGN_NONZERO && s != ASSUMPTION_SIGN_UNKNOWN) || (t == ASSUMPTION_TYPE_BOOLEAN && s != ASSUMPTION_SIGN_UNKNOWN)) {
		typeCombo->blockSignals(true);
		typeCombo->setCurrentIndex(typeCombo->findData(ASSUMPTION_TYPE_REAL));
		typeCombo->blockSignals(false);
	}
	onVariableChanged();
}
void UnknownEditDialog::onVariableChanged() {
	okButton->setEnabled(!nameEdit->text().trimmed().isEmpty());
}
void UnknownEditDialog::onCustomToggled(bool b) {
	typeCombo->setEnabled(b);
	signCombo->setEnabled(b);
	onVariableChanged();
}
UnknownVariable *UnknownEditDialog::createVariable(ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	Variable *var = NULL;
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString())) {
		var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString(), true);
		if(name_edited && (!var || var->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			if(!var) *replaced_item = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString(), true);
			else *replaced_item = var;
		}
	}
	UnknownVariable *v;
	if(var && var->isLocal() && !var->isKnown()) {
		v = (UnknownVariable*) var;
		v->clearNames(); v->setHidden(false); v->setApproximate(false); v->setDescription(""); v->setTitle("");
		if(!modifyVariable(v)) return NULL;
		return v;
	}
	v = new UnknownVariable("", "");
	if(namesEditDialog) namesEditDialog->modifyNames(v, nameEdit->text());
	else NamesEditDialog::modifyName(v, nameEdit->text());
	if(temporaryBox->isChecked()) v->setCategory(CALCULATOR->temporaryCategory());
	else v->setCategory(CALCULATOR->getVariableById(VARIABLE_ID_X)->category());
	if(customBox->isChecked()) {
		v->setAssumptions(new Assumptions());
		v->assumptions()->setType((AssumptionType) typeCombo->currentData().toInt());
		v->assumptions()->setSign((AssumptionSign) signCombo->currentData().toInt());
	}
	v->setChanged(false);
	CALCULATOR->addVariable(v);
	return v;
}
bool UnknownEditDialog::modifyVariable(UnknownVariable *v, ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString(), v)) {
		Variable *var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString(), true);
		if(name_edited && (!var || var->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			if(!var) *replaced_item = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString(), true);
			else if(var != v) *replaced_item = var;
		}
	}
	if(v->isLocal()) {
		if(namesEditDialog) namesEditDialog->modifyNames(v, nameEdit->text());
		else NamesEditDialog::modifyName(v, nameEdit->text());
		if(temporaryBox->isChecked()) v->setCategory(CALCULATOR->temporaryCategory());
		else if(v->category() == CALCULATOR->temporaryCategory()) v->setCategory(CALCULATOR->getVariableById(VARIABLE_ID_X)->category());
	}
	if(!customBox->isChecked()) {
		v->setAssumptions(NULL);
	} else {
		if(!v->assumptions()) v->setAssumptions(new Assumptions());
		v->assumptions()->setType((AssumptionType) typeCombo->currentData().toInt());
		v->assumptions()->setSign((AssumptionSign) signCombo->currentData().toInt());
	}
	return true;
}
void UnknownEditDialog::setVariable(UnknownVariable *v) {
	o_variable = v;
	name_edited = false;
	nameEdit->setText(QString::fromStdString(v->getName(1).name));
	nameEdit->setEnabled(v->isLocal());
	if(namesEditDialog) namesEditDialog->setNames(v, nameEdit->text());
	Assumptions *ass = v->assumptions();
	customBox->setChecked(ass);
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	typeCombo->setCurrentIndex(typeCombo->findData(ass->type()));
	signCombo->setCurrentIndex(signCombo->findData(ass->sign()));
	temporaryBox->setChecked(v->category() == CALCULATOR->temporaryCategory());
	temporaryBox->setEnabled(v->isLocal());
	okButton->setEnabled(false);

}
void UnknownEditDialog::onNameEdited(const QString &str) {
	if(!str.trimmed().isEmpty() && !CALCULATOR->variableNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidVariableName(str.trimmed().toStdString())));
	}
	name_edited = true;
	okButton->setEnabled(!nameEdit->text().trimmed().isEmpty());
}
void UnknownEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
bool UnknownEditDialog::editVariable(QWidget *parent, UnknownVariable *v, ExpressionItem **replaced_item) {
	UnknownEditDialog *d = new UnknownEditDialog(parent);
	d->setWindowTitle(tr("Edit Unknown Variable"));
	d->setVariable(v);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyVariable(v, replaced_item)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
UnknownVariable* UnknownEditDialog::newVariable(QWidget *parent, ExpressionItem **replaced_item) {
	UnknownEditDialog *d = new UnknownEditDialog(parent);
	d->setWindowTitle(tr("New Unknown Variable"));
	std::string v_name;
	int i = 1;
	do {
		v_name = "v"; v_name += i2s(i);
		i++;
	} while(CALCULATOR->nameTaken(v_name));
	d->setName(QString::fromStdString(v_name));
	UnknownVariable *v = NULL;
	while(d->exec() == QDialog::Accepted) {
		v = d->createVariable(replaced_item);
		if(v) break;
	}
	d->deleteLater();
	return v;
}

