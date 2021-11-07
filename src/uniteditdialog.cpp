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
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QAction>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "uniteditdialog.h"
#include "functioneditdialog.h"

UnitEditDialog::UnitEditDialog(QWidget *parent) : QDialog(parent) {
	o_unit = NULL;
	name_edited = false;
	namesEditDialog = NULL;
	QVBoxLayout *box = new QVBoxLayout(this);
	tabs = new QTabWidget(this);
	tabs->setUsesScrollButtons(false);
	box->addWidget(tabs);
	QWidget *w1 = new QWidget(this);
	QWidget *w2 = new QWidget(this);
	tabs->addTab(w1, tr("General"));
	tabs->addTab(w2, tr("Relation"));
	QGridLayout *grid = new QGridLayout(w1);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	namesAction = nameEdit->addAction(LOAD_ICON("configure"), QLineEdit::TrailingPosition);
	connect(namesAction, SIGNAL(triggered()), this, SLOT(editNames()));
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
			nameEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Category:"), this), 1, 0);
	categoryEdit = new QComboBox(this);
	std::unordered_map<std::string, bool> hash;
	QVector<Unit*> items;
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(!CALCULATOR->units[i]->category().empty()) {
			if(hash.find(CALCULATOR->units[i]->category()) == hash.end()) {
				items.push_back(CALCULATOR->units[i]);
				hash[CALCULATOR->units[i]->category()] = true;
			}
		}
	}
	for(int i = 0; i < items.count(); i++) {
		categoryEdit->addItem(QString::fromStdString(items[i]->category()));
	}
	categoryEdit->setEditable(true);
	categoryEdit->setCurrentText(QString());
	grid->addWidget(categoryEdit, 1, 1);
	grid->addWidget(new QLabel(tr("Descriptive name:"), this), 2, 0);
	titleEdit = new QLineEdit(this);
	grid->addWidget(titleEdit, 2, 1);
	grid->addWidget(new QLabel(tr("System:"), this), 3, 0);
	systemEdit = new QComboBox(this);
	systemEdit->addItem("SI");
	systemEdit->addItem("CGS");
	systemEdit->addItem(tr("Imperial"));
	systemEdit->addItem(tr("US Survey"));
	systemEdit->setEditable(true);
	systemEdit->setCurrentText(QString());
	grid->addWidget(systemEdit, 3, 1);
	hideBox = new QCheckBox(tr("Hide unit"), this);
	grid->addWidget(hideBox, 4, 1, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("Description:"), this), 5, 0, 1, 2);
	descriptionEdit = new SmallTextEdit(2, this);
	grid->addWidget(descriptionEdit, 6, 0, 1, 2);
	grid = new QGridLayout(w2);
	grid->addWidget(new QLabel(tr("Class:"), this), 0, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setToolTip("<div>" + tr("The class that this unit belongs to. Named derived units are defined in relation to a single other unit, with an optional exponent, while (unnamed) derived units are defined by a unit expression with one or multiple units.") + "</div>");
	typeCombo->setEditable(false);
	typeCombo->addItem(tr("Base unit"));
	typeCombo->addItem(tr("Named derived unit"));
	typeCombo->addItem(tr("Derived unit"));
	typeCombo->setCurrentIndex(0);
	grid->addWidget(typeCombo, 0, 1);
	baseLabel = new QLabel(tr("Base unit(s):"), this);
	grid->addWidget(baseLabel, 1, 0);
	baseEdit = new MathLineEdit(this, true);
	baseEdit->setToolTip("<div>" + tr("Unit (for named derived unit) or unit expression (for unnamed derived unit) that this unit is defined in relation to") + "</div>");
	grid->addWidget(baseEdit, 1, 1);
	exponentLabel = new QLabel(tr("Exponent:"), this);
	grid->addWidget(exponentLabel, 2, 0);
	exponentEdit = new QSpinBox(this);
	exponentEdit->setRange(-9, 9);
	exponentEdit->setValue(1);
	grid->addWidget(exponentEdit, 2, 1);
	relationLabel = new QLabel(tr("Relation:"), this);
	grid->addWidget(relationLabel, 3, 0);
	relationEdit = new MathLineEdit(this);
	relationEdit->setToolTip("<div>" + tr("Relation to the base unit. For linear relations this should just be a number.<br><br>For non-linear relations use \\x for the factor and \\y for the exponent (e.g. \"\\x + 273.15\" for the relation between degrees Celsius and Kelvin).") + "<div>");
	grid->addWidget(relationEdit, 3, 1);
	inverseLabel = new QLabel(tr("Inverse relation:"), this);
	grid->addWidget(inverseLabel, 4, 0);
	inverseEdit = new MathLineEdit(this);
	inverseEdit->setToolTip(tr("Specify for non-linear relation, for conversion back to the base unit."));
	grid->addWidget(inverseEdit, 4, 1);
	mixBox = new QCheckBox(tr("Mix with base unit"), this);
	grid->addWidget(mixBox, 5, 0, 1, 2, Qt::AlignLeft);
	priorityLabel = new QLabel(tr("Priority:"), this);
	grid->addWidget(priorityLabel, 6, 0);
	priorityEdit = new QSpinBox(this);
	priorityEdit->setRange(1, 100);
	priorityEdit->setValue(1);
	grid->addWidget(priorityEdit, 6, 1);
	mbunLabel = new QLabel(tr("Minimum base unit number:"), this);
	grid->addWidget(mbunLabel, 7, 0);
	mbunEdit = new QSpinBox(this);
	mbunEdit->setRange(1, 100);
	mbunEdit->setValue(1);
	grid->addWidget(mbunEdit, 7, 1);
	prefixBox = new QCheckBox(tr("Use with prefixes by default"), this);
	grid->addWidget(prefixBox, 8, 0, 1, 2, Qt::AlignLeft);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	box->addWidget(buttonBox);
	typeChanged(0);
	nameEdit->setFocus();
	connect(priorityEdit, SIGNAL(valueChanged(int)), this, SLOT(onUnitChanged()));
	connect(exponentEdit, SIGNAL(valueChanged(int)), this, SLOT(exponentChanged(int)));
	connect(mbunEdit, SIGNAL(valueChanged(int)), this, SLOT(onUnitChanged()));
	connect(prefixBox, SIGNAL(toggled(bool)), this, SLOT(onUnitChanged()));
	connect(mixBox, SIGNAL(toggled(bool)), this, SLOT(onUnitChanged()));
	connect(mixBox, SIGNAL(toggled(bool)), priorityEdit, SLOT(setEnabled(bool)));
	connect(mixBox, SIGNAL(toggled(bool)), mbunEdit, SLOT(setEnabled(bool)));
	connect(mixBox, SIGNAL(toggled(bool)), priorityLabel, SLOT(setEnabled(bool)));
	connect(mixBox, SIGNAL(toggled(bool)), mbunLabel, SLOT(setEnabled(bool)));
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(onUnitChanged()));
	connect(descriptionEdit, SIGNAL(textChanged()), this, SLOT(onUnitChanged()));
	connect(hideBox, SIGNAL(clicked()), this, SLOT(onUnitChanged()));
	connect(titleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onUnitChanged()));
	connect(relationEdit, SIGNAL(textEdited(const QString&)), this, SLOT(relationChanged(const QString&)));
	connect(inverseEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onUnitChanged()));
	connect(baseEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onUnitChanged()));
	connect(categoryEdit, SIGNAL(currentTextChanged(const QString&)), this, SLOT(onUnitChanged()));
	connect(systemEdit, SIGNAL(currentTextChanged(const QString&)), this, SLOT(systemChanged(const QString&)));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
UnitEditDialog::~UnitEditDialog() {}

void UnitEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(TYPE_UNIT, this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_unit, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
	onUnitChanged();
}
Unit *UnitEditDialog::createUnit(ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	Unit *unit = NULL;
	Unit *bu = NULL;
	if(typeCombo->currentIndex() == 1) {
		bu = CALCULATOR->getUnit(baseEdit->text().trimmed().toStdString());
		if(!bu) bu = CALCULATOR->getCompositeUnit(baseEdit->text().trimmed().toStdString());
		if(!bu) {
			tabs->setCurrentIndex(1);
			baseEdit->setFocus();
			QMessageBox::critical(this, tr("Error"), tr("Base unit does not exist."), QMessageBox::Ok);
			return NULL;
		}
	}
	if(CALCULATOR->unitNameTaken(nameEdit->text().trimmed().toStdString())) {
		unit = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString());
		if(name_edited && (!unit || unit->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			if(!unit) *replaced_item = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
			else *replaced_item = unit;
		}
	}
	Unit *u = NULL;
	if(unit && unit->isLocal()) {
		unit->clearNames();
		return modifyUnit(unit);
	}
	if(typeCombo->currentIndex() == 1) {
		u = new AliasUnit("", "", "", "", "", bu, settings->unlocalizeExpression(relationEdit->text().trimmed().toStdString()), exponentEdit->value(), settings->unlocalizeExpression(inverseEdit->text().trimmed().toStdString()), true);
		if(mixBox->isChecked()) {
			((AliasUnit*) u)->setMixWithBase(priorityEdit->value());
			((AliasUnit*) u)->setMixWithBaseMinimum(mbunEdit->value());
		}
	} else if(typeCombo->currentIndex() == 2) {
		u = new CompositeUnit("", "", "", settings->unlocalizeExpression(baseEdit->text().trimmed().toStdString()), true);
	} else {
		u = new Unit();
	}
	if(namesEditDialog && typeCombo->currentIndex() != 2) {
		namesEditDialog->modifyNames(u, nameEdit->text());
	} else {
		NamesEditDialog::modifyName(u, nameEdit->text());
	}
	u->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	u->setTitle(titleEdit->text().trimmed().toStdString());
	u->setCategory(categoryEdit->currentText().trimmed().toStdString());
	u->setSystem(systemEdit->currentText().trimmed().toStdString());
	u->setHidden(hideBox->isChecked());
	if(u->subtype() != SUBTYPE_COMPOSITE_UNIT) {
		u->setUseWithPrefixesByDefault(prefixBox->isChecked());
	}
	u->setChanged(false);
	CALCULATOR->addUnit(u);
	return u;
}
Unit *UnitEditDialog::modifyUnit(Unit *u, ExpressionItem **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	Unit *bu = NULL;
	if(typeCombo->currentIndex() == 1) {
		bu = CALCULATOR->getUnit(baseEdit->text().trimmed().toStdString());
		if(!bu) bu = CALCULATOR->getCompositeUnit(baseEdit->text().trimmed().toStdString());
		if(!bu || bu == u) {
			tabs->setCurrentIndex(1);
			baseEdit->setFocus();
			QMessageBox::critical(this, tr("Error"), tr("Base unit does not exist."), QMessageBox::Ok);
			return NULL;
		}
	}
	if(CALCULATOR->unitNameTaken(nameEdit->text().trimmed().toStdString(), u)) {
		Unit *unit = CALCULATOR->getActiveUnit(nameEdit->text().trimmed().toStdString());
		if(name_edited && (!unit || unit->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A unit or variable with the same name already exists.\nDo you want to overwrite it?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			if(!unit) *replaced_item = CALCULATOR->getActiveVariable(nameEdit->text().trimmed().toStdString());
			else if(unit != u) *replaced_item = unit;
		}
	}
	Unit *old_u = u;
	switch(typeCombo->currentIndex()) {
		case 1: {
			AliasUnit *au;
			if(u->subtype() != SUBTYPE_ALIAS_UNIT) {
				au = new AliasUnit("", "", "", "", "", bu, settings->unlocalizeExpression(relationEdit->text().trimmed().toStdString()), exponentEdit->value(), settings->unlocalizeExpression(inverseEdit->text().trimmed().toStdString()), true);
				u = au;
			} else {
				au = (AliasUnit*) u;
				au->setBaseUnit(bu);
				au->setExpression(settings->unlocalizeExpression(relationEdit->text().trimmed().toStdString()));
				au->setInverseExpression(au->hasNonlinearExpression() ? settings->unlocalizeExpression(inverseEdit->text().trimmed().toStdString()) : "");
				au->setExponent(exponentEdit->value());
			}
			if(mixBox->isChecked()) {
				((AliasUnit*) u)->setMixWithBase(priorityEdit->value());
				((AliasUnit*) u)->setMixWithBaseMinimum(mbunEdit->value());
			} else {
				((AliasUnit*) u)->setMixWithBase(0);
			}
			break;
		}
		case 2: {
			if(u->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				u = new CompositeUnit("", "", "", settings->unlocalizeExpression(baseEdit->text().trimmed().toStdString()), true);
			} else {
				((CompositeUnit*) u)->setBaseExpression(settings->unlocalizeExpression(baseEdit->text().trimmed().toStdString()));
			}
			break;
		}
		default: {
			if(u->subtype() != SUBTYPE_BASE_UNIT) {
				u = new Unit();
			}
			break;
		}
	}
	if(typeCombo->currentIndex() != 2) {
		u->clearNames();
		NamesEditDialog::modifyName(u, nameEdit->text());
	} else if(namesEditDialog) {
		namesEditDialog->modifyNames(u, nameEdit->text());
	} else {
		if(old_u != u) u->set(old_u);
		NamesEditDialog::modifyName(u, nameEdit->text());
	}
	u->setApproximate(false);
	u->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	u->setTitle(titleEdit->text().trimmed().toStdString());
	u->setCategory(categoryEdit->currentText().trimmed().toStdString());
	u->setSystem(systemEdit->currentText().trimmed().toStdString());
	u->setHidden(hideBox->isChecked());
	if(u->subtype() != SUBTYPE_COMPOSITE_UNIT) {
		u->setUseWithPrefixesByDefault(prefixBox->isChecked());
	}
	if(u != old_u) {
		old_u->destroy();
		u->setChanged(false);
		CALCULATOR->addUnit(u);
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i]->subtype() == SUBTYPE_ALIAS_UNIT && ((AliasUnit*) CALCULATOR->units[i])->firstBaseUnit() == old_u) {
				((AliasUnit*) CALCULATOR->units[i])->setBaseUnit(u);
			} else if(CALCULATOR->units[i]->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				size_t i2 = ((CompositeUnit*) CALCULATOR->units[i])->find(old_u);
				if(i2 > 0) {
					int exp = 1; Prefix *p = NULL;
					((CompositeUnit*) CALCULATOR->units[i])->get(i2, &exp, &p);
					((CompositeUnit*) CALCULATOR->units[i])->del(i2);
					((CompositeUnit*) CALCULATOR->units[i])->add(u, exp, p);
				}
			}
		}
	}
	return u;
}
void UnitEditDialog::setUnit(Unit *u) {
	nameEdit->setText(QString::fromStdString(u->getName(1).name));
	o_unit = u;
	switch(u->subtype()) {
		case SUBTYPE_BASE_UNIT: {
			typeCombo->setCurrentIndex(0);
			typeChanged(0);
			break;
		}
		case SUBTYPE_ALIAS_UNIT: {
			AliasUnit *au = (AliasUnit*) u;
			mixBox->setChecked(au->mixWithBase() > 0);
			baseEdit->setText(QString::fromStdString(au->firstBaseUnit()->preferredDisplayName(settings->printops.abbreviate_names, true, false, false, &can_display_unicode_string_function, (void*) baseEdit).name));
			exponentEdit->setValue(au->firstBaseExponent());
			mbunEdit->setValue(au->mixWithBaseMinimum() > 1 ? au->mixWithBaseMinimum() : 1);
			priorityEdit->setValue(au->mixWithBase());
			typeCombo->setCurrentIndex(1);
			bool is_relative = false;
			if(au->uncertainty(&is_relative).empty()) {
				relationEdit->setText(QString::fromStdString(settings->localizeExpression(au->expression())));
			} else if(is_relative) {
				std::string value = CALCULATOR->getFunctionById(FUNCTION_ID_UNCERTAINTY)->referenceName();
				value += "(";
				value += settings->localizeExpression(au->expression());
				value += CALCULATOR->getComma();
				value += " ";
				value += settings->localizeExpression(au->uncertainty());
				value += CALCULATOR->getComma();
				value += " 1)";
				relationEdit->setText(QString::fromStdString(value));
			} else {
				relationEdit->setText(QString::fromStdString(settings->localizeExpression(au->expression() + SIGN_PLUSMINUS + au->uncertainty())));
			}
			inverseEdit->setText(QString::fromStdString(settings->localizeExpression(au->inverseExpression())));
			break;
		}
		case SUBTYPE_COMPOSITE_UNIT: {
			typeCombo->setCurrentIndex(2);
			baseEdit->setText(QString::fromStdString(((CompositeUnit*) u)->print(false, settings->printops.abbreviate_names, true, &can_display_unicode_string_function, (void*) baseEdit)));
			break;
		}
	}
	prefixBox->setChecked(u->useWithPrefixesByDefault());
	exponentEdit->setReadOnly(!u->isLocal());
	priorityEdit->setReadOnly(!u->isLocal());
	mbunEdit->setReadOnly(!u->isLocal());
	nameEdit->setReadOnly(!u->isLocal());
	baseEdit->setReadOnly(!u->isLocal());
	relationEdit->setReadOnly(!u->isLocal());
	inverseEdit->setReadOnly(!u->isLocal());
	descriptionEdit->blockSignals(true);
	descriptionEdit->setPlainText(QString::fromStdString(u->description()));
	descriptionEdit->blockSignals(false);
	titleEdit->setText(QString::fromStdString(u->title()));
	categoryEdit->setCurrentText(QString::fromStdString(u->category()));
	systemEdit->blockSignals(true);
	systemEdit->setCurrentText(QString::fromStdString(u->system()));
	systemEdit->blockSignals(false);
	descriptionEdit->setReadOnly(!u->isLocal());
	titleEdit->setReadOnly(!u->isLocal());
	hideBox->setChecked(u->isHidden());
	okButton->setEnabled(false);
}
void UnitEditDialog::onNameEdited(const QString &str) {
	if(!str.trimmed().isEmpty() && !CALCULATOR->unitNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidUnitName(str.trimmed().toStdString())));
	}
	name_edited = true;
	onUnitChanged();

}
void UnitEditDialog::relationChanged(const QString &str) {
	inverseEdit->setEnabled(str.indexOf("\\x") >= 0);
	if(!inverseEdit->isEnabled()) inverseEdit->clear();
	inverseLabel->setEnabled(inverseEdit->isEnabled());
	onUnitChanged();
}
void UnitEditDialog::systemChanged(const QString &str) {
	if(str == "SI" || str == "CGS") {
		prefixBox->setChecked(true);
	}
}
void UnitEditDialog::typeChanged(int i) {
	baseEdit->setEnabled(i > 0);
	exponentEdit->setEnabled(i == 1);
	relationEdit->setEnabled(i == 1);
	inverseEdit->setEnabled(i == 1 && relationEdit->text().indexOf("\\x") >= 0);
	mixBox->setEnabled(i == 1 && exponentEdit->value() == 1);
	if(i != 1 || exponentEdit->value() != 1) mixBox->setChecked(false);
	if(i == 2) prefixBox->setChecked(false);
	if(i != 1) {
		exponentEdit->setValue(1);
		relationEdit->setText("1");
	}
	if(i == 0) baseEdit->clear();
	if(!inverseEdit->isEnabled()) inverseEdit->clear();
	priorityEdit->setEnabled(mixBox->isChecked());
	mbunEdit->setEnabled(mixBox->isChecked());
	prefixBox->setEnabled(i != 2);
	baseLabel->setEnabled(baseEdit->isEnabled());
	exponentLabel->setEnabled(exponentEdit->isEnabled());
	relationLabel->setEnabled(relationEdit->isEnabled());
	inverseLabel->setEnabled(inverseEdit->isEnabled());
	priorityLabel->setEnabled(priorityEdit->isEnabled());
	mbunLabel->setEnabled(mbunEdit->isEnabled());
	namesAction->setEnabled(i != 2);
}
void UnitEditDialog::onUnitChanged() {
	okButton->setEnabled(!nameEdit->isReadOnly() && !nameEdit->text().trimmed().isEmpty() && (typeCombo->currentIndex() == 0 || (!baseEdit->text().trimmed().isEmpty() && (typeCombo->currentIndex() != 1 || !relationEdit->text().trimmed().isEmpty()))));
}
void UnitEditDialog::exponentChanged(int i) {
	if(i != 1) {
		mixBox->setChecked(false);
		priorityEdit->setEnabled(false);
		mbunEdit->setEnabled(false);
		priorityLabel->setEnabled(false);
		mbunLabel->setEnabled(false);
	}
	mixBox->setEnabled(i == 1);
	onUnitChanged();
}
void UnitEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
Unit *UnitEditDialog::editUnit(QWidget *parent, Unit *u, ExpressionItem **replaced_item) {
	UnitEditDialog *d = new UnitEditDialog(parent);
	d->setWindowTitle(tr("Edit Unit"));
	d->setUnit(u);
	while(d->exec() == QDialog::Accepted) {
		Unit *u_new = d->modifyUnit(u, replaced_item);
		if(u_new) {
			d->deleteLater();
			return u_new;
		}
	}
	d->deleteLater();
	return NULL;
}
Unit *UnitEditDialog::newUnit(QWidget *parent, ExpressionItem **replaced_item) {
	UnitEditDialog *d = new UnitEditDialog(parent);
	d->setWindowTitle(tr("New Unit"));
	Unit *u = NULL;
	while(d->exec() == QDialog::Accepted) {
		u = d->createUnit(replaced_item);
		if(u) break;
	}
	d->deleteLater();
	return u;
}

