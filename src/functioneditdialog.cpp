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
#include <QKeyEvent>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functioneditdialog.h"

class MathTextEdit : public QPlainTextEdit {

	public:

		MathTextEdit(QWidget *parent) : QPlainTextEdit(parent) {
#ifndef _WIN32
			setAttribute(Qt::WA_InputMethodEnabled, settings->enable_input_method);
#endif
		}
		~MathTextEdit() {}

	protected:

		void keyPressEvent(QKeyEvent *event) override {
			if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::GroupSwitchModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
				switch(event->key()) {
					case Qt::Key_Asterisk: {
						insertPlainText(settings->multiplicationSign());
						return;
					}
					case Qt::Key_Slash: {
						insertPlainText(settings->divisionSign(false));
						return;
					}
					case Qt::Key_Minus: {
						insertPlainText(SIGN_MINUS);
						return;
					}
					case Qt::Key_Dead_Circumflex: {
						insertPlainText(settings->caret_as_xor ? " xor " : "^");
						return;
					}
					case Qt::Key_Dead_Tilde: {
						insertPlainText("~");
						return;
					}
					case Qt::Key_AsciiCircum: {
						if(settings->caret_as_xor) {
							insertPlainText(" xor ");
							return;
						}
						break;
					}
				}
			}
			if(event->key() == Qt::Key_Asterisk && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::KeypadModifier) || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
				insertPlainText("^");
				return;
			}
			QPlainTextEdit::keyPressEvent(event);
		}
};

FunctionEditDialog::FunctionEditDialog(QWidget *parent) : QDialog(parent) {
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	topbox->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Expression:"), this), 1, 0, 1, 2);
	expressionEdit = new MathTextEdit(this);
	grid->addWidget(expressionEdit, 2, 0, 1, 2);
	QHBoxLayout *box = new QHBoxLayout();
	QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
	box->addWidget(new QLabel(tr("Argument references:"), this), 1);
	ref1Button = new QRadioButton(tr("x, y, z"), this); group->addButton(ref1Button, 1); box->addWidget(ref1Button);
	ref1Button->setChecked(true);
	ref2Button = new QRadioButton(tr("\\x, \\y, \\z, \\a, \\b, …"), this); group->addButton(ref2Button, 2); box->addWidget(ref2Button);
	grid->addLayout(box, 3, 0, 1, 2);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	topbox->addWidget(buttonBox);
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onExpressionChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
FunctionEditDialog::~FunctionEditDialog() {}

UserFunction *FunctionEditDialog::createFunction(MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	MathFunction *func = NULL;
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString())) {
		if(QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
			*replaced_item = func;
		}
	}
	UserFunction *f;
	if(func && func->isLocal() && func->subtype() == SUBTYPE_USER_FUNCTION) {
		f = (UserFunction*) func;
		if(f->countNames() > 1) f->clearNames();
		f->setHidden(false); f->setApproximate(false); f->setDescription(""); f->setCondition(""); f->setExample(""); f->clearArgumentDefinitions(); f->setTitle("");
		if(!modifyFunction(f)) return NULL;
		return f;
	}
	std::string str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
	if(ref1Button->isChecked()) {
		gsub("x", "\\x", str);
		gsub("y", "\\y", str);
		gsub("z", "\\z", str);
	}
	gsub(settings->multiplicationSign(), "*", str);
	gsub(settings->divisionSign(), "/", str);
	gsub(SIGN_MINUS, "-", str);
	f = new UserFunction("", nameEdit->text().trimmed().toStdString(), str);
	CALCULATOR->addFunction(f);
	return f;
}
bool FunctionEditDialog::modifyFunction(MathFunction *f, MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString(), f)) {
		if(QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			MathFunction *func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
			if(func != f) *replaced_item = func;
		}
	}
	f->setLocal(true);
	if(f->countNames() > 1 && f->getName(1).name != nameEdit->text().trimmed().toStdString()) f->clearNames();
	f->setName(nameEdit->text().trimmed().toStdString());
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		std::string str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
		if(ref1Button->isChecked()) {
			gsub("x", "\\x", str);
			gsub("y", "\\y", str);
			gsub("z", "\\z", str);
		}
		gsub(settings->multiplicationSign(), "*", str);
		gsub(settings->divisionSign(), "/", str);
		gsub(SIGN_MINUS, "-", str);
		((UserFunction*) f)->setFormula(str);
	}
	return true;
}
void FunctionEditDialog::setFunction(MathFunction *f) {
	bool read_only = !f->isLocal();
	nameEdit->setText(QString::fromStdString(f->getName(1).name));
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		expressionEdit->setEnabled(true);
		std::string str = CALCULATOR->localizeExpression(((UserFunction*) f)->formula(), settings->evalops.parse_options);
		gsub("*", settings->multiplicationSign(), str);
		gsub("/", settings->divisionSign(false), str);
		gsub("-", SIGN_MINUS, str);
		expressionEdit->setPlainText(QString::fromStdString(str));
	} else {
		read_only = true;
		expressionEdit->setEnabled(false);
		expressionEdit->clear();
	}
	okButton->setEnabled(!read_only);
	nameEdit->setReadOnly(read_only);
	ref1Button->setEnabled(!read_only);
	ref2Button->setEnabled(!read_only);
	expressionEdit->setReadOnly(read_only);
}
void FunctionEditDialog::onNameEdited(const QString &str) {
	okButton->setEnabled(!str.trimmed().isEmpty() && (!expressionEdit->isEnabled() || !expressionEdit->document()->isEmpty()));
	if(!str.trimmed().isEmpty() && !CALCULATOR->functionNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidFunctionName(str.trimmed().toStdString())));
	}
}
void FunctionEditDialog::onExpressionChanged() {
	okButton->setEnabled((!expressionEdit->document()->isEmpty() || !expressionEdit->isEnabled()) && !nameEdit->text().trimmed().isEmpty());
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
bool FunctionEditDialog::editFunction(QWidget *parent, MathFunction *f, MathFunction **replaced_item) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
	d->setRefType(2);
	d->setWindowTitle(tr("Edit Function"));
	d->setFunction(f);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyFunction(f, replaced_item)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
UserFunction* FunctionEditDialog::newFunction(QWidget *parent, MathFunction **replaced_item) {
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
		f = d->createFunction(replaced_item);
		if(f) break;
	}
	d->deleteLater();
	return f;
}

