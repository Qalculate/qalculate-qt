/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functioneditdialog.h"

FunctionEditDialog::FunctionEditDialog(QWidget *parent) : QDialog(parent), read_only(false) {
	QGridLayout *grid = new QGridLayout(this);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Expression:"), this), 1, 0, 1, 2);
	expressionEdit = new QPlainTextEdit(this);
	grid->addWidget(expressionEdit, 2, 0, 1, 2);
	QHBoxLayout *box = new QHBoxLayout();
	QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
	box->addWidget(new QLabel(tr("Parameter references:"), this), 1); 
	ref1Button = new QRadioButton(tr("x, y, z"), this); group->addButton(ref1Button, 1); box->addWidget(ref1Button);
	ref1Button->setChecked(true);
	ref2Button = new QRadioButton(tr("\\x, \\y, \\z, \\a, \\b, …"), this); group->addButton(ref2Button, 2); box->addWidget(ref2Button);
	grid->addLayout(box, 3, 0, 1, 2);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	grid->addWidget(buttonBox, 4, 0, 1, 2);
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onExpressionChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
}
FunctionEditDialog::~FunctionEditDialog() {}

UserFunction *FunctionEditDialog::createFunction() {
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString())) {
		if(QMessageBox::question(this, tr("Question"), tr("An function with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
	}
	UserFunction *f;
	MathFunction *func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
	if(func && func->isLocal() && func->subtype() == SUBTYPE_USER_FUNCTION) {
		f = (UserFunction*) func;
		if(!modifyFunction(f)) return NULL;
		return f;
	}
	std::string str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
	if(ref1Button->isChecked()) {
		gsub("x", "\\x", str);
		gsub("y", "\\y", str);
		gsub("z", "\\z", str);
	}
	f = new UserFunction("", nameEdit->text().trimmed().toStdString(), str);
	CALCULATOR->addFunction(f);
	return f;
}
bool FunctionEditDialog::modifyFunction(MathFunction *f) {
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString(), f)) {
		if(QMessageBox::question(this, tr("Question"), tr("An function with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
	}
	f->setLocal(true);
	if(f->countNames() > 1) f->clearNames();
	f->setName(nameEdit->text().trimmed().toStdString());
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		std::string str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
		if(ref1Button->isChecked()) {
			gsub("x", "\\x", str);
			gsub("y", "\\y", str);
			gsub("z", "\\z", str);
		}
		((UserFunction*) f)->setFormula(str);
	}
	return true;
}
void FunctionEditDialog::setFunction(MathFunction *f) {
	read_only = !f->isLocal();
	nameEdit->setText(QString::fromStdString(f->getName(1).name));
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		expressionEdit->setEnabled(true);
		expressionEdit->setPlainText(QString::fromStdString(CALCULATOR->localizeExpression(((UserFunction*) f)->formula(), settings->evalops.parse_options)));
	} else {
		read_only = true;
		expressionEdit->setEnabled(false);
		expressionEdit->clear();
	}
	okButton->setEnabled(!read_only);
	nameEdit->setReadOnly(read_only);
	expressionEdit->setReadOnly(read_only);
}
void FunctionEditDialog::onNameEdited(const QString &str) {
	if(!read_only) okButton->setEnabled(!str.trimmed().isEmpty() && (!expressionEdit->isEnabled() || !expressionEdit->document()->isEmpty()));
	if(!str.trimmed().isEmpty() && !CALCULATOR->functionNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidFunctionName(str.trimmed().toStdString())));
	}
}
void FunctionEditDialog::onExpressionChanged() {
	if(!read_only) okButton->setEnabled((!expressionEdit->document()->isEmpty() || !expressionEdit->isEnabled()) && !nameEdit->text().trimmed().isEmpty());
}
void FunctionEditDialog::setExpression(const QString &str) {
	expressionEdit->setPlainText(str);
	onExpressionChanged();
}
void FunctionEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
QString FunctionEditDialog::expression() const {
	return expressionEdit->toPlainText();
}
void FunctionEditDialog::setRefType(int i) {
	if(i == 1) ref1Button->setChecked(true);
	else if(i == 2) ref2Button->setChecked(true);
}
bool FunctionEditDialog::editFunction(QWidget *parent, MathFunction *f) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
	d->setRefType(2);
	d->setWindowTitle(tr("Edit Function"));
	d->setFunction(f);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyFunction(f)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
UserFunction* FunctionEditDialog::newFunction(QWidget *parent) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
	d->setWindowTitle(tr("New Function"));
	d->setRefType(1);
	std::string f_name;
	int i = 1;
	do {
		f_name = "f"; f_name += i2s(i);
		i++;
	} while(CALCULATOR->nameTaken(f_name));
	d->setName(QString::fromStdString(f_name));
	UserFunction *f = NULL;
	while(d->exec() == QDialog::Accepted) {
		f = d->createFunction();
		if(f) break;
	}
	d->deleteLater();
	return f;
}

