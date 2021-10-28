/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "csvdialog.h"

CSVDialog::CSVDialog(bool do_import, QWidget *parent, MathStructure *current_result, KnownVariable *var) : QDialog(parent), b_import(do_import), o_variable(var), m_result(current_result) {
	setWindowTitle(do_import ? tr("Import CSV File") : tr("Export CSV File"));
	if(m_result && !m_result->isVector()) m_result = NULL;
	QVBoxLayout *box = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	int r = 0;
	if(!b_import) {
		QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
		resultButton = new QRadioButton(tr("Current result"), this); group->addButton(resultButton, 0); resultButton->setChecked(m_result && !var);
		if(!m_result) resultButton->setEnabled(false);
		grid->addWidget(resultButton, r, 0); r++;
		variableButton = new QRadioButton(tr("Matrix/vector variable:"), this); group->addButton(variableButton, 1); variableButton->setChecked(!m_result || var);
		grid->addWidget(variableButton, r, 0);
		connect(group, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(exportTypeToggled(QAbstractButton*, bool)));
		nameEdit = new QLineEdit(this);
		if(var) nameEdit->setText(QString::fromStdString(var->name()));
		grid->addWidget(nameEdit, r, 1); r++;
		if(var) {
			nameEdit->hide();
			variableButton->hide();
			resultButton->hide();
		}
	}
	grid->addWidget(new QLabel(tr("File:"), this), r, 0);
	fileEdit = new QLineEdit(this);
	QAction *action = fileEdit->addAction(LOAD_ICON("document-open"), QLineEdit::TrailingPosition);
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
	fileEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	connect(action, SIGNAL(triggered()), this, SLOT(onSelectFile()));
	connect(fileEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onFileEdited()));
	grid->addWidget(fileEdit, r, 1); r++;
	if(b_import) {
		grid->addWidget(new QLabel(tr("Import as"), this), r, 0);
		QHBoxLayout *hbox = new QHBoxLayout();
		grid->addLayout(hbox, r, 1); r++;
		QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
		matrixButton = new QRadioButton(tr("matrix"), this); group->addButton(matrixButton); matrixButton->setChecked(true);
		hbox->addWidget(matrixButton);
		vectorButton = new QRadioButton(tr("vectors"), this); group->addButton(vectorButton);
		hbox->addWidget(vectorButton);
		hbox->addStretch(1);
		grid->addWidget(new QLabel(tr("Name:"), this), r, 0);
		nameEdit = new QLineEdit(this);
		grid->addWidget(nameEdit, r, 1); r++;
		grid->addWidget(new QLabel(tr("First row:"), this), r, 0);
		rowSpin = new QSpinBox(this);
		rowSpin->setRange(1, INT_MAX); rowSpin->setValue(1);
		grid->addWidget(rowSpin, r, 1); r++;
		headingsBox = new QCheckBox(tr("Includes headings"), this); headingsBox->setChecked(true);
		grid->addWidget(headingsBox, r, 1); r++;
	}
	grid->addWidget(new QLabel(tr("Delimiter:"), this), r, 0);
	delimiterCombo = new QComboBox(this);
	delimiterCombo->addItem(tr("Comma"), ",");
	delimiterCombo->addItem(tr("Tabulator"), "\t");
	delimiterCombo->addItem(tr("Semicolon"), ";");
	delimiterCombo->addItem(tr("Space"), " ");
	delimiterCombo->addItem(tr("Other"), QString());
	grid->addWidget(delimiterCombo, r, 1); r++;
	connect(delimiterCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onDelimiterChanged(int)));
	delimiterEdit = new QLineEdit(this);
	delimiterEdit->setEnabled(false);
	grid->addWidget(delimiterEdit, r, 1); r++;
	connect(delimiterEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onDelimiterEdited()));
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	box->addWidget(buttonBox);
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	fileEdit->setFocus();
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
CSVDialog::~CSVDialog() {}

void CSVDialog::onNameEdited(const QString &str) {
	if(!str.trimmed().isEmpty() && !CALCULATOR->variableNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidVariableName(str.trimmed().toStdString())));
	}
	enableDisableOk();
}
void CSVDialog::enableDisableOk() {
	okButton->setEnabled(((!b_import && !variableButton->isChecked()) || !nameEdit->text().trimmed().isEmpty()) && !fileEdit->text().trimmed().isEmpty() && (!delimiterCombo->currentData().toString().isEmpty() || !delimiterEdit->text().isEmpty()));
}
void CSVDialog::onSelectFile() {
	QString str;
	if(b_import) str = QFileDialog::getOpenFileName(this, QString(), fileEdit->text());
	else str = QFileDialog::getSaveFileName(this, QString(), fileEdit->text());
	if(!str.isEmpty()) {
		fileEdit->setText(str);
		enableDisableOk();
	}
}
void CSVDialog::exportTypeToggled(QAbstractButton *w, bool b) {
	if(!b) return;
	nameEdit->setEnabled(w == variableButton);
	enableDisableOk();
}
void CSVDialog::onDelimiterEdited() {
	enableDisableOk();
}
void CSVDialog::onFileEdited() {
	enableDisableOk();
}
void CSVDialog::onDelimiterChanged(int i) {
	delimiterEdit->setEnabled(delimiterCombo->itemData(i).toString().isEmpty());
	enableDisableOk();
}
bool CSVDialog::importExport() {
	std::string namestr = nameEdit->text().trimmed().toStdString();
	if(b_import) {
		if(CALCULATOR->variableNameTaken(namestr)) {
			if(QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
				nameEdit->setFocus();
				return false;
			}
		}
		if(!CALCULATOR->importCSV(fileEdit->text().trimmed().toLocal8Bit().data(), rowSpin->value(), headingsBox->isChecked(), delimiterCombo->currentData().toString().isEmpty() ? delimiterEdit->text().toStdString() : delimiterCombo->currentData().toString().toStdString(), matrixButton->isChecked(), namestr)) {
			QMessageBox::critical(this, tr("Error"), tr("Could not import from file \n%1").arg(fileEdit->text()), QMessageBox::Ok);
			settings->displayMessages(this);
			fileEdit->setFocus();
			return false;
		}
	} else {
		const MathStructure *m;
		if(o_variable) {
			m = &o_variable->get();
		} else if(resultButton->isChecked() && m_result) {
			m = m_result;
		} else {
			Variable *var = CALCULATOR->getActiveVariable(namestr);
			if(!var || !var->isKnown() || !((KnownVariable*) var)->get().isVector()) {
				QMessageBox::critical(this, tr("Error"), tr("No matrix or vector variable with the entered name was found."), QMessageBox::Ok);
				nameEdit->setFocus();
				return false;
			}
			m = &((KnownVariable*) var)->get();
		}
		CALCULATOR->startControl(600000);
		if(!CALCULATOR->exportCSV(*m, fileEdit->text().trimmed().toLocal8Bit().data(), delimiterCombo->currentData().toString().isEmpty() ? delimiterEdit->text().toStdString() : delimiterCombo->currentData().toString().toStdString())) {
			QMessageBox::critical(this, tr("Error"), tr("Could not export to file \n%1").arg(fileEdit->text()), QMessageBox::Ok);
			settings->displayMessages(this);
			fileEdit->setFocus();
			CALCULATOR->stopControl();
			return false;
		}
		CALCULATOR->stopControl();
	}
	settings->displayMessages(this);
	return true;
}
bool CSVDialog::importCSVFile(QWidget *parent) {
	CSVDialog *d = new CSVDialog(true, parent);
	bool b = false;
	while(d->exec() == QDialog::Accepted) {
		if(d->importExport()) {b = true; break;}
	}
	d->deleteLater();
	return b;
}
bool CSVDialog::exportCSVFile(QWidget *parent, MathStructure *current_result, KnownVariable *var) {
	CSVDialog *d = new CSVDialog(false, parent, current_result, var);
	bool b = false;
	while(d->exec() == QDialog::Accepted) {
		if(d->importExport()) {b = true; break;}
	}
	d->deleteLater();
	return b;
}

