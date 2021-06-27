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
#include <QHBoxLayout>
#include <QTableWidget>
#include <QSpinBox>
#include <QHeaderView>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "variableeditdialog.h"

VariableEditDialog::VariableEditDialog(QWidget *parent, bool allow_empty_value, bool edit_matrix) : QDialog(parent), b_empty(allow_empty_value), b_matrix(edit_matrix), b_changed(false) {
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	grid->addWidget(nameEdit, 0, 1);
	temporaryBox = new QCheckBox(tr("Temporary"), this);
	temporaryBox->setChecked(true);
	grid->addWidget(temporaryBox, b_matrix ? 3 : 2, 0, 1, 2, Qt::AlignRight);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	if(b_matrix) {
		QHBoxLayout *hbox = new QHBoxLayout();
		grid->addLayout(hbox, 1, 0, 1, 2);
		hbox->addStretch(1);
		rowsSpin = new QSpinBox(this);
		rowsSpin->setRange(1, 20);
		rowsSpin->setValue(8);
		rowsSpin->setAlignment(Qt::AlignRight);
		hbox->addWidget(rowsSpin);
		connect(rowsSpin, SIGNAL(valueChanged(int)), this, SLOT(matrixRowsChanged(int)));
		hbox->addWidget(new QLabel(SIGN_MULTIPLICATION));
		columnsSpin = new QSpinBox(this);
		columnsSpin->setRange(1, 20);
		columnsSpin->setValue(8);
		columnsSpin->setAlignment(Qt::AlignRight);
		hbox->addWidget(columnsSpin);
		connect(columnsSpin, SIGNAL(valueChanged(int)), this, SLOT(matrixColumnsChanged(int)));
		matrixTable = new QTableWidget(this);
		QFontMetrics fm(matrixTable->font());
		matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		QTableWidgetItem *proto = new QTableWidgetItem();
		proto->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		matrixTable->setItemPrototype(proto);
		matrixTable->setRowCount(8);
		matrixTable->setColumnCount(8);
		matrixTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
		grid->addWidget(matrixTable, 2, 0, 1, 2);
		connect(matrixTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(onMatrixChanged()));
		resize(fm.boundingRect("000000").width() * 12, sizeHint().height());
	} else {
		grid->addWidget(new QLabel(tr("Value:"), this), 1, 0);
		valueEdit = new MathLineEdit(this);
		valueEdit->setAlignment(Qt::AlignRight);
		if(b_empty) valueEdit->setPlaceholderText(tr("current result"));
		grid->addWidget(valueEdit, 1, 1);
		connect(valueEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onValueEdited(const QString&)));
	}
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
VariableEditDialog::~VariableEditDialog() {}

void VariableEditDialog::matrixRowsChanged(int i) {
	matrixTable->setRowCount(i);
	matrixTable->adjustSize();
	b_changed = true;
}
void VariableEditDialog::matrixColumnsChanged(int i) {
	matrixTable->setColumnCount(i);
	matrixTable->adjustSize();
	b_changed = true;
}
void VariableEditDialog::onMatrixChanged() {
	b_changed = true;
	if(!b_empty) okButton->setEnabled(!nameEdit->text().trimmed().isEmpty());
}
bool VariableEditDialog::valueHasChanged() const {
	return b_changed;
}
KnownVariable *VariableEditDialog::createVariable(MathStructure *default_value, ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	Variable *var = NULL;
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString())) {
		if(QMessageBox::question(this, tr("Question"), tr("An unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
			if(!var) *replaced_item = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString());
			else *replaced_item = var;
		}
	}
	KnownVariable *v;
	if(var && var->isLocal() && var->isKnown()) {
		v = (KnownVariable*) var;
		if(v->countNames() > 1) v->clearNames();
		v->setHidden(false); v->setDescription(""); v->setTitle("");
		if(!modifyVariable(v, default_value)) return NULL;
		return v;
	}
	if(default_value && ((!b_matrix && valueEdit->text().isEmpty()) || !b_changed)) {
		v = new KnownVariable("", nameEdit->text().trimmed().toStdString(), *default_value);
	} else {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		QString str;
		if(b_matrix) {
			int max_c = -1, max_r = -1;
			for(int r = 0; r < matrixTable->rowCount(); r++) {
				bool b = false;
				for(int c = 0; c < matrixTable->columnCount(); c++) {
					QTableWidgetItem *item = matrixTable->item(r, c);
					if(item && !item->text().isEmpty()) {
						b = true;
						if(c > max_c) max_c = c;
					}
				}
				if(b) max_r = r;
			}
			if(max_r < 0) {max_r = matrixTable->rowCount() - 1; max_c = matrixTable->columnCount() - 1;}
			str = "[";
			for(int r = 0; r <= max_r; r++) {
				if(r > 0) str += ", ";
				str += "[";
				for(int c = 0; c <= max_c; c++) {
					if(c > 0) str += ", ";
					QTableWidgetItem *item = matrixTable->item(r, c);
					if(!item || item->text().isEmpty()) str += "0";
					else str += item->text();
				}
				str += "]";
			}
			str += "]";
		} else {
			str = valueEdit->text().trimmed();
		}
		v = new KnownVariable("", nameEdit->text().trimmed().toStdString(), CALCULATOR->unlocalizeExpression(str.toStdString(), pa));
	}
	if(temporaryBox->isChecked()) v->setCategory(CALCULATOR->temporaryCategory());
	v->setChanged(false);
	CALCULATOR->addVariable(v);
	return v;
}
bool VariableEditDialog::modifyVariable(KnownVariable *v, MathStructure *default_value, ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(CALCULATOR->variableNameTaken(nameEdit->text().trimmed().toStdString(), v)) {
		if(QMessageBox::question(this, tr("Question"), tr("An unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			Variable *var = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
			if(!var) *replaced_item = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString());
			else if(var != v) *replaced_item = var;
		}
	}
	if(v->countNames() > 1 && v->getName(1).name != nameEdit->text().trimmed().toStdString()) v->clearNames();
	v->setName(nameEdit->text().trimmed().toStdString());
	v->setApproximate(false); v->setUncertainty(""); v->setUnit("");
	if(default_value && ((!b_matrix && valueEdit->text().isEmpty()) || !b_changed)) {
		v->set(*default_value);
	} else {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		QString str;
		if(b_matrix) {
			int max_c = -1, max_r = -1;
			for(int r = 0; r < matrixTable->rowCount(); r++) {
				bool b = false;
				for(int c = 0; c < matrixTable->columnCount(); c++) {
					QTableWidgetItem *item = matrixTable->item(r, c);
					if(item && !item->text().isEmpty()) {
						b = true;
						if(c > max_c) max_c = c;
					}
				}
				if(b) max_r = r;
			}
			if(max_r < 0) {max_r = matrixTable->rowCount() - 1; max_c = matrixTable->columnCount() - 1;}
			str = "[";
			for(int r = 0; r <= max_r; r++) {
				if(r > 0) str += ", ";
				str += "[";
				for(int c = 0; c <= max_c; c++) {
					if(c > 0) str += ", ";
					QTableWidgetItem *item = matrixTable->item(r, c);
					if(!item || item->text().isEmpty()) str += "0";
					else str += item->text();
				}
				str += "]";
			}
			str += "]";
		} else {
			str = valueEdit->text().trimmed();
		}
		v->set(CALCULATOR->unlocalizeExpression(str.toStdString(), pa));
	}
	if(temporaryBox->isChecked()) v->setCategory(CALCULATOR->temporaryCategory());
	else if(v->category() == CALCULATOR->temporaryCategory()) v->setCategory("");
	return true;
}
void VariableEditDialog::setVariable(KnownVariable *v) {
	nameEdit->setText(QString::fromStdString(v->getName(1).name));
	if(v->isExpression() && !b_matrix) {
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		valueEdit->setText(QString::fromStdString(CALCULATOR->localizeExpression(v->expression(), pa)));
	} else {
		PrintOptions po = settings->printops;
		po.is_approximate = NULL;
		po.allow_non_usable = false;
		po.base = 10;
		if(b_matrix) {
			QTableWidgetItem *item;
			if(v->get().isMatrix()) {
				MathStructure m(v->get());
				rowsSpin->setValue(m.rows());
				columnsSpin->setValue(m.columns());
				CALCULATOR->startControl(2000);
				for(size_t i = 0; i < m.rows(); i++) {
					for(size_t i2 = 0; i2 < m.columns(); i2++) {
						m[i][i2].format(po);
						item = new QTableWidgetItem(QString::fromStdString(m[i][i2].print(po)));
						item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
						matrixTable->setItem((int) i, (int) i2, item);
					}
				}
				CALCULATOR->stopControl();
			} else {
				item = new QTableWidgetItem(QString::fromStdString(CALCULATOR->print(v->get(), 1000, po)));
				item->setTextAlignment(Qt::AlignRight);
				matrixTable->setItem(0, 0, item);
			}
		} else {
			valueEdit->setText(QString::fromStdString(CALCULATOR->print(v->get(), 1000, po)));
		}
	}
	nameEdit->setReadOnly(!v->isLocal());
	if(b_matrix) {
		matrixTable->setEditTriggers(v->isLocal() ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
		rowsSpin->setReadOnly(!v->isLocal());
		columnsSpin->setReadOnly(!v->isLocal());
	} else {
		valueEdit->setReadOnly(!v->isLocal());
	}
	temporaryBox->setChecked(v->category() == CALCULATOR->temporaryCategory());
	okButton->setEnabled(v->isLocal());
	b_changed = false;
}
void VariableEditDialog::onNameEdited(const QString &str) {
	okButton->setEnabled(!str.trimmed().isEmpty() && (b_empty || (!b_matrix && !valueEdit->text().trimmed().isEmpty()) || (b_matrix && b_changed)));
	if(!str.trimmed().isEmpty() && !CALCULATOR->variableNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidVariableName(str.trimmed().toStdString())));
	}
}
void VariableEditDialog::onValueEdited(const QString &str) {
	b_changed = true;
	if(!b_empty) okButton->setEnabled(!str.trimmed().isEmpty() && !nameEdit->text().trimmed().isEmpty());
}
void VariableEditDialog::setValue(const QString &str) {
	valueEdit->setText(str);
	if(!b_empty) onValueEdited(str);
}
void VariableEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
QString VariableEditDialog::value() const {
	return valueEdit->text();
}
bool VariableEditDialog::editVariable(QWidget *parent, KnownVariable *v, ExpressionItem **replaced_item) {
	VariableEditDialog *d = new VariableEditDialog(parent, false, v->get().isMatrix());
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
	bool edit_matrix = default_value && default_value->isMatrix();
	VariableEditDialog *d = new VariableEditDialog(parent, default_value != NULL && (value_str.isEmpty() || edit_matrix), edit_matrix);
	d->setWindowTitle(tr("New Variable"));
	if(edit_matrix) {
		KnownVariable v(CALCULATOR->temporaryCategory(), "", *default_value);
		d->setVariable(&v);
	} else {
		d->setValue(value_str);
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
			if(default_value && str == value_str) d->setValue("");
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

