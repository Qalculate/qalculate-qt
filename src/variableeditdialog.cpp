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
#include <QVBoxLayout>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QAction>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "variableeditdialog.h"
#include "functioneditdialog.h"
#include "matrixwidget.h"

VariableEditDialog::VariableEditDialog(QWidget *parent, bool allow_empty_value, bool edit_matrix) : QDialog(parent), b_empty(allow_empty_value), b_matrix(edit_matrix), b_changed(false) {
	o_variable = NULL;
	name_edited = false;
	namesEditDialog = NULL;
	QVBoxLayout *box = new QVBoxLayout(this);
	QTabWidget *tabs = new QTabWidget(this);
	tabs->setUsesScrollButtons(false);
	box->addWidget(tabs);
	QWidget *w1 = new QWidget(this);
	QWidget *w2 = new QWidget(this);
	tabs->addTab(w1, tr("Required"));
	tabs->addTab(w2, tr("Description"));
	QGridLayout *grid = new QGridLayout(w1);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	connect(nameEdit->addAction(LOAD_ICON("configure"), QLineEdit::TrailingPosition), SIGNAL(triggered()), this, SLOT(editNames()));
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
			nameEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	int r = 0;
	grid->addWidget(nameEdit, r, 1); r++;
	if(b_matrix) {
		matrixEdit = new MatrixWidget(this);
		grid->addWidget(matrixEdit, r, 0, 1, 2); r++;
		connect(matrixEdit, SIGNAL(matrixChanged()), this, SLOT(onMatrixChanged()));
		connect(matrixEdit, SIGNAL(dimensionChanged(int, int)), this, SLOT(onMatrixDimensionChanged()));
	} else {
		grid->addWidget(new QLabel(tr("Value:"), this), r, 0); r++;
		valueEdit = new MathTextEdit(this);
		if(b_empty) valueEdit->setPlaceholderText(tr("current result"));
		grid->addWidget(valueEdit, r, 0, 1, 2); r++;
		connect(valueEdit, SIGNAL(textChanged()), this, SLOT(onValueEdited()));
	}
	temporaryBox = new QCheckBox(tr("Temporary"), this);
	temporaryBox->setChecked(true);
	grid->addWidget(temporaryBox, r, 0, 1, 2, Qt::AlignRight); r++;
	grid = new QGridLayout(w2);
	grid->addWidget(new QLabel(tr("Category:"), this), 0, 0);
	categoryEdit = new QComboBox(this);
	categoryEdit->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	int l = unicode_length(CALCULATOR->temporaryCategory());
	if(l < 20) l = 20;
	categoryEdit->setMinimumContentsLength(l);
	std::unordered_map<std::string, bool> hash;
	QVector<Variable*> items;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			if(hash.find(CALCULATOR->variables[i]->category()) == hash.end()) {
				items.push_back(CALCULATOR->variables[i]);
				hash[CALCULATOR->variables[i]->category()] = true;
			}
		}
	}
	for(int i = 0; i < items.count(); i++) {
		categoryEdit->addItem(QString::fromStdString(items[i]->category()));
	}
	categoryEdit->setEditable(true);
	categoryEdit->setCurrentText(QString::fromStdString(CALCULATOR->temporaryCategory()));
	grid->addWidget(categoryEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Descriptive name:"), this), 1, 0);
	titleEdit = new QLineEdit(this);
	grid->addWidget(titleEdit, 1, 1);
	hideBox = new QCheckBox(tr("Hide variable"), this);
	grid->addWidget(hideBox, 2, 1, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("Description:"), this), 3, 0, 1, 2);
	descriptionEdit = new SmallTextEdit(2, this);
	grid->addWidget(descriptionEdit, 4, 0, 1, 2);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	nameEdit->setFocus();
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(temporaryBox, SIGNAL(clicked()), this, SLOT(temporaryClicked()));
	connect(descriptionEdit, SIGNAL(textChanged()), this, SLOT(onVariableChanged()));
	connect(hideBox, SIGNAL(clicked()), this, SLOT(onVariableChanged()));
	connect(titleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onVariableChanged()));
	connect(categoryEdit, SIGNAL(currentTextChanged(const QString&)), this, SLOT(categoryChanged(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
VariableEditDialog::~VariableEditDialog() {}

void VariableEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_variable, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
	onVariableChanged();
}
void VariableEditDialog::categoryChanged(const QString &str) {
	temporaryBox->setChecked(str == QString::fromStdString(CALCULATOR->temporaryCategory()));
	onVariableChanged();
}
void VariableEditDialog::temporaryClicked() {
	categoryEdit->blockSignals(true);
	if(temporaryBox->isChecked()) {
		categoryEdit->setCurrentText(QString::fromStdString(CALCULATOR->temporaryCategory()));
	} else {
		categoryEdit->setCurrentText(QString());
	}
	categoryEdit->blockSignals(false);
	onVariableChanged();
}
void VariableEditDialog::onMatrixDimensionChanged() {
	b_changed = true;
}
void VariableEditDialog::onMatrixChanged() {
	b_changed = true;
	onVariableChanged();
}
bool VariableEditDialog::valueHasChanged() const {
	return b_changed;
}
KnownVariable *VariableEditDialog::createVariable(MathStructure *default_value, ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	Variable *var = NULL;
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString())) {
		var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
		if(name_edited && (!var || var->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			if(!var) *replaced_item = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString());
			else *replaced_item = var;
		}
	}
	KnownVariable *v;
	if(var && var->isLocal() && var->isKnown()) {
		v = (KnownVariable*) var;
		if(v->countNames() > 1) v->clearNames();
		if(!modifyVariable(v, default_value)) return NULL;
		return v;
	}
	if(default_value && ((!b_matrix && valueEdit->toPlainText().isEmpty()) || !b_changed)) {
		v = new KnownVariable("", nameEdit->text().trimmed().toStdString(), *default_value);
	} else {
		std::string str;
		if(b_matrix) {
			str = matrixEdit->getMatrixString().toStdString();
		} else {
			str = settings->unlocalizeExpression(valueEdit->toPlainText().toStdString());
		}
		v = new KnownVariable("", nameEdit->text().trimmed().toStdString(), str);
	}
	if(namesEditDialog) namesEditDialog->modifyNames(v, nameEdit->text());
	v->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	v->setTitle(titleEdit->text().trimmed().toStdString());
	v->setCategory(categoryEdit->currentText().trimmed().toStdString());
	v->setHidden(hideBox->isChecked());
	v->setChanged(false);
	CALCULATOR->addVariable(v);
	return v;
}
bool VariableEditDialog::modifyVariable(KnownVariable *v, MathStructure *default_value, ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString(), v)) {
		Variable *var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
		if(name_edited && (!var || var->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			if(!var) *replaced_item = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString());
			else if(var != v) *replaced_item = var;
		}
	}
	if(namesEditDialog) {
		namesEditDialog->modifyNames(v, nameEdit->text());
	} else {
		if(v->countNames() > 1 && v->getName(1).name != nameEdit->text().trimmed().toStdString()) v->clearNames();
		v->setName(nameEdit->text().trimmed().toStdString());
	}
	v->setApproximate(false); v->setUncertainty(""); v->setUnit("");
	if(default_value && ((!b_matrix && valueEdit->toPlainText().isEmpty()) || !b_changed)) {
		v->set(*default_value);
	} else {
		std::string str;
		if(b_matrix) {
			str = matrixEdit->getMatrixString().toStdString();
		} else {
			str = settings->unlocalizeExpression(valueEdit->toPlainText().toStdString());
		}
		v->set(str);
	}
	v->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	v->setTitle(titleEdit->text().trimmed().toStdString());
	v->setCategory(categoryEdit->currentText().trimmed().toStdString());
	v->setHidden(hideBox->isChecked());
	return true;
}
void VariableEditDialog::setVariable(KnownVariable *v) {
	nameEdit->setText(QString::fromStdString(v->getName(1).name));
	if(!nameEdit->text().isEmpty()) o_variable = v;
	if(v->isExpression() && !b_matrix) {
		std::string value_str = settings->localizeExpression(v->expression());
		bool is_relative = false;
		if((!v->uncertainty(&is_relative).empty() || !v->unit().empty()) && !is_relative && v->expression().find_first_not_of(NUMBER_ELEMENTS) != std::string::npos) {
			value_str.insert(0, 1, '(');
			value_str += ')';
		}
		if(!v->uncertainty(&is_relative).empty()) {
			if(is_relative) {
				value_str.insert(0, "(");
				value_str.insert(0, CALCULATOR->f_uncertainty->referenceName());
				value_str += CALCULATOR->getComma();
				value_str += " ";
				value_str += settings->localizeExpression(v->uncertainty());
				value_str += CALCULATOR->getComma();
				value_str += " 1)";
			} else {
				value_str += SIGN_PLUSMINUS;
				value_str += settings->localizeExpression(v->uncertainty());
			}
		}
		if(!v->unit().empty() && v->unit() != "auto") {
			value_str += " ";
			value_str += settings->localizeExpression(v->unit(), true);
		}
		valueEdit->setPlainText(QString::fromStdString(value_str));
	} else if(b_matrix) {
		matrixEdit->setMatrix(v->get());
	} else {
		PrintOptions po = settings->printops;
		po.is_approximate = NULL;
		po.allow_non_usable = false;
		po.preserve_precision = true;
		po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
		if(po.number_fraction_format == FRACTION_DECIMAL) po.number_fraction_format = FRACTION_DECIMAL_EXACT;
		po.base = 10;
		valueEdit->setPlainText(QString::fromStdString(CALCULATOR->print(v->get(), 1000, po)));
	}
	nameEdit->setReadOnly(!v->isLocal());
	if(b_matrix) {
		matrixEdit->setEditable(v->isLocal());
	} else {
		valueEdit->setReadOnly(!v->isLocal());
	}
	temporaryBox->setChecked(v->category() == CALCULATOR->temporaryCategory());
	descriptionEdit->blockSignals(true);
	descriptionEdit->setPlainText(QString::fromStdString(v->description()));
	descriptionEdit->blockSignals(false);
	titleEdit->setText(QString::fromStdString(v->title()));
	categoryEdit->blockSignals(true);
	categoryEdit->setCurrentText(QString::fromStdString(v->category()));
	categoryEdit->blockSignals(false);
	descriptionEdit->setReadOnly(!v->isLocal());
	titleEdit->setReadOnly(!v->isLocal());
	hideBox->setChecked(v->isHidden());
	okButton->setEnabled(false);
	b_changed = false;
}
void VariableEditDialog::onNameEdited(const QString &str) {
	if(!str.trimmed().isEmpty() && !CALCULATOR->variableNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidVariableName(str.trimmed().toStdString())));
	}
	name_edited = true;
	onVariableChanged();
}
void VariableEditDialog::onVariableChanged() {
	okButton->setEnabled(!nameEdit->isReadOnly() && !nameEdit->text().trimmed().isEmpty() && (b_empty || (!b_matrix && !valueEdit->toPlainText().trimmed().isEmpty()) || (b_matrix && b_changed)));
}
void VariableEditDialog::onValueEdited() {
	b_changed = true;
	onVariableChanged();
}
void VariableEditDialog::setValue(const QString &str) {
	valueEdit->setPlainText(str);
	if(!b_empty) onValueEdited();
}
void VariableEditDialog::disableValue() {
	valueEdit->setReadOnly(true);
}
void VariableEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
QString VariableEditDialog::value() const {
	return valueEdit->toPlainText();
}
bool VariableEditDialog::editVariable(QWidget *parent, KnownVariable *v, ExpressionItem **replaced_item) {
	VariableEditDialog *d = new VariableEditDialog(parent, false, v->get().isMatrix() && v->get().rows() * v->get().columns() <= 10000);
	d->setWindowTitle(tr("Edit Variable"));
	d->setVariable(v);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyVariable(v, NULL, replaced_item)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
KnownVariable* VariableEditDialog::newVariable(QWidget *parent, MathStructure *default_value, const QString &value_str, ExpressionItem **replaced_item) {
	bool edit_matrix = default_value && default_value->isMatrix() && default_value->rows() * default_value->columns() <= 10000;
	VariableEditDialog *d = new VariableEditDialog(parent, default_value != NULL && (value_str.isEmpty() || value_str.length() > 1000 || edit_matrix), edit_matrix);
	d->setWindowTitle(tr("New Variable"));
	QString vstr = value_str;
	if(edit_matrix) {
		KnownVariable v(CALCULATOR->temporaryCategory(), "", *default_value);
		d->setVariable(&v);
	} else if(vstr.length() > 1000) {
		d->setValue(QString());
	} else {
		if(vstr.isEmpty() && default_value) {
			PrintOptions po = settings->printops;
			po.is_approximate = NULL;
			po.allow_non_usable = false;
			po.preserve_precision = true;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			po.show_ending_zeroes = false;
			if(po.number_fraction_format == FRACTION_DECIMAL) po.number_fraction_format = FRACTION_DECIMAL_EXACT;
			po.base = 10;
			vstr = QString::fromStdString(CALCULATOR->print(*default_value, 1000, po));
		}
		d->setValue(vstr);
	}
	std::string v_name;
	int i = 1;
	do {
		v_name = "v"; v_name += i2s(i);
		i++;
	} while(CALCULATOR->nameTaken(v_name));
	d->setName(QString::fromStdString(v_name));
	KnownVariable *v = NULL;
	while(d->exec() == QDialog::Accepted) {
		if(edit_matrix) {
			v = d->createVariable(default_value, replaced_item);
			if(v) break;
		} else {
			QString str = d->value().trimmed();
			if(default_value && str == vstr) d->setValue("");
			v = d->createVariable(default_value, replaced_item);
			if(v) break;
			d->setValue(str);
		}
	}
	d->deleteLater();
	return v;
}
KnownVariable* VariableEditDialog::newMatrix(QWidget *parent, ExpressionItem **replaced_item) {
	VariableEditDialog *d = new VariableEditDialog(parent, false, true);
	d->setWindowTitle(tr("New Variable"));
	std::string v_name;
	int i = 1;
	do {
		v_name = "v"; v_name += i2s(i);
		i++;
	} while(CALCULATOR->nameTaken(v_name));
	d->setName(QString::fromStdString(v_name));
	KnownVariable *v = NULL;
	while(d->exec() == QDialog::Accepted) {
		v = d->createVariable(NULL, replaced_item);
		if(v) break;
	}
	d->deleteLater();
	return v;
}

