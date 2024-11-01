/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "periodictabledialog.h"

PeriodicTableDialog::PeriodicTableDialog(QWidget *parent) : QDialog(parent, Qt::Window | Qt::MSWindowsFixedSizeDialogHint) {
	setWindowTitle(tr("Periodic Table"));
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	topbox->addLayout(grid);
	grid->setSpacing(3);
	QFontMetrics fm(font());
	QSize size = fm.boundingRect("AB").size();
	size.setWidth(size.width() * 2); size.setHeight(size.height() * 2);
	for(int i = 1; i <= 18; i++) {
		QLabel *label = new QLabel(QString::number(i), this);
		label->setMinimumSize(size);
		label->setAlignment(Qt::AlignCenter);
		grid->addWidget(label, 0, i > 2 ? i + 1 : i, Qt::AlignCenter);
	}
	for(int i = 1; i <= 7; i++) {
		QLabel *label = new QLabel(QString::number(i), this);
		label->setMinimumSize(size);
		label->setAlignment(Qt::AlignCenter);
		grid->addWidget(label, i, 0, Qt::AlignCenter);
	}
	QLabel *label = new QLabel("*", this);
	label->setMinimumSize(size);
	label->setAlignment(Qt::AlignCenter);
	grid->addWidget(label, 6, 3, Qt::AlignCenter);
	label = new QLabel("**", this);
	label->setMinimumSize(size);
	label->setAlignment(Qt::AlignCenter);
	grid->addWidget(label, 7, 3, Qt::AlignCenter);
	label = new QLabel("*", this);
	label->setMinimumSize(size);
	label->setAlignment(Qt::AlignCenter);
	grid->addWidget(label, 9, 3, Qt::AlignCenter);
	label = new QLabel("**", this);
	label->setMinimumSize(size);
	label->setAlignment(Qt::AlignCenter);
	grid->addWidget(label, 10, 3, Qt::AlignCenter);
	DataSet *dc = CALCULATOR->getDataSet("atom");
	if(dc) {
		DataProperty *p_xpos = dc->getProperty("x_pos");
		DataProperty *p_ypos = dc->getProperty("y_pos");
		DataProperty *p_weight = dc->getProperty("weight");
		DataProperty *p_number = dc->getProperty("number");
		DataProperty *p_symbol = dc->getProperty("symbol");
		DataProperty *p_class = dc->getProperty("class");
		DataProperty *p_name = dc->getProperty("name");
		if(p_xpos && p_ypos && p_weight && p_number && p_symbol && p_class && p_name) {
			int x_pos = 0, y_pos = 0;
			for(int i = 1; i < 120; i++) {
				DataObject *e = dc->getObject(i2s(i));
				if(e) {
					x_pos = s2i(e->getProperty(p_xpos));
					y_pos = s2i(e->getProperty(p_ypos));
				}
				if(e && x_pos > 0 && x_pos <= 18 && y_pos > 0 && y_pos <= 10) {
					QPushButton *e_button = new QPushButton(QString::fromStdString(e->getProperty(p_symbol)), this);
					e_button->setFlat(true);
					int group = s2i(e->getProperty(p_class));
					if(group > 0 && group <= 11) {
						QPalette pal = e_button->palette();
						switch(group) {
							case 1: {pal.setColor(QPalette::Button, QColor("#eeccee")); break;}
							case 2: {pal.setColor(QPalette::Button, QColor("#ddccee")); break;}
							case 3: {pal.setColor(QPalette::Button, QColor("#ccddff")); break;}
							case 4: {pal.setColor(QPalette::Button, QColor("#ddeeff")); break;}
							case 5: {pal.setColor(QPalette::Button, QColor("#cceeee")); break;}
							case 6: {pal.setColor(QPalette::Button, QColor("#bbffbb")); break;}
							case 7: {pal.setColor(QPalette::Button, QColor("#eeffdd")); break;}
							case 8: {pal.setColor(QPalette::Button, QColor("#ffffaa")); break;}
							case 9: {pal.setColor(QPalette::Button, QColor("#ffddaa")); break;}
							case 10: {pal.setColor(QPalette::Button, QColor("#ffccdd")); break;}
							case 11: {pal.setColor(QPalette::Button, QColor("#aaeedd")); break;}
							default: {}
						}
						pal.setColor(QPalette::ButtonText, QColor("#000000"));
						pal.setColor(QPalette::Text, QColor("#000000"));
						pal.setColor(QPalette::WindowText, QColor("#000000"));
						e_button->setAutoFillBackground(true);
						e_button->setPalette(pal);
						e_button->update();
					}
					if(x_pos > 2) {
						grid->addWidget(e_button, y_pos, x_pos + 1);
					} else {
						grid->addWidget(e_button, y_pos, x_pos);
					}
					std::string tip = e->getProperty(p_number);
					tip += " ";
					tip += e->getProperty(p_name);
					std::string weight = e->getPropertyDisplayString(p_weight);
					if(!weight.empty() && weight != "-") {
						tip += "\n";
						tip += weight;
					}
					e_button->setToolTip(QString::fromStdString(tip));
					e_button->setProperty("ELEMENT OBJECT", QVariant::fromValue((void*) e));
					if(i == 1) {
						QMargins margins = e_button->contentsMargins();
						if(margins.left() + margins.right() > size.width() / 2 - 3) {
							size.setWidth(size.width() / 2 + 4 + margins.left() + margins.right());
						}
						if(margins.bottom() + margins.top() > size.height() / 2 - 3) {
							size.setWidth(size.height() / 2 + 4 + margins.bottom() + margins.top());
						}
					}
					e_button->setMinimumSize(size);
					e_button->setMaximumSize(size);
					connect(e_button, SIGNAL(clicked()), this, SLOT(elementClicked()));
				}
			}
		}
	}
	for(int i = 0; i < 20; i++) {
		grid->setColumnStretch(i, 1);
	}
	for(int i = 0; i < 10; i++) {
		grid->setRowStretch(i, 1);
	}
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	buttonBox->setFocus();
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
PeriodicTableDialog::~PeriodicTableDialog() {}

void PeriodicTableDialog::reject() {
	for(QMap<DataObject*, QDialog*>::const_iterator it = elementDialogs.constBegin(); it != elementDialogs.constEnd(); ++it) {
		(*it)->reject();
	}
	QDialog::reject();
}
void PeriodicTableDialog::onAlwaysOnTopChanged() {
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	else setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	for(QMap<DataObject*, QDialog*>::const_iterator it = elementDialogs.constBegin(); it != elementDialogs.constEnd(); ++it) {
		if(settings->always_on_top) (*it)->setWindowFlags((*it)->windowFlags() | Qt::WindowStaysOnTopHint);
		else (*it)->setWindowFlags((*it)->windowFlags() & ~Qt::WindowStaysOnTopHint);
	}
}
void PeriodicTableDialog::elementClicked() {
	DataObject *e = (DataObject*) sender()->property("ELEMENT OBJECT").value<void*>();
	QMap<DataObject*, QDialog*>::const_iterator qit = elementDialogs.find(e);
	if(qit != elementDialogs.constEnd()) {
		(*qit)->show();
		qApp->processEvents();
		(*qit)->raise();
		(*qit)->activateWindow();
		return;
	}
	QDialog *dialog = new QDialog(this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	dialog->setWindowTitle(tr("Element Data"));
	QVBoxLayout *topbox = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	topbox->addLayout(grid);
	DataSet *ds = e->parentSet();
	DataProperty *p_number = ds->getProperty("number");
	DataProperty *p_symbol = ds->getProperty("symbol");
	DataProperty *p_class = ds->getProperty("class");
	DataProperty *p_name = ds->getProperty("name");
	int group = s2i(e->getProperty(p_class));
	QString group_name;
	switch(group) {
		case ALKALI_METALS: {group_name = tr("Alkali Metal"); break;}
		case ALKALI_EARTH_METALS: {group_name = tr("Alkaline-Earth Metal"); break;}
		case LANTHANIDES: {group_name = tr("Lanthanide"); break;}
		case ACTINIDES: {group_name = tr("Actinide"); break;}
		case TRANSITION_METALS: {group_name = tr("Transition Metal"); break;}
		case METALS: {group_name = tr("Metal"); break;}
		case METALLOIDS: {group_name = tr("Metalloid"); break;}
		case NONMETALS: {group_name = tr("Polyatomic Non-Metal"); break;}
		case HALOGENS: {group_name = tr("Diatomic Non-Metal"); break;}
		case NOBLE_GASES: {group_name = tr("Noble Gas"); break;}
		case TRANSACTINIDES: {group_name = tr("Unknown chemical properties"); break;}
		default: {group_name = tr("Unknown"); break;}
	}
	grid->addWidget(new QLabel(QStringLiteral("<div style=\"font-size: large\">%1</div>").arg(QString::fromStdString(e->getProperty(p_number))), dialog), 0, 0, 1, 3, Qt::AlignRight);
	grid->addWidget(new QLabel(QStringLiteral("<div style=\"font-size: xx-large\">%1</div>").arg(QString::fromStdString(e->getProperty(p_symbol))), dialog), 1, 0, 1, 3);
	grid->addWidget(new QLabel(QStringLiteral("<div style=\"font-size: x-large\">%1</div>").arg(QString::fromStdString(e->getProperty(p_name))), dialog), 2, 0, 1, 3);
	grid->addWidget(new QLabel(QStringLiteral("<div style=\"font-weight: bold\">%1</div>").arg(tr("%1:").arg(QString::fromStdString(p_class->title()))), dialog), 3, 0);
	grid->addWidget(new QLabel(group_name, dialog), 3, 1);
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	int row = 4;
	while(dp) {
		if(!dp->isHidden() && dp != p_number && dp != p_class && dp != p_symbol && dp != p_name) {
			std::string sval = e->getPropertyDisplayString(dp);
			if(!sval.empty()) {
				grid->addWidget(new QLabel(QStringLiteral("<div style=\"font-weight: bold\">%1</div>").arg(tr("%1:").arg(QString::fromStdString(dp->title()))), dialog), row, 0);
				grid->addWidget(new QLabel(QString::fromStdString(sval), dialog), row, 1);
				QPushButton *button = new QPushButton(LOAD_ICON("edit-paste"), QString(), dialog);
				grid->addWidget(button, row, 2);
				button->setProperty("ELEMENT OBJECT", QVariant::fromValue((void*) e));
				button->setProperty("ELEMENT PROPERTY", QVariant::fromValue((void*) dp));
				connect(button, SIGNAL(clicked()), this, SLOT(propertyClicked()));
				row++;
			}
		}
		dp = ds->getNextProperty(&it);
	}
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, dialog);
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), dialog, SLOT(reject()));
	dialog->layout()->setSizeConstraint(QLayout::SetFixedSize);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	elementDialogs[e] = dialog;
	dialog->show();
}
void PeriodicTableDialog::propertyClicked() {
	emit insertPropertyRequest((DataObject*) sender()->property("ELEMENT OBJECT").value<void*>(), (DataProperty*) sender()->property("ELEMENT PROPERTY").value<void*>());
}

