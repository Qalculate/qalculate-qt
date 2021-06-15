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
#include <QDebug>

#include "qalculateqtsettings.h"
#include "variableeditdialog.h"

VariableEditDialog::VariableEditDialog(QWidget *parent, bool allow_empty_value) : QDialog(parent), b_empty(allow_empty_value) {
	QGridLayout *grid = new QGridLayout(this);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Value:"), this), 1, 0);
	valueEdit = new QLineEdit(this);
	if(b_empty) valueEdit->setPlaceholderText(tr("current result"));
	grid->addWidget(valueEdit, 1, 1);
	temporaryBox = new QCheckBox(tr("Temporary"), this);
	temporaryBox->setChecked(true);
	grid->addWidget(temporaryBox, 2, 0, 1, 2, Qt::AlignRight);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	grid->addWidget(buttonBox, 3, 0, 1, 2);
	connect(nameEdit, SLOT(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	if(!b_empty) connect(valueEdit, SLOT(textEdited(const QString&)), this, SLOT(onValueEdited(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
}
VariableEditDialog::~VariableEditDialog() {}

KnownVariable *VariableEditDialog::createVariable(MathStructure *default_value) {
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString())) {
		if(QMessageBox::question(this, tr("Question"), tr("An unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
	}
	KnownVariable *v;
	Variable *var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
	if(var && var->isLocal() && var->isKnown()) {
		v = (KnownVariable*) var;
		editVariable(v, default_value);
		return v;
	}
	if(default_value && valueEdit->text().isEmpty()) {
		v = new KnownVariable("", nameEdit->text().trimmed().toStdString(), *default_value);
	} else {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		v = new KnownVariable("", nameEdit->text().trimmed().toStdString(), CALCULATOR->unlocalizeExpression(valueEdit->text().trimmed().toStdString(), pa));
	}
	if(temporaryBox->isChecked()) v->setCategory(CALCULATOR->temporaryCategory());
	v->setChanged(false);
	CALCULATOR->addVariable(v);
	return v;
}
void VariableEditDialog::editVariable(KnownVariable *v, MathStructure *default_value) {
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString(), v)) {
		if(QMessageBox::question(this, tr("Question"), tr("An unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return;
		}
	}
	if(v->countNames() > 1) v->clearNames();
	v->setName(nameEdit->text().trimmed().toStdString());
	if(default_value && valueEdit->text().isEmpty()) {
		v->set(*default_value);
	} else {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		v->set(CALCULATOR->unlocalizeExpression(valueEdit->text().trimmed().toStdString(), pa));
	}
	if(temporaryBox->isChecked()) v->setCategory(CALCULATOR->temporaryCategory());
	else if(v->category() == CALCULATOR->temporaryCategory()) v->setCategory("");
}
void VariableEditDialog::setVariable(KnownVariable *v) {
	nameEdit->setText(QString::fromStdString(v->getName(1).name));
	if(v->isExpression()) {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		valueEdit->setText(QString::fromStdString(CALCULATOR->localizeExpression(v->expression(), pa)));
	} else {
		PrintOptions po = settings->printops;
		po.allow_non_usable = false;
		po.base = 10;
		valueEdit->setText(QString::fromStdString(CALCULATOR->print(v->get(), 100, po)));
	}
	okButton->setEnabled(true);
}
void VariableEditDialog::onNameEdited(const QString &str) {
	okButton->setEnabled(!str.trimmed().isEmpty() && (b_empty || !valueEdit->text().trimmed().isEmpty()));
	if(!str.trimmed().isEmpty() && !CALCULATOR->variableNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidVariableName(str.trimmed().toStdString())));
	}
}
void VariableEditDialog::onValueEdited(const QString &str) {
	okButton->setEnabled(!str.trimmed().isEmpty() && !nameEdit->text().trimmed().isEmpty());
}
void VariableEditDialog::setValue(const QString &str) {
	valueEdit->setText(str);
	if(!b_empty) onValueEdited(str);
}

