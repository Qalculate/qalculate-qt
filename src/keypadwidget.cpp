/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QGridLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QLabel>
#include <QTimer>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTextDocument>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QScrollBar>
#include <QWidgetAction>
#include <QComboBox>
#include <QDebug>

#include "keypadwidget.h"
#include "qalculateqtsettings.h"

#define BUTTON_DATA "QALCULATE DATA1"
#define BUTTON_DATA2 "QALCULATE DATA2"
#define BUTTON_DATA3 "QALCULATE DATA3"
#define BUTTON_VALUE "QALCULATE VALUE1"
#define BUTTON_VALUE2 "QALCULATE VALUE2"
#define BUTTON_VALUE3 "QALCULATE VALUE3"

#define SYMBOL_BUTTON_BOX(x)			button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						connect(button, SIGNAL(clicked()), this, SLOT(onSymbolButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onSymbolButtonClicked())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onSymbolButtonClicked())); \
						box->addWidget(button);

#define SYMBOL_BUTTON3(x, y, z, r, c)		button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						button->setProperty(BUTTON_DATA2, y); \
						button->setProperty(BUTTON_DATA3, z); \
						connect(button, SIGNAL(clicked()), this, SLOT(onSymbolButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onSymbolButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onSymbolButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define SYMBOL_OPERATOR_SYMBOL_BUTTON(x, y, z, r, c)	button = new KeypadButton(x, this); \
							button->setProperty(BUTTON_DATA, x); \
							button->setProperty(BUTTON_DATA2, y); \
							button->setProperty(BUTTON_DATA3, z); \
							connect(button, SIGNAL(clicked()), this, SLOT(onSymbolButtonClicked())); \
							connect(button, SIGNAL(clicked2()), this, SLOT(onOperatorButtonClicked2())); \
							connect(button, SIGNAL(clicked3()), this, SLOT(onSymbolButtonClicked3())); \
							grid->addWidget(button, r, c, 1, 1);

#define SYMBOL_OPERATOR_ITEM_BUTTON(x, y, z, r, c)	button = new KeypadButton(x, this); \
							button->setProperty(BUTTON_DATA, x); \
							button->setProperty(BUTTON_DATA2, y); \
							button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) z)); \
							connect(button, SIGNAL(clicked()), this, SLOT(onSymbolButtonClicked())); \
							connect(button, SIGNAL(clicked2()), this, SLOT(onOperatorButtonClicked2())); \
							connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
							grid->addWidget(button, r, c, 1, 1);

#define SYMBOL2_ITEM_BUTTON(x, y, o, r, c)	button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						button->setProperty(BUTTON_DATA2, y); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o)); \
						connect(button, SIGNAL(clicked()), this, SLOT(onSymbolButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onSymbolButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define SYMBOL_ITEM2_BUTTON(x, o1, o2, r, c)	button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) o1)); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o2)); \
						connect(button, SIGNAL(clicked()), this, SLOT(onSymbolButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define OPERATOR_ITEM2_BUTTON(x, o1, o2, r, c)	button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) o1)); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o2)); \
						connect(button, SIGNAL(clicked()), this, SLOT(onOperatorButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define SYMBOL_BUTTON(x, r, c)			SYMBOL_BUTTON3(x, x, x, r, c)

#define SYMBOL_BUTTON2(x, y, r, c)		SYMBOL_BUTTON3(x, y, y, r, c)


#define OPERATOR_BUTTON3(x, y, z, r, c)		button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						button->setProperty(BUTTON_DATA2, y); \
						button->setProperty(BUTTON_DATA3, z); \
						connect(button, SIGNAL(clicked()), this, SLOT(onOperatorButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onOperatorButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onOperatorButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define OPERATOR_ITEM_SYMBOL_BUTTON(x, y, z, r, c)	button = new KeypadButton(x, this); \
							button->setProperty(BUTTON_DATA, x); \
							button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) y)); \
							button->setProperty(BUTTON_DATA3, z); \
							connect(button, SIGNAL(clicked()), this, SLOT(onOperatorButtonClicked())); \
							connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked2())); \
							connect(button, SIGNAL(clicked3()), this, SLOT(onSymbolButtonClicked3())); \
							grid->addWidget(button, r, c, 1, 1);

#define OPERATOR_SYMBOL_BUTTON(x, y, r, c)	button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, x); \
						button->setProperty(BUTTON_DATA2, y); \
						button->setProperty(BUTTON_DATA3, y); \
						connect(button, SIGNAL(clicked()), this, SLOT(onOperatorButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onSymbolButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onSymbolButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define OPERATOR_BUTTON(x, r, c) 		OPERATOR_BUTTON3(x, x, x, r, c)
#define OPERATOR_BUTTON2(x, y, r, c) 		OPERATOR_BUTTON3(x, y, y, r, c)

#define ITEM_BUTTON3(o1, o2, o3, x, r, c)	button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) o1)); \
						button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) o2)); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o3)); \
						button->setToolTip(QString::fromStdString(o1->title(true, settings->printops.use_unicode_signs)), (void*) o1 == (void*) o2 ? QString() : QString::fromStdString(o2->title(true, settings->printops.use_unicode_signs)), (void*) o2 == (void*) o3 ? QString() : QString::fromStdString(o3->title(true, settings->printops.use_unicode_signs))); \
						connect(button, SIGNAL(clicked()), this, SLOT(onItemButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define ITEM_OPERATOR_ITEM_BUTTON(o1, o2, o3, x, r, c)	button = new KeypadButton(x, this); \
							button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) o1)); \
							button->setProperty(BUTTON_DATA2, o2); \
							button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o3)); \
							button->setToolTip(QString::fromStdString(o1->title(true, settings->printops.use_unicode_signs)), o2, QString::fromStdString(o3->title(true, settings->printops.use_unicode_signs))); \
							connect(button, SIGNAL(clicked()), this, SLOT(onItemButtonClicked())); \
							connect(button, SIGNAL(clicked2()), this, SLOT(onOperatorButtonClicked2())); \
							connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
							grid->addWidget(button, r, c, 1, 1);

#define SET_ITEM_BUTTON2(button, o1, o2)	button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) o1)); \
						button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) o2)); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o2)); \
						button->setToolTip(QString::fromStdString(o1->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(o2->title(true, settings->printops.use_unicode_signs)));

#define ITEM_BUTTON(o, x, r, c) 		ITEM_BUTTON3(o, o, o, x, r, c)
#define ITEM_BUTTON2(o1, o2, x, r, c) 		ITEM_BUTTON3(o1, o2, o2, x, r, c)

#define BASE_BUTTON(x, i, r, c)			button = new KeypadButton(x, this); \
						button->setCheckable(true); \
						grid->addWidget(button, r, c); \
						button->setProperty(BUTTON_DATA, i); \
						connect(button, SIGNAL(clicked()), this, SLOT(onBaseButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onBaseButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onBaseButtonClicked2()));

#define MENU_ITEM(x)				item = x;\
						if(item) {\
							action = menu->addAction(QString::fromStdString(item->title(true, settings->printops.use_unicode_signs)), this, SLOT(onItemButtonClicked()));\
							action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) item));\
						}

#define MENU_SYMBOL(x)				action = menu->addAction(x, this, SLOT(onSymbolButtonClicked()));\
						action->setProperty(BUTTON_DATA, x);

#define CREATE_MENU				menu = new QMenu(this);\
						button->setMenu(menu);\
						button->setPopupMode(settings->separate_keypad_menu_buttons ? QToolButton::MenuButtonPopup : QToolButton::DelayedPopup);\
						button->menuSet();

#define ADD_SECTION(str) \
	if(!menu->style()->styleHint(QStyle::SH_Menu_SupportsSections)) { \
		aw = new QWidgetAction(this); \
		QLabel *label = new QLabel(str, this); \
		label->setAlignment(Qt::AlignCenter); \
		aw->setDefaultWidget(label); \
		menu->addSeparator(); \
		menu->addAction(aw); \
		menu->addSeparator(); \
	} else { \
		menu->addSection(str); \
	}

void KeypadWidget::createKeypad(int i) {
	KeypadButton *button;
	QMenu *menu;
	MathFunction *f, *f2;
	int c = 0;
	QGridLayout *grid;
	QHBoxLayout *box;
	if(i == KEYPAD_GENERAL && !sinButton) {
		grid = new QGridLayout(keypadG);
		grid->setContentsMargins(0, 0, 0, 0);
		button = new KeypadButton("MS", this);
		connect(button, SIGNAL(clicked()), this, SIGNAL(MSClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(MSClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(MSClicked()));
		button->setToolTip(tr("Memory store"));
		grid->addWidget(button, c, 0, 1, 1);
		button = new KeypadButton("MC", this);
		button->setToolTip(tr("Memory clear"));
		connect(button, SIGNAL(clicked()), this, SIGNAL(MCClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(MCClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(MCClicked()));
		grid->addWidget(button, c, 1, 1, 1);
		button = new KeypadButton("MR", this);
		button->setToolTip(tr("Memory recall"));
		connect(button, SIGNAL(clicked()), this, SIGNAL(MRClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(MRClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(MRClicked()));
		grid->addWidget(button, c, 2, 1, 1);
		button = new KeypadButton("M+", this);
		button->setToolTip(tr("Memory add"), tr("Memory subtract"));
		connect(button, SIGNAL(clicked()), this, SIGNAL(MPlusClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(MMinusClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(MMinusClicked()));
		grid->addWidget(button, c, 3, 1, 1);
		//: Standard calculator button. Do not use more than three characters.
		button = new KeypadButton(tr("STO"), this);
		connect(button, SIGNAL(clicked()), this, SIGNAL(storeClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(newFunctionClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(newFunctionClicked()));
		button->setToolTip(tr("Store"), tr("New function"));
		grid->addWidget(button, c, 4, 1, 1);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateStoreMenu()));
		c++;
		button = new KeypadButton("hyp");
		button->setCheckable(true);
		grid->addWidget(button, c, 0, 1, 1);
		connect(button, SIGNAL(toggled(bool)), this, SLOT(onHypToggled(bool)));
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), CALCULATOR->getFunctionById(FUNCTION_ID_ASIN), tr("sin"), c, 1);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateSinMenu()));
		sinButton = button;
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_COS), CALCULATOR->getFunctionById(FUNCTION_ID_ACOS), tr("cos"), c, 2);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateCosMenu()));
		cosButton = button;
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_TAN), CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), tr("tan"), c, 3);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateTanMenu()));
		tanButton = button;
		ITEM_BUTTON3(CALCULATOR->getVariableById(VARIABLE_ID_PI), CALCULATOR->getVariableById(VARIABLE_ID_EULER), CALCULATOR->getVariableById(VARIABLE_ID_CATALAN), SIGN_PI, c, 4);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updatePiMenu()));
		c++;
		OPERATOR_ITEM2_BUTTON("^", CALCULATOR->getFunctionById(FUNCTION_ID_SQUARE), CALCULATOR->getFunctionById(FUNCTION_ID_EXP), c, 2);
		button->setRichText("x<sup>y</sup>");
		button->setToolTip(tr("Exponentiation"), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_SQUARE)->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_EXP)->title(true, settings->printops.use_unicode_signs)));
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updatePowerMenu()));
		ITEM_BUTTON3(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), CALCULATOR->getFunctionById(FUNCTION_ID_CBRT), CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), SIGN_SQRT, c, 1);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateSqrtMenu()));
		f = CALCULATOR->getActiveFunction("log10");
		if(f) {
			ITEM_BUTTON3(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), f, CALCULATOR->getFunctionById(FUNCTION_ID_LOGN), "ln", c, 0);
		} else {
			ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), CALCULATOR->getFunctionById(FUNCTION_ID_LOGN), "ln", c, 0);
		}
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateLnMenu()));
		ITEM_OPERATOR_ITEM_BUTTON(CALCULATOR->getVariableById(VARIABLE_ID_I), "∠", CALCULATOR->getFunctionById(FUNCTION_ID_ARG), CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0 ? "j" : "i", c, 3);
		imaginaryButton = button;
		QFont ifont(button->font());
		ifont.setStyle(QFont::StyleItalic);
		button->setFont(ifont);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateComplexMenu()));
		f = CALCULATOR->getActiveFunction("cis");
		if(f) {
			ITEM_BUTTON3(CALCULATOR->getVariableById(VARIABLE_ID_E), CALCULATOR->getFunctionById(FUNCTION_ID_EXP), f, "e", c, 4);
		} else {
			ITEM_BUTTON2(CALCULATOR->getVariableById(VARIABLE_ID_E), CALCULATOR->getFunctionById(FUNCTION_ID_EXP), "e", c, 4);
		}
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateEMenu()));
		c++;
		f = CALCULATOR->getActiveFunction("perm"); f2 = CALCULATOR->getActiveFunction("comb");
		if(f && f2) {
			OPERATOR_ITEM2_BUTTON("!", f, f2, c, 0);
			button->setToolTip(QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_FACTORIAL)->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(f2->title(true, settings->printops.use_unicode_signs)));
		} else {
			OPERATOR_BUTTON("!", c, 0);
		}
		button->setText("x!");
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateFactorialMenu()));
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_SUM), CALCULATOR->getFunctionById(FUNCTION_ID_PRODUCT), "Σ", c, 1);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateSumMenu()));
		SYMBOL_BUTTON3("x", "y", "z", c, 2);
		button->setToolTip(QString(), "<i>y</i>", "<i>z</i>");
		button->setFont(ifont);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateXMenu()));
		SYMBOL_BUTTON("=", c, 3);
		button->setRichText("<i>x</i> =");
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateEqualsMenu()));
		button = new KeypadButton("<font size=\"-1\">a(x)<sup>b</sup></font>", this);
		connect(button, SIGNAL(clicked()), this, SIGNAL(factorizeClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(expandClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(expandClicked()));
		button->setToolTip(tr("Factorize"), tr("Expand"));
		grid->addWidget(button, c, 4);
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateFactorizeMenu()));

		c++;
		if(!settings->show_percent_in_numpad) {
			SYMBOL_BUTTON2("%", "‰", c, 1);
			button->setToolTip(tr("Percent or remainder"), QString::fromStdString(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE)->title(true, settings->printops.use_unicode_signs)));
			CREATE_MENU
			connect(menu, SIGNAL(aboutToShow()), this, SLOT(updatePercentageMenu()));
			percentButton = button;
		} else {
			button = new KeypadButton("(x)", this);
			button->setToolTip(tr("Smart parentheses"), tr("Vector brackets"));
			connect(button, SIGNAL(clicked()), this, SIGNAL(parenthesesClicked()));
			connect(button, SIGNAL(clicked2()), this, SIGNAL(bracketsClicked()));
			connect(button, SIGNAL(clicked3()), this, SIGNAL(bracketsClicked()));
			grid->addWidget(button, c, 2, 1, 1);
			parButton = button;
		}
		SYMBOL_ITEM2_BUTTON("±", CALCULATOR->getFunctionById(FUNCTION_ID_UNCERTAINTY), CALCULATOR->getFunctionById(FUNCTION_ID_INTERVAL), c, 0);
		button->setToolTip(tr("Uncertainty/interval"), tr("Relative error"), tr("Interval"));
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateIntervalMenu()));
		std::string sunit = settings->latest_button_unit;
		if(sunit.empty()) sunit = "m";
		Unit *u = CALCULATOR->getActiveUnit(sunit);
		button = new KeypadButton(QString::fromStdString(sunit), this);
		Prefix *p1 = CALCULATOR->getExactDecimalPrefix(-3), *p2 = CALCULATOR->getExactDecimalPrefix(3);
		button->setProperty(BUTTON_DATA, u ? QVariant::fromValue((void*) u) : QString::fromStdString(sunit));
		button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) p1));
		button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) p2));
		connect(button, SIGNAL(clicked()), this, SLOT(onItemButtonClicked()));
		connect(button, SIGNAL(clicked2()), this, SLOT(onUnitButtonClicked2()));
		connect(button, SIGNAL(clicked3()), this, SLOT(onUnitButtonClicked3()));
		grid->addWidget(button, c, settings->show_percent_in_numpad ? 1 : 2, 1, 1);
		unitButton = button;
		unitButton->setToolTip(QString::fromStdString(u ? u->title(true, settings->printops.use_unicode_signs) : sunit), p1 ? QString::fromStdString(p1->longName()) : QString(), p2 ? QString::fromStdString(p2->longName()) : QString());
		CREATE_MENU
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateUnitsMenu()));
		backButton = new KeypadButton(LOAD_ICON("go-back"), this, true);
		backButton->setToolTip(tr("Move cursor left"), tr("Move cursor to start"));
		connect(backButton, SIGNAL(clicked()), this, SIGNAL(leftClicked()));
		connect(backButton, SIGNAL(clicked2()), this, SIGNAL(startClicked()));
		connect(backButton, SIGNAL(clicked3()), this, SIGNAL(startClicked()));
		grid->addWidget(backButton, c, 3, 1, 1);
		forwardButton = new KeypadButton(LOAD_ICON("go-forward"), this, true);
		forwardButton->setToolTip(tr("Move cursor right"), tr("Move cursor to end"));
		connect(forwardButton, SIGNAL(clicked()), this, SIGNAL(rightClicked()));
		connect(forwardButton, SIGNAL(clicked2()), this, SIGNAL(endClicked()));
		connect(forwardButton, SIGNAL(clicked3()), this, SIGNAL(endClicked()));
		grid->addWidget(forwardButton, c, 4, 1, 1);
		for(c = 0; c < 5; c++) grid->setColumnStretch(c, 1);
		for(int r = 0; r < 5; r++) grid->setRowStretch(r, 1);
	} else if(i == KEYPAD_PROGRAMMING && !binButton) {
		grid = new QGridLayout(keypadP);
		grid->setContentsMargins(0, 0, 0, 0);
		BASE_BUTTON("BIN", 2, 0, 0); binButton = button;
		BASE_BUTTON("OCT", 8, 0, 1); octButton = button;
		BASE_BUTTON("DEC", 10, 0, 2); decButton = button;
		BASE_BUTTON("HEX", 16, 0, 3); hexButton = button;
		box = new QHBoxLayout();
		SYMBOL_BUTTON_BOX("A"); aButton = button;
		SYMBOL_BUTTON_BOX("B"); bButton = button;
		SYMBOL_BUTTON_BOX("C"); cButton = button;
		SYMBOL_BUTTON_BOX("D"); dButton = button;
		SYMBOL_BUTTON_BOX("E"); eButton = button;
		SYMBOL_BUTTON_BOX("F"); fButton = button;
		updateBase();
		grid->addLayout(box, 1, 0, 1, 4);
		OPERATOR_BUTTON2("&", "&&", 2, 0);
		button->setText("AND");
		button->setToolTip(tr("Bitwise AND"), tr("Logical AND"));
		OPERATOR_BUTTON2("|", "||", 2, 1);
		button->setText("OR");
		button->setToolTip(tr("Bitwise OR"), tr("Logical OR"));
		OPERATOR_BUTTON("xor", 2, 2);
		button->setText("XOR");
		button->setToolTip(tr("Bitwise Exclusive OR"));
		OPERATOR_BUTTON2("~", "NOT", 2, 3);
		button->setText("NOT");
		button->setToolTip(tr("Bitwise NOT"), tr("Logical NOT"));
		OPERATOR_BUTTON("<<", 3, 0);
		button->setToolTip(tr("Bitwise Left Shift"));
		OPERATOR_BUTTON(">>", 3, 1);
		button->setToolTip(tr("Bitwise Right Shift"));
		ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_BIT_CMP), tr("cmp"), 3, 2);
		ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_CIRCULAR_SHIFT), tr("rot"), 3, 3);
		OPERATOR_BUTTON2("mod", "rem", 4, 0);
		button->setText("mod");
		button->setToolTip(QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_MOD)->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_REM)->title(true, settings->printops.use_unicode_signs)));
		OPERATOR_BUTTON("//", 4, 1);
		button->setText("div");
		f = CALCULATOR->getActiveFunction("div");
		if(f) button->setToolTip(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)));
		f = CALCULATOR->getActiveFunction("log10"); f2 = CALCULATOR->getActiveFunction("log2");
		if(f && f2) {
			ITEM_BUTTON2(f2, f, "log<sub>2</sub>", 4, 2);
		} else {
			ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), "ln", 4, 2);
		}
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_ASCII), CALCULATOR->getFunctionById(FUNCTION_ID_CHAR), tr("a→1"), 4, 3);
		for(c = 0; c < 4; c++) grid->setColumnStretch(c, 1);
		for(int r = 0; r < 5; r++) grid->setRowStretch(r, 1);
	} else if(i == KEYPAD_ALGEBRA && !xButton) {
		grid = new QGridLayout(keypadX);
		grid->setContentsMargins(0, 0, 0, 0);
		for(size_t i = 0; i < 3; i++) {
			SYMBOL_BUTTON((i == 0 ? "x" : (i == 1 ? "y" : "z")), 0, i);
			if(i == 0) xButton = button;
			int id = 0;
			if(i == 0) id = VARIABLE_ID_X;
			else if(i == 1) id = VARIABLE_ID_Y;
			else if(i == 2) id = VARIABLE_ID_Z;
			QFont ifont(button->font());
			ifont.setStyle(QFont::StyleItalic);
			button->setFont(ifont);
			menu = new QMenu(this);
			button->setMenu(menu);
			connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateAssumptions()));
			menu->setProperty(BUTTON_DATA, id);
		}
		SYMBOL_BUTTON("n", 0, 3);
		SYMBOL_BUTTON2("=", SIGN_NOT_EQUAL, 1, 2);
		SYMBOL_BUTTON("/.", 1, 3);
		button->setToolTip(QString::fromStdString(CALCULATOR->localWhereString()));
		SYMBOL_BUTTON("<", 2, 0);
		SYMBOL_BUTTON(SIGN_LESS_OR_EQUAL, 2, 1);
		SYMBOL_BUTTON(">", 2, 2);
		SYMBOL_BUTTON(SIGN_GREATER_OR_EQUAL, 2, 3);
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_SUM), CALCULATOR->getFunctionById(FUNCTION_ID_PRODUCT), "∑", 3, 0);
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_DIFFERENTIATE), CALCULATOR->getFunctionById(FUNCTION_ID_D_SOLVE), "<font size=\"-1\"><i>d/dx</i></font>", 3, 1);
		ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_INTEGRATE), "∫", 3, 2);
		button = new KeypadButton("<font size=\"-1\">a(x)<sup>b</sup></font>", this);
		connect(button, SIGNAL(clicked()), this, SIGNAL(factorizeClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(expandClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(expandPartialFractionsClicked()));
		button->setToolTip(tr("Factorize"), tr("Expand"), tr("Expand partial fractions"));
		grid->addWidget(button, 3, 3);
		ITEM_BUTTON(CALCULATOR->getVariableById(VARIABLE_ID_PI), SIGN_PI, 1, 0);
		ITEM_BUTTON2(CALCULATOR->getVariableById(VARIABLE_ID_E), CALCULATOR->getFunctionById(FUNCTION_ID_EXP), "e", 1, 1);
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), SIGN_SQRT, 4, 0);
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_CBRT), CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), "∛", 4, 1);
		ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), CALCULATOR->getFunctionById(FUNCTION_ID_LOGN), "ln", 4, 2);
		ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), "|x|", 4, 3);
		for(c = 0; c < 4; c++) grid->setColumnStretch(c, 1);
		for(int r = 0; r < 5; r++) grid->setRowStretch(r, 1);
	} else if(i == KEYPAD_CUSTOM && !customEditButton) {
		grid = new QGridLayout(keypadC);
		grid->setContentsMargins(0, 0, 0, 0);
		customGrid = grid;
		customEditButton = new KeypadButton(LOAD_ICON("document-edit"), this);
		customEditButton->setCheckable(true);
		menu = new QMenu(this);
		addColumnAction = menu->addAction(tr("Add column"), this, SLOT(addCustomColumn())); addColumnAction->setEnabled(settings->custom_button_columns < 100);
		addRowAction = menu->addAction(tr("Add row"), this, SLOT(addCustomRow())); addRowAction->setEnabled(settings->custom_button_rows < 100);
		removeColumnAction = menu->addAction(tr("Remove column"), this, SLOT(removeCustomColumn())); removeColumnAction->setEnabled(settings->custom_button_columns > 1);
		removeRowAction = menu->addAction(tr("Remove row"), this, SLOT(removeCustomRow())); removeRowAction->setEnabled(settings->custom_button_rows > 1);
		customEditButton->setMenu(menu);
		customEditButton->setPopupMode(QToolButton::MenuButtonPopup);
		connect(customEditButton, SIGNAL(toggled(bool)), this, SLOT(onCustomEditClicked(bool)));
		grid->addWidget(customEditButton, 0, 0);
		customButtons.resize(settings->custom_button_columns);
		for(c = 0; c < settings->custom_button_columns; c++) {
			customButtons[c].resize(settings->custom_button_rows);
			for(int r = 0; r < settings->custom_button_rows; r++) {
				if(c == 0 && r == 0) {
					customButtons[c][r] = NULL;
				} else {
					button = new KeypadButton(QString(), this);
					connect(button, SIGNAL(clicked()), this, SLOT(onCustomButtonClicked()));
					connect(button, SIGNAL(clicked2()), this, SLOT(onCustomButtonClicked2()));
					connect(button, SIGNAL(clicked3()), this, SLOT(onCustomButtonClicked3()));
					grid->addWidget(button, r, c);
					customButtons[c][r] = button;
				}
				grid->setRowStretch(r, 1);
			}
			grid->setColumnStretch(c, 1);
		}
		for(size_t i = 0; i < settings->custom_buttons.size();) {
			custom_button *cb = &settings->custom_buttons[i];
			if(cb->c > 0 && cb->r > 0 && (cb->c != 1 || cb->r != 1) && cb->c <= customButtons.size() && cb->r <= customButtons[cb->c - 1].size()) {
				button = customButtons[cb->c - 1][cb->r - 1];
				if(button) {
					if(cb->label.contains("</")) button->setRichText(cb->label);
					else button->setText(cb->label);
					button->setProperty(BUTTON_DATA, cb->type[0]);
					button->setProperty(BUTTON_VALUE, QString::fromStdString(cb->value[0]));
					button->setProperty(BUTTON_DATA2, cb->type[1]);
					button->setProperty(BUTTON_VALUE2, QString::fromStdString(cb->value[1]));
					button->setProperty(BUTTON_DATA3, cb->type[2]);
					button->setProperty(BUTTON_VALUE3, QString::fromStdString(cb->value[2]));
					button->setToolTip(settings->shortcutText(cb->type[0], cb->value[0]), settings->shortcutText(cb->type[1], cb->value[1]), settings->shortcutText(cb->type[2], cb->value[2]));
				}
				i++;
			} else {
				settings->custom_buttons.erase(settings->custom_buttons.begin() + i);
			}
		}
		b_edit = false;
	} else if(i == KEYPAD_NUMBERPAD && !acButton[1]) {
		createNumpad(keypadN, 1);
	}
}
void KeypadWidget::updateButtonLocation() {
	KeypadButton *button, *prev_parbutton = parButton;
	QMenu *menu;
	if(settings->show_percent_in_numpad) {
		 if(percentButton && keypadG->layout()) {
			QGridLayout *grid = (QGridLayout*) keypadG->layout();
			percentButton->deleteLater();
			percentButton = NULL;
			button = new KeypadButton("(x)", this);
			button->setToolTip(tr("Smart parentheses"), tr("Vector brackets"));
			connect(button, SIGNAL(clicked()), this, SIGNAL(parenthesesClicked()));
			connect(button, SIGNAL(clicked2()), this, SIGNAL(bracketsClicked()));
			connect(button, SIGNAL(clicked3()), this, SIGNAL(bracketsClicked()));
			grid->removeWidget(unitButton);
			grid->addWidget(button, 4, 2, 1, 1);
			grid->addWidget(unitButton, 4, 1, 1, 1);
			parButton = button;
		}
		if(prev_parbutton && (settings->hide_numpad ? keypadN->layout() : numpad->layout())) {
			QGridLayout *grid = (QGridLayout*) (settings->hide_numpad ? keypadN->layout() : numpad->layout());
			prev_parbutton->deleteLater();
			if(parButton == prev_parbutton) parButton = NULL;
			SYMBOL_BUTTON2("%", "‰", 0, 0);
			button->setToolTip(tr("Percent or remainder"), QString::fromStdString(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE)->title(true, settings->printops.use_unicode_signs)));
			percentButton = button;
		}
	} else {
		if(percentButton && (settings->hide_numpad ? keypadN->layout() : numpad->layout())) {
			QGridLayout *grid = (QGridLayout*) (settings->hide_numpad ? keypadN->layout() : numpad->layout());
			percentButton->deleteLater();
			percentButton = NULL;
			button = new KeypadButton("(x)", this);
			button->setToolTip(tr("Smart parentheses"), tr("Vector brackets"));
			connect(button, SIGNAL(clicked()), this, SIGNAL(parenthesesClicked()));
			connect(button, SIGNAL(clicked2()), this, SIGNAL(bracketsClicked()));
			connect(button, SIGNAL(clicked3()), this, SIGNAL(bracketsClicked()));
			grid->addWidget(button, 0, 0, 1, 1);
			parButton = button;
		}
		if(prev_parbutton && keypadG->layout()) {
			QGridLayout *grid = (QGridLayout*) keypadG->layout();
			prev_parbutton->deleteLater();
			if(parButton == prev_parbutton) parButton = NULL;
			grid->removeWidget(unitButton);
			SYMBOL_BUTTON2("%", "‰", 4, 1);
			grid->addWidget(unitButton, 4, 2, 1, 1);
			button->setToolTip(tr("Percent or remainder"), QString::fromStdString(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE)->title(true, settings->printops.use_unicode_signs)));
			CREATE_MENU
			connect(menu, SIGNAL(aboutToShow()), this, SLOT(updatePercentageMenu()));
			percentButton = button;
		}
	}
}
void KeypadWidget::createNumpad(QWidget *w, int i) {
	if(acButton[i]) return;
	KeypadButton *button;
	MathFunction *f, *f2;
	QGridLayout *grid = new QGridLayout(w);
	grid->setContentsMargins(0, 0, 0, 0);
	int c = 0;
	SYMBOL_BUTTON2("(", "[", 1, c)
	button->setToolTip(tr("Left parenthesis"), tr("Left vector bracket"));
	SYMBOL_BUTTON2(")", "]", 2, c)
	button->setToolTip(tr("Right parenthesis"), tr("Right vector bracket"));
	if(settings->show_percent_in_numpad) {
		SYMBOL_BUTTON2("%", "‰", 0, c);
		button->setToolTip(tr("Percent or remainder"), QString::fromStdString(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE)->title(true, settings->printops.use_unicode_signs)));
		percentButton = button;
	} else {
		button = new KeypadButton("(x)", this);
		button->setToolTip(tr("Smart parentheses"), tr("Vector brackets"));
		connect(button, SIGNAL(clicked()), this, SIGNAL(parenthesesClicked()));
		connect(button, SIGNAL(clicked2()), this, SIGNAL(bracketsClicked()));
		connect(button, SIGNAL(clicked3()), this, SIGNAL(bracketsClicked()));
		grid->addWidget(button, 0, c, 1, 1);
		parButton = button;
	}
	SYMBOL_BUTTON3(QString::fromStdString(CALCULATOR->getComma()), " ", "\n", 3, c)
	button->setToolTip(tr("Argument separator"), tr("Blank space"), tr("New line"));
	commaButton[i] = button;
	c++;
	SYMBOL_OPERATOR_SYMBOL_BUTTON("0", "⁰", "°", 3, c)
	button->setToolTip(QString(), "x<sup>0</sup>", QString::fromStdString(CALCULATOR->getDegUnit()->title(true, settings->printops.use_unicode_signs)));
	f = CALCULATOR->getActiveFunction("inv");
	if(f) {
		SYMBOL_OPERATOR_ITEM_BUTTON("1", "¹", f, 2, c)
		button->setToolTip(QString(), "x<sup>1</sup>", "1/x");
	} else {
		SYMBOL_OPERATOR_SYMBOL_BUTTON("1", "¹", "¹", 2, c)
		button->setToolTip(QString(), "x<sup>1</sup>");
	}
	SYMBOL_OPERATOR_SYMBOL_BUTTON("4", "⁴", "¼", 1, c)
	button->setToolTip(QString(), "x<sup>4</sup>", "1/4");
	SYMBOL_OPERATOR_SYMBOL_BUTTON("7", "⁷", "⅐", 0, c)
	button->setToolTip(QString(), "x<sup>7</sup>", "1/7");
	c++;
	SYMBOL_BUTTON3(QString::fromStdString(CALCULATOR->getDecimalPoint()), " ", "\n", 3, c)
	button->setToolTip(tr("Decimal point"), tr("Blank space"), tr("New line"));
	dotButton[i] = button;
	SYMBOL_OPERATOR_SYMBOL_BUTTON("2", "²", "½", 2, c)
	button->setToolTip(QString(), "x<sup>2</sup>", "1/2");
	SYMBOL_OPERATOR_SYMBOL_BUTTON("5", "⁵", "⅕", 1, c)
	button->setToolTip(QString(), "x<sup>5</sup>", "1/5");
	SYMBOL_OPERATOR_SYMBOL_BUTTON("8", "⁸", "⅛", 0, c)
	button->setToolTip(QString(), "x<sup>8</sup>", "1/8");
	c++;
	SYMBOL_OPERATOR_SYMBOL_BUTTON("3", "³", "⅓", 2, c)
	button->setToolTip(QString(), "x<sup>3</sup>", "1/3");
	SYMBOL_OPERATOR_SYMBOL_BUTTON("6", "⁶", "⅙", 1, c)
	button->setToolTip(QString(), "x<sup>6</sup>", "1/6");
	SYMBOL_OPERATOR_SYMBOL_BUTTON("9", "⁹", "⅑", 0, c)
	button->setToolTip(QString(), "x<sup>9</sup>", "1/9");
	f = CALCULATOR->getActiveFunction("exp10"); f2 = CALCULATOR->getActiveFunction("exp2");
	if(f && f2) {
		OPERATOR_ITEM2_BUTTON("E", f, f2, 3, c);
		button->setToolTip("10<sup>x</sup>", QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(f2->title(true, settings->printops.use_unicode_signs)));
	} else {
		OPERATOR_BUTTON("E", 3, c);
		button->setToolTip("10<sup>x</sup>");
	}
	button->setText("EXP");
	c++;
	ITEM_BUTTON(settings->vans[0], "ANS", 3, c);
	button = new KeypadButton("ANS", this);
	button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) settings->vans[0])); \
	button->setToolTip(QString::fromStdString(settings->vans[0]->title(true, settings->printops.use_unicode_signs)), tr("Previous result (static)"));
	connect(button, SIGNAL(clicked()), this, SLOT(onItemButtonClicked()));
	connect(button, SIGNAL(clicked2()), this, SIGNAL(answerClicked()));
	connect(button, SIGNAL(clicked3()), this, SIGNAL(answerClicked()));
	grid->addWidget(button, 3, c, 1, 1);
	OPERATOR_BUTTON3(settings->multiplicationSign(), "&", "<<", 1, c);
	button->setToolTip(tr("Multiplication"), tr("Bitwise AND"), tr("Bitwise Shift"));
	multiplicationButton[i] = button;
	button = new KeypadButton(LOAD_ICON("edit-delete"), this, true);
	connect(button, SIGNAL(clicked()), this, SIGNAL(delClicked()));
	connect(button, SIGNAL(clicked2()), this, SIGNAL(backspaceClicked()));
	connect(button, SIGNAL(clicked3()), this, SIGNAL(backspaceClicked()));
	button->setToolTip(tr("Delete"), tr("Backspace"));
	grid->addWidget(button, 0, c, 1, 1);
	delButton[i] = button;
	OPERATOR_SYMBOL_BUTTON("+", "+", 2, c);
	button->setToolTip(tr("Addition"), tr("Plus"));
	c++;
	f = CALCULATOR->getActiveFunction("neg");
	if(f) {
		OPERATOR_ITEM_SYMBOL_BUTTON(SIGN_MINUS, f, SIGN_MINUS, 2, c);
		button->setToolTip(tr("Subtraction"), QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)) + " (" + QKeySequence(Qt::CTRL | Qt::Key_Minus).toString(QKeySequence::NativeText) + ")", tr("Minus"));
	} else {
		OPERATOR_SYMBOL_BUTTON(SIGN_MINUS, SIGN_MINUS, 2, c);
		button->setToolTip(tr("Subtraction"), tr("Minus"));
	}
	OPERATOR_BUTTON3(settings->divisionSign(), "|", "~", 1, c);
	divisionButton[i] = button;
	button->setProperty(BUTTON_DATA, settings->divisionSign(false));
	button->setToolTip(tr("Division"), tr("Bitwise OR"), tr("Bitwise NOT"));
	button = new KeypadButton(LOAD_ICON("edit-clear"), this);
	button->setToolTip(tr("Clear expression"));
	connect(button, SIGNAL(clicked()), this, SIGNAL(clearClicked()));
	connect(button, SIGNAL(clicked2()), this, SIGNAL(clearClicked()));
	connect(button, SIGNAL(clicked3()), this, SIGNAL(clearClicked()));
	grid->addWidget(button, 0, c, 1, 1);
	acButton[i] = button;
	button = new KeypadButton("=", this);
	button->setToolTip(tr("Calculate expression"), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_SOLVE)->title(true, settings->printops.use_unicode_signs)));
	button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) CALCULATOR->getFunctionById(FUNCTION_ID_SOLVE))); \
	connect(button, SIGNAL(clicked()), this, SIGNAL(equalsClicked()));
	connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked()));
	connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked()));
	grid->addWidget(button, 3, c, 1, 1);
	for(c = 0; c < 6; c++) grid->setColumnStretch(c, 1);
	for(int r = 0; r < 4; r++) grid->setRowStretch(r, 1);
}


KeypadWidget::KeypadWidget(QWidget *parent) : QWidget(parent) {

	QHBoxLayout *box = new QHBoxLayout(this);
	leftStack = new QStackedLayout();

	box->addLayout(leftStack, 11);
	box->addSpacing(box->spacing());

	numpad = new QWidget(this);
	if(settings->hide_numpad) numpad->hide();
	box->addWidget(numpad, 12);
	keypadG = new QWidget(this);
	leftStack->addWidget(keypadG);
	keypadP = new QWidget(this);
	leftStack->addWidget(keypadP);
	keypadX = new QWidget(this);
	leftStack->addWidget(keypadX);
	keypadC = new QWidget(this);
	leftStack->addWidget(keypadC);
	keypadN = new QWidget(this);
	leftStack->addWidget(keypadN);

	sinButton = NULL;
	cosButton = NULL;
	tanButton = NULL;
	delButton[0] = NULL;
	delButton[1] = NULL;
	acButton[0] = NULL;
	acButton[1] = NULL;
	backButton = NULL;
	forwardButton = NULL;
	dotButton[0] = NULL;
	commaButton[0] = NULL;
	multiplicationButton[0] = NULL;
	divisionButton[0] = NULL;
	dotButton[1] = NULL;
	commaButton[1] = NULL;
	multiplicationButton[1] = NULL;
	divisionButton[1] = NULL;
	imaginaryButton = NULL;
	binButton = NULL;
	octButton = NULL;
	decButton = NULL;
	hexButton = NULL;
	aButton = NULL;
	bButton = NULL;
	cButton = NULL;
	dButton = NULL;
	eButton = NULL;
	fButton = NULL;
	unitButton = NULL;
	storeButton = NULL;
	customOKButton = NULL;
	customEditButton = NULL;
	xButton = NULL;
	percentButton = NULL;
	parButton = NULL;

	updateStretch();

	setKeypadType(settings->keypad_type);

}
KeypadWidget::~KeypadWidget() {}

void KeypadWidget::updateSinMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SIN))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SINH))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ASIN))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ASINH))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getActiveFunction("csc"))
		MENU_ITEM(CALCULATOR->getActiveFunction("csch"))
		MENU_ITEM(CALCULATOR->getActiveFunction("arccsc"))
		MENU_ITEM(CALCULATOR->getActiveFunction("arcsch"))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SINC))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SININT))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SINHINT))
	}
}
void KeypadWidget::updateCosMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_COS))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_COSH))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ACOS))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ACOSH))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getActiveFunction("sec"))
		MENU_ITEM(CALCULATOR->getActiveFunction("sech"))
		MENU_ITEM(CALCULATOR->getActiveFunction("arcsec"))
		MENU_ITEM(CALCULATOR->getActiveFunction("arsech"))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_COSINT))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_COSHINT))
	}
}
void KeypadWidget::updateTanMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_TAN))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_TANH))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ATANH))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getActiveFunction("cot"))
		MENU_ITEM(CALCULATOR->getActiveFunction("coth"))
		MENU_ITEM(CALCULATOR->getActiveFunction("arccot"))
		MENU_ITEM(CALCULATOR->getActiveFunction("arcoth"))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ATAN2))
	}
}
void KeypadWidget::updatePiMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	QAction *action;
	if(menu->isEmpty()) {
		QMap<QString, ExpressionItem*> map;
		Variable *v[9];
		v[0] = CALCULATOR->getActiveVariable("apery");
		v[1] = CALCULATOR->getVariableById(VARIABLE_ID_CATALAN);
		v[2] = CALCULATOR->getVariableById(VARIABLE_ID_EULER);
		v[3] = CALCULATOR->getActiveVariable("golden");
		v[4] = CALCULATOR->getActiveVariable("omega");
		v[5] = CALCULATOR->getActiveVariable("plastic");
		v[6] = CALCULATOR->getActiveVariable("pythagoras");
		for(size_t i = 0; i < 7; i++) {
			if(v[i]) map[QString::fromStdString(v[i]->title(true, settings->printops.use_unicode_signs))] = v[i];
		}
		for(QMap<QString, ExpressionItem*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
			action = menu->addAction(it.key(), this, SLOT(onItemButtonClicked()));
			action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) it.value()));
		}
		menu->addSeparator();
		map.clear();
		v[0] = CALCULATOR->getActiveVariable("c");
		v[1] = CALCULATOR->getActiveVariable("newtonian_constant");
		v[2] = CALCULATOR->getActiveVariable("planck");
		v[3] = CALCULATOR->getActiveVariable("boltzmann");
		v[4] = CALCULATOR->getActiveVariable("avogadro");
		v[5] = CALCULATOR->getActiveVariable("magnetic_constant");
		v[6] = CALCULATOR->getActiveVariable("electric_constant");
		v[7] = CALCULATOR->getActiveVariable("characteristic_impedance");
		v[8] = CALCULATOR->getActiveVariable("standard_gravity");
		for(size_t i = 0; i < 9; i++) {
			if(v[i]) map[QString::fromStdString(v[i]->title(true, settings->printops.use_unicode_signs))] = v[i];
		}
		for(QMap<QString, ExpressionItem*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
			action = menu->addAction(it.key(), this, SLOT(onItemButtonClicked()));
			action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) it.value()));
		}
		menu->addSeparator();
		action = menu->addAction(tr("Enable units in physical constants"), this, SLOT(variableUnitsActivated())); action->setCheckable(true); action->setObjectName("action_variable_units");
		menu->addSeparator();
		menu->addAction(tr("All constants"), this, SIGNAL(openVariablesRequest()));
	}
	action = findChild<QAction*>("action_variable_units");
	if(action) action->setChecked(CALCULATOR->variableUnitsEnabled());
}
void KeypadWidget::updateLnMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getActiveFunction("log10"))
		MENU_ITEM(CALCULATOR->getActiveFunction("log2"))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_LOGN))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_LOGINT))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_POLYLOG))
	}
}
void KeypadWidget::updateSqrtMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_CBRT))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ROOT))
		MENU_ITEM(CALCULATOR->getActiveFunction("sqrtpi"))
	}
}
void KeypadWidget::updatePowerMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SQUARE))
		MENU_ITEM(CALCULATOR->getActiveFunction("exp10"))
		MENU_ITEM(CALCULATOR->getActiveFunction("exp2"))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_LAMBERT_W))
	}
}
void KeypadWidget::updateEMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_EXP))
		MENU_ITEM(CALCULATOR->getActiveFunction("cis"))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_EXPINT))
	}
}
void KeypadWidget::updateFactorialMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_GAMMA))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_DOUBLE_FACTORIAL))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_MULTI_FACTORIAL))
		MENU_ITEM(CALCULATOR->getActiveFunction("hyperfactorial"))
		MENU_ITEM(CALCULATOR->getActiveFunction("superfactorial"))
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getActiveFunction("perm"))
		MENU_ITEM(CALCULATOR->getActiveFunction("comb"))
		MENU_ITEM(CALCULATOR->getActiveFunction("derangement"))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_BINOMIAL))
	}
}
void KeypadWidget::updateSumMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_PRODUCT))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_FOR))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_IF))
	}
}
void KeypadWidget::updateXMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	menu->clear();
	QAction *action;
	QMap<QString, ExpressionItem*> map;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->isKnown() && !CALCULATOR->variables[i]->isHidden() && CALCULATOR->variables[i]->isActive() && CALCULATOR->variables[i] != CALCULATOR->getVariableById(VARIABLE_ID_X)) {
			map[QString::fromStdString(CALCULATOR->variables[i]->title(true, settings->printops.use_unicode_signs))] = CALCULATOR->variables[i];
		}
	}
	for(QMap<QString, ExpressionItem*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
		action = menu->addAction(it.key(), this, SLOT(onItemButtonClicked()));
		action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) it.value()));
	}
}
void KeypadWidget::updateEqualsMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_SYMBOL(SIGN_NOT_EQUAL);
		MENU_SYMBOL("<");
		MENU_SYMBOL(SIGN_LESS_OR_EQUAL);
		MENU_SYMBOL(">");
		MENU_SYMBOL(SIGN_GREATER_OR_EQUAL);
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SOLVE))
		MENU_ITEM(CALCULATOR->getActiveFunction("solve2"))
		MENU_ITEM(CALCULATOR->getActiveFunction("linearfunction"))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_D_SOLVE))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_NEWTON_RAPHSON))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_SECANT_METHOD))
	}
}
void KeypadWidget::updateFactorizeMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		menu->addAction(tr("Expand"), this, SIGNAL(expandClicked()));
		menu->addAction(tr("Expand partial fractions"), this, SIGNAL(expandPartialFractionsClicked()));
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_DIFFERENTIATE))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_INTEGRATE))
	}
}
void KeypadWidget::updatePercentageMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	if(menu->isEmpty()) {
		ExpressionItem *item;
		QAction *action;
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_MOD));
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_REM));
		MENU_ITEM(CALCULATOR->getFunction("div"));
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ABS));
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_GCD));
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_LCM));
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_DIVISORS));
		menu->addSeparator();
		MENU_ITEM(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE));
		MENU_ITEM(CALCULATOR->getVariableById(VARIABLE_ID_PERMYRIAD));
		menu->addSeparator();
		action = menu->addAction(tr("Percentage Calculation Tool"), this, SIGNAL(openPercentageCalculationRequest()));
	}
}
void KeypadWidget::updateUnitsMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	menu->clear();
	ExpressionItem *item;
	QAction *action;
	QMap<QString, ExpressionItem*> map, map2;
	const char *si_units[] = {"m", "g", "s", "A", "K", "J", "W", "L", "V", "ohm", "N", "Pa", "C", "F", "S", "oC", "Hz", "cd", "mol", "Wb", "T", "H", "lm", "lx", "Bq", "Gy", "Sv", "kat"};
	size_t n = 0;
	for(size_t i = 0; i < 5 && i < settings->recent_units.size(); i++) {
		item = settings->recent_units[i];
		action = menu->addAction(QString::fromStdString(item->title(true, settings->printops.use_unicode_signs)), this, SLOT(onUnitItemClicked()));
		action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) item));
		n++;
	}
	if(n > 0) menu->addSeparator();
	for(size_t i = 0; i < 28; i++) {
		item = CALCULATOR->getActiveUnit(si_units[i]);
		for(size_t i = 0; item && i < 5 && i < settings->recent_units.size(); i++) {
			if(item == settings->recent_units[i]) {
				item = NULL;
				break;
			}
		}
		if(item) {
			if(n >= 15) map2[QString::fromStdString(item->title(true, settings->printops.use_unicode_signs))] = item;
			else map[QString::fromStdString(item->title(true, settings->printops.use_unicode_signs))] = item;
			n++;
		}
	}
	for(QMap<QString, ExpressionItem*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
		action = menu->addAction(it.key(), this, SLOT(onUnitItemClicked()));
		action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) it.value()));
	}
	if(!map2.isEmpty()) {
		QMenu *menu2 = menu->addMenu(tr("more"));
		for(QMap<QString, ExpressionItem*>::const_iterator it = map2.constBegin(); it != map2.constEnd(); ++it) {
			action = menu2->addAction(it.key(), this, SLOT(onUnitItemClicked()));
			action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) it.value()));
		}
	}
	menu->addSeparator();
	for(int i = -9; i <= 12; i += 3) {
		Prefix *p = CALCULATOR->getExactDecimalPrefix(i);
		if(p) {
			action = menu->addAction(QString::fromStdString(p->longName(true, true)), this, SLOT(onPrefixItemClicked()));
			action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) p));
		}
	}
	QMenu *menu2 = menu->addMenu(tr("more"));
	for(int i = -30; i <= 30; i += 3) {
		if(i == -9) i = -2;
		else if(i == 1) i = -1;
		else if(i == 2) i = 1;
		else if(i == 4) i = 2;
		else if(i == 5) i = 15;
		Prefix *p = CALCULATOR->getExactDecimalPrefix(i);
		if(p) {
			action = menu2->addAction(QString::fromStdString(p->longName(true, true)), this, SLOT(onPrefixItemClicked()));
			action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) p));
		}
	}
	menu->addSeparator();
	menu->addAction(tr("All units"), this, SIGNAL(openUnitsRequest()));
}
void KeypadWidget::updateStoreMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	menu->clear();
	QAction *action;
	QMap<QString, ExpressionItem*> map;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(CALCULATOR->variables[i]->isLocal()) {
			map[QString::fromStdString(CALCULATOR->variables[i]->title(true, settings->printops.use_unicode_signs))] = CALCULATOR->variables[i];
		}
	}
	for(QMap<QString, ExpressionItem*>::const_iterator it = map.constBegin(); it != map.constEnd(); ++it) {
		action = menu->addAction(it.key(), this, SLOT(onItemButtonClicked()));
		action->setProperty(BUTTON_DATA, QVariant::fromValue((void*) it.value()));
	}
	if(!map.isEmpty()) menu->addSeparator();
	menu->addAction(tr("All variables"), this, SIGNAL(openVariablesRequest()));
}
void KeypadWidget::updateCustomActionOK() {
	QTreeWidgetItem *item = actionList->currentItem();
	customOKButton->setEnabled(item && (item->data(0, Qt::UserRole).toInt() < 0 || ((!labelEdit || !labelEdit->text().trimmed().isEmpty()) && (!SHORTCUT_REQUIRES_VALUE(item->data(0, Qt::UserRole).toInt()) || !valueEdit->currentText().isEmpty()))));
}
void KeypadWidget::customActionOKClicked() {
	QString value = valueEdit->currentText();
	QTreeWidgetItem *item = actionList->currentItem();
	if(!item) return;
	if(settings->testShortcutValue(item->data(0, Qt::UserRole).toInt(), value, customActionDialog)) {
		customActionDialog->accept();
	} else {
		valueEdit->setFocus();
	}
	valueEdit->setCurrentText(value);
}
void KeypadWidget::currentCustomActionChanged(QTreeWidgetItem *item, QTreeWidgetItem *item_prev) {
	if(!item || !SHORTCUT_USES_VALUE(item->data(0, Qt::UserRole).toInt())) {
		valueEdit->clear();
		valueEdit->clearEditText();
		valueEdit->setEnabled(false);
		valueLabel->setEnabled(false);
		return;
	}
	int i = item->data(0, Qt::UserRole).toInt();
	int i_prev = -1;
	if(item_prev) i_prev = item_prev->data(0, Qt::UserRole).toInt();
	valueEdit->setEnabled(true);
	valueLabel->setEnabled(true);
	if(i == SHORTCUT_TYPE_FUNCTION || i == SHORTCUT_TYPE_FUNCTION_WITH_DIALOG) {
		if(i_prev != SHORTCUT_TYPE_FUNCTION && i_prev != SHORTCUT_TYPE_FUNCTION_WITH_DIALOG) {
			valueEdit->clear();
			QStringList citems;
			for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
				MathFunction *f = CALCULATOR->functions[i];
				if(f->isActive() && !f->isHidden()) citems << QString::fromStdString(f->referenceName());
			}
			citems.sort(Qt::CaseInsensitive);
			valueEdit->addItems(citems);
			valueEdit->clearEditText();
		}
	} else {
		valueEdit->clear();
		if(i == SHORTCUT_TYPE_UNIT || i == SHORTCUT_TYPE_CONVERT_TO) {
			QStringList citems;
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				Unit *u = CALCULATOR->units[i];
				if(u->isActive() && !u->isHidden()) citems << QString::fromStdString(u->referenceName());
			}
			citems.sort(Qt::CaseInsensitive);
			valueEdit->addItems(citems);
		} else if(i == SHORTCUT_TYPE_VARIABLE) {
			QStringList citems;
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				Variable *v = CALCULATOR->variables[i];
				if(v->isActive() && !v->isHidden()) citems << QString::fromStdString(v->referenceName());
			}
			citems.sort(Qt::CaseInsensitive);
			valueEdit->addItems(citems);
		} else if(i == SHORTCUT_TYPE_OPERATOR) {
			QStringList citems;
			citems << "+" << (settings->printops.use_unicode_signs ? SIGN_MINUS : "-") << settings->multiplicationSign(false) << settings->divisionSign(false) << "^" << ".+" << (QStringLiteral(".") + (settings->printops.use_unicode_signs ? SIGN_MINUS : "-")) << (QStringLiteral(".") + settings->multiplicationSign(false)) << (QStringLiteral(".") + settings->divisionSign(false)) << ".^" << "mod" << "rem" << "//" << "&" << "|" << "<<" << ">>" << "&&" << "||" << "xor" << "=" << SIGN_NOT_EQUAL << "<" << SIGN_LESS_OR_EQUAL << SIGN_GREATER_OR_EQUAL << ">";
			valueEdit->addItems(citems);
		} else if(i == SHORTCUT_TYPE_COPY_RESULT) {
			settings->updateActionValueTexts();
			valueEdit->addItems(settings->copy_action_value_texts);
		}
		if(i == SHORTCUT_TYPE_COPY_RESULT) valueEdit->setCurrentText(settings->copy_action_value_texts[0]);
		else valueEdit->clearEditText();
	}
}

void KeypadWidget::editCustomAction(KeypadButton *button, int i) {
	if(i < 1 || i > 3) return;
	QDialog *dialog = new QDialog(this);
	customActionDialog = dialog;
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Button Action"));
	QVBoxLayout *box = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	if(i == 1) {
		grid->addWidget(new QLabel(tr("Label:"), dialog), 0, 0);
		labelEdit = new QLineEdit(dialog);
		labelEdit->setText(button->richText().isEmpty() ? button->text() : button->richText());
		grid->addWidget(labelEdit, 0, 1);
	} else {
		labelEdit = NULL;
	}
	actionList = new QTreeWidget(dialog);
	actionList->setColumnCount(1);
	actionList->setHeaderLabel(tr("Action"));
	actionList->setSelectionMode(QAbstractItemView::SingleSelection);
	actionList->setRootIsDecorated(false);
	actionList->header()->setVisible(true);
	actionList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	actionList->setSortingEnabled(true);
	actionList->sortByColumn(-1, Qt::AscendingOrder);
	grid->addWidget(actionList, i != 1 ? 0 : 1, 0, 1, 2);
	int type = -1;
	if(button->property(i == 2 ? BUTTON_DATA2 : (i == 3 ? BUTTON_DATA3 : BUTTON_DATA)).isValid()) type = button->property(i == 2 ? BUTTON_DATA2 : (i == 3 ? BUTTON_DATA3 : BUTTON_DATA)).toInt();
	QTreeWidgetItem *item = new QTreeWidgetItem(actionList, QStringList(tr("None")));
	item->setData(0, Qt::UserRole, -1);
	actionList->setCurrentItem(item);
	for(int i = SHORTCUT_TYPE_FUNCTION; i <= SHORTCUT_TYPE_QUIT; i++) {
		item = new QTreeWidgetItem(actionList, QStringList(settings->shortcutTypeText((shortcut_type) i)));
		item->setData(0, Qt::UserRole, i);
		if(i == type) actionList->setCurrentItem(item);
		if(i == SHORTCUT_TYPE_HISTORY_SEARCH) {
			item = new QTreeWidgetItem(actionList, QStringList(settings->shortcutTypeText((shortcut_type) SHORTCUT_TYPE_HISTORY_CLEAR)));
			item->setData(0, Qt::UserRole, SHORTCUT_TYPE_HISTORY_CLEAR);
			if(type == SHORTCUT_TYPE_HISTORY_CLEAR) actionList->setCurrentItem(item);
		} else if(i == SHORTCUT_TYPE_APPROXIMATE) {
			item = new QTreeWidgetItem(actionList, QStringList(settings->shortcutTypeText((shortcut_type) SHORTCUT_TYPE_TOGGLE_FRACTION)));
			item->setData(0, Qt::UserRole, SHORTCUT_TYPE_TOGGLE_FRACTION);
			if(type == SHORTCUT_TYPE_TOGGLE_FRACTION) actionList->setCurrentItem(item);
		}
	}
	valueLabel = new QLabel(tr("Value:"), dialog);
	grid->addWidget(valueLabel, i != 1 ? 1 : 2, 0);
	valueEdit = new QComboBox(dialog);
	valueEdit->setEditable(true);
	valueEdit->setLineEdit(new MathLineEdit());
	grid->addWidget(valueEdit, i != 1 ? 1 : 2, 1);
	grid->setColumnStretch(1, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(customActionOKClicked()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	if(labelEdit) connect(labelEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateCustomActionOK()));
	connect(actionList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(updateCustomActionOK()));
	connect(actionList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(currentCustomActionChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(valueEdit, SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateCustomActionOK()));
	customOKButton = buttonBox->button(QDialogButtonBox::Ok);
	currentCustomActionChanged(actionList->currentItem(), NULL);
	if(button->property(i == 2 ? BUTTON_VALUE2 : (i == 3 ? BUTTON_VALUE3 : BUTTON_VALUE)).isValid()) {
		QString value = button->property(i == 2 ? BUTTON_VALUE2 : (i == 3 ? BUTTON_VALUE3 : BUTTON_VALUE)).toString();
		if(type == SHORTCUT_TYPE_COPY_RESULT) {
			int v = value.toInt();
			if(v >= 0 && v < settings->copy_action_value_texts.size()) valueEdit->setCurrentText(settings->copy_action_value_texts[v]);
		} else {
			valueEdit->setCurrentText(value);
		}
	}
	customOKButton->setEnabled(false);
	if(labelEdit) labelEdit->setFocus();
	else actionList->setFocus();
	dialog->resize(dialog->sizeHint().width(), dialog->sizeHint().width() * 1.25);
	if(dialog->exec() == QDialog::Accepted) {
		int grid_i = customGrid->indexOf(button);
		int c = 0, r = 0, cs = 0, rs = 0;
		customGrid->getItemPosition(grid_i, &r, &c, &rs, &cs);
		c++; r++;
		size_t index = 0;
		for(size_t i = settings->custom_buttons.size(); i > 0; i--) {
			if(settings->custom_buttons[i - 1].r == r && settings->custom_buttons[i - 1].c == c) {
				index = i;
				break;
			}
		}
		if(index == 0) {
			custom_button cb;
			cb.c = c; cb.r = r; cb.type[0] = -1; cb.type[1] = -1; cb.type[2] = -1;
			settings->custom_buttons.push_back(cb);
			index = settings->custom_buttons.size();
		}
		index--;
		custom_button *cb = &settings->custom_buttons[index];
		cb->type[i - 1] = actionList->currentItem()->data(0, Qt::UserRole).toInt();
		cb->value[i - 1] = valueEdit->currentText().toStdString();
		button->setProperty(i == 2 ? BUTTON_DATA2 : (i == 3 ? BUTTON_DATA3 : BUTTON_DATA), actionList->currentItem()->data(0, Qt::UserRole).toInt());
		button->setProperty(i == 2 ? BUTTON_VALUE2 : (i == 3 ? BUTTON_VALUE3 : BUTTON_VALUE), valueEdit->currentText());
		button->setToolTip(settings->shortcutText(cb->type[0], cb->value[0]), settings->shortcutText(cb->type[1], cb->value[1]), settings->shortcutText(cb->type[2], cb->value[2]));
		if(labelEdit) {
			settings->custom_buttons[index].label = labelEdit->text().trimmed();
			if(labelEdit->text().contains("</")) button->setRichText(labelEdit->text().trimmed());
			else button->setText(labelEdit->text());
		}
	}
	dialog->deleteLater();
}
void KeypadWidget::onCustomEditClicked(bool b) {
	b_edit = b;
}
void KeypadWidget::addCustomRow() {
	settings->custom_button_rows++;
	int r = settings->custom_button_rows - 1;
	for(int c = 0; c < settings->custom_button_columns; c++) {
		customButtons[c].resize(settings->custom_button_rows);
		KeypadButton *button = new KeypadButton(QString(), this);
		connect(button, SIGNAL(clicked()), this, SLOT(onCustomButtonClicked()));
		connect(button, SIGNAL(clicked2()), this, SLOT(onCustomButtonClicked2()));
		connect(button, SIGNAL(clicked3()), this, SLOT(onCustomButtonClicked3()));
		customGrid->addWidget(button, r, c);
		customButtons[c][r] = button;
		customGrid->setRowStretch(r, 1);
	}
	addRowAction->setEnabled(settings->custom_button_rows < 100);
	removeRowAction->setEnabled(true);
}
void KeypadWidget::addCustomColumn() {
	settings->custom_button_columns++;
	customButtons.resize(settings->custom_button_columns);
	int c = settings->custom_button_columns - 1;
	customButtons[c].resize(settings->custom_button_rows);
	for(int r = 0; r < settings->custom_button_rows; r++) {
		KeypadButton *button = new KeypadButton(QString(), this);
		connect(button, SIGNAL(clicked()), this, SLOT(onCustomButtonClicked()));
		connect(button, SIGNAL(clicked2()), this, SLOT(onCustomButtonClicked2()));
		connect(button, SIGNAL(clicked3()), this, SLOT(onCustomButtonClicked3()));
		customGrid->addWidget(button, r, c);
		customButtons[c][r] = button;
		customGrid->setRowStretch(r, 1);
	}
	customGrid->setColumnStretch(c, 1);
	updateStretch();
	addColumnAction->setEnabled(settings->custom_button_columns < 100);
	removeColumnAction->setEnabled(true);
}
void KeypadWidget::removeCustomRow() {
	if(settings->custom_button_rows <= 1) return;
	settings->custom_button_rows--;
	int r = settings->custom_button_rows;
	customGrid->setRowStretch(r, 0);
	for(int c = 0; c < settings->custom_button_columns; c++) {
		for(size_t i = settings->custom_buttons.size(); i > 0; i--) {
			if(settings->custom_buttons[i - 1].r == r + 1 && settings->custom_buttons[i - 1].c == c + 1) {
				settings->custom_buttons.erase(settings->custom_buttons.begin() + (i - 1));
				break;
			}
		}
		customGrid->removeWidget(customButtons[c][r]);
		customButtons[c][r]->deleteLater();
		customButtons[c].pop_back();
	}
	addRowAction->setEnabled(true);
	removeRowAction->setEnabled(settings->custom_button_rows > 1);
}
void KeypadWidget::removeCustomColumn() {
	if(settings->custom_button_columns <= 1) return;
	settings->custom_button_columns--;
	int c = settings->custom_button_columns;
	customGrid->setColumnStretch(c, 0);
	for(int r = 0; r < settings->custom_button_rows; r++) {
		for(size_t i = settings->custom_buttons.size(); i > 0; i--) {
			if(settings->custom_buttons[i - 1].r == r + 1 && settings->custom_buttons[i - 1].c == c + 1) {
				settings->custom_buttons.erase(settings->custom_buttons.begin() + (i - 1));
				break;
			}
		}
		customGrid->removeWidget(customButtons[c][r]);
		customButtons[c][r]->deleteLater();
	}
	customButtons.pop_back();
	updateStretch();
	addColumnAction->setEnabled(true);
	removeColumnAction->setEnabled(settings->custom_button_columns > 1);
}
void KeypadWidget::onCustomButtonClicked() {
	KeypadButton *button = qobject_cast<KeypadButton*>(sender());
	if(b_edit || !sender()->property(BUTTON_DATA).isValid() || sender()->property(BUTTON_DATA).toInt() < 0) {
		editCustomAction(button, 1);
	} else {
		emit shortcutClicked(sender()->property(BUTTON_DATA).toInt(), sender()->property(BUTTON_VALUE).toString());
	}
}
void KeypadWidget::onCustomButtonClicked2() {
	KeypadButton *button = qobject_cast<KeypadButton*>(sender());
	if(b_edit || !sender()->property(BUTTON_DATA2).isValid() || sender()->property(BUTTON_DATA2).toInt() < 0) {
		editCustomAction(button, 2);
	} else {
		emit shortcutClicked(sender()->property(BUTTON_DATA2).toInt(), sender()->property(BUTTON_VALUE2).toString());
	}
}
void KeypadWidget::onCustomButtonClicked3() {
	KeypadButton *button = qobject_cast<KeypadButton*>(sender());
	if(b_edit || !sender()->property(BUTTON_DATA3).isValid() || sender()->property(BUTTON_DATA3).toInt() < 0) {
		editCustomAction(button, 3);
	} else {
		emit shortcutClicked(sender()->property(BUTTON_DATA3).toInt(), sender()->property(BUTTON_VALUE3).toString());
	}
}
void KeypadWidget::intervalDisplayActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	if(v < 0) {
		settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
		settings->adaptive_interval_display = true;
	} else {
		settings->printops.interval_display = (IntervalDisplay) v;
		settings->adaptive_interval_display = false;
	}
	emit resultFormatUpdated(0);
}
void KeypadWidget::intervalCalculationActivated() {
	settings->evalops.interval_calculation = (IntervalCalculation) qobject_cast<QAction*>(sender())->data().toInt();
	emit expressionCalculationUpdated(0);
}
void KeypadWidget::conciseInputActivated() {
	CALCULATOR->setConciseUncertaintyInputEnabled(!CALCULATOR->conciseUncertaintyInputEnabled());
	emit expressionFormatUpdated(false);
}
void KeypadWidget::variableUnitsActivated() {
	CALCULATOR->setVariableUnitsEnabled(!CALCULATOR->variableUnitsEnabled());
	emit expressionCalculationUpdated(0);
}
void KeypadWidget::updateIntervalMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	QAction *action;
	QActionGroup *group;
	QWidgetAction *aw;
	if(menu->isEmpty()) {
		ADD_SECTION(tr("Interval Display"));
		group = new QActionGroup(this); group->setObjectName("group_interval_display");
		action = menu->addAction(tr("Adaptive"), this, SLOT(intervalDisplayActivated())); action->setCheckable(true); group->addAction(action); action->setData(-1);
		action = menu->addAction(tr("Significant digits"), this, SLOT(intervalDisplayActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_DISPLAY_SIGNIFICANT_DIGITS);
		action = menu->addAction(tr("Interval"), this, SLOT(intervalDisplayActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_DISPLAY_INTERVAL);
		action = menu->addAction(tr("Plus/minus"), this, SLOT(intervalDisplayActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_DISPLAY_PLUSMINUS);
		action = menu->addAction(tr("Relative"), this, SLOT(intervalDisplayActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_DISPLAY_RELATIVE);
		action = menu->addAction(tr("Concise"), this, SLOT(intervalDisplayActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_DISPLAY_CONCISE);
		ADD_SECTION(tr("Interval Calculation"));
		group = new QActionGroup(this); group->setObjectName("group_interval_calculation");
		action = menu->addAction(tr("Variance formula"), this, SLOT(intervalCalculationActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_CALCULATION_VARIANCE_FORMULA);
		action = menu->addAction(tr("Interval arithmetic"), this, SLOT(intervalCalculationActivated())); action->setCheckable(true); group->addAction(action); action->setData(INTERVAL_CALCULATION_INTERVAL_ARITHMETIC);
		menu->addSeparator();
		action = menu->addAction(tr("Allow concise uncertainty input"), this, SLOT(conciseInputActivated())); action->setCheckable(true); action->setObjectName("action_concise_input");
	}
	group = findChild<QActionGroup*>("group_interval_display");
	if(!group) return;
	QList<QAction*> actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == (settings->adaptive_interval_display ? -1 : settings->printops.interval_display)) {
			actions.at(i)->setChecked(true);
			break;
		}
	}
	group = findChild<QActionGroup*>("group_interval_calculation");
	if(!group) return;
	actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == settings->evalops.interval_calculation) {
			actions.at(i)->setChecked(true);
			break;
		}
	}
	action = findChild<QAction*>("action_concise_input");
	if(action) action->setChecked(CALCULATOR->conciseUncertaintyInputEnabled());
}
void KeypadWidget::complexFormActivated() {
	settings->evalops.complex_number_form = (ComplexNumberForm) qobject_cast<QAction*>(sender())->data().toInt();
	settings->complex_angle_form = (settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS);
	emit expressionCalculationUpdated(0);
}
void KeypadWidget::updateComplexMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	ExpressionItem *item;
	QAction *action;
	QActionGroup *group;
	if(menu->isEmpty()) {
		MENU_SYMBOL("∠")
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_RE))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_IM))
		MENU_ITEM(CALCULATOR->getFunctionById(FUNCTION_ID_ARG))
		MENU_ITEM(CALCULATOR->getActiveFunction("conj"))
		menu->addSeparator();
		menu = menu->addMenu(tr("Complex number form"));
		group = new QActionGroup(this); group->setObjectName("group_complex_form");
		action = menu->addAction(tr("Rectangular"), this, SLOT(complexFormActivated())); action->setCheckable(true); group->addAction(action); action->setData(COMPLEX_NUMBER_FORM_RECTANGULAR);
		action = menu->addAction(tr("Exponential"), this, SLOT(complexFormActivated())); action->setCheckable(true); group->addAction(action); action->setData(COMPLEX_NUMBER_FORM_EXPONENTIAL);
		action = menu->addAction(tr("Polar"), this, SLOT(complexFormActivated())); action->setCheckable(true); group->addAction(action); action->setData(COMPLEX_NUMBER_FORM_POLAR);
		action = menu->addAction(tr("Angle/phasor"), this, SLOT(complexFormActivated())); action->setCheckable(true); group->addAction(action); action->setData(COMPLEX_NUMBER_FORM_CIS);
	}
	group = findChild<QActionGroup*>("group_complex_form");
	if(!group) return;
	QList<QAction*> actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == settings->evalops.complex_number_form) {
			actions.at(i)->setChecked(true);
			break;
		}
	}
}
void KeypadWidget::updateAssumptions() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	QAction *action;
	QActionGroup *group;
	QString name;
	QWidgetAction *aw;
	int id = menu->property(BUTTON_DATA).toInt();
	if(id == VARIABLE_ID_Y) name = "y";
	else if(id == VARIABLE_ID_Z) name = "z";
	else name = "x";
	if(menu->isEmpty()) {
		ADD_SECTION(tr("Type", "Assumptions type"));
		group = new QActionGroup(this); group->setObjectName("group_type_" + name);
		action = menu->addAction(tr("Number"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_TYPE_NUMBER); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Real"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_TYPE_REAL); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Rational"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_TYPE_RATIONAL); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Integer"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_TYPE_INTEGER); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Boolean"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_TYPE_BOOLEAN); action->setProperty(BUTTON_DATA, id);
		ADD_SECTION(tr("Sign", "Assumptions sign"));
		group = new QActionGroup(this); group->setObjectName("group_sign_" + name);
		action = menu->addAction(tr("Unknown", "Unknown assumptions sign"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_SIGN_UNKNOWN); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Non-zero"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_SIGN_NONZERO); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Positive"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_SIGN_POSITIVE); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Non-negative"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_SIGN_NONNEGATIVE); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Negative"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_SIGN_NEGATIVE); action->setProperty(BUTTON_DATA, id);
		action = menu->addAction(tr("Non-positive"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
		action->setData(ASSUMPTION_SIGN_NONPOSITIVE);
		menu->addSeparator();
		action = menu->addAction(tr("Default assumptions"), this, SLOT(defaultAssumptionsActivated())); action->setProperty(BUTTON_DATA, id);
	}
	Variable *v = CALCULATOR->getVariableById(id);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	group = findChild<QActionGroup*>("group_type_" + name);
	if(!group) return;
	QList<QAction*> actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == ass->type()) {
			actions.at(i)->setChecked(true);
			break;
		}
	}
	group = findChild<QActionGroup*>("group_sign_" + name);
	if(!group) return;
	actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == ass->sign()) {
			actions.at(i)->setChecked(true);
			break;
		}
	}
}
void KeypadWidget::defaultAssumptionsActivated() {
	QAction *action = qobject_cast<QAction*>(sender());
	Variable *v = CALCULATOR->getVariableById(action->property(BUTTON_DATA).toInt());
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	uv->setAssumptions(NULL);
	emit expressionCalculationUpdated(0);
}
void KeypadWidget::assumptionsTypeActivated() {
	QAction *action = qobject_cast<QAction*>(sender());
	Variable *v = CALCULATOR->getVariableById(action->property(BUTTON_DATA).toInt());
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	if(!uv->assumptions()) uv->setAssumptions(new Assumptions());
	uv->assumptions()->setType((AssumptionType) action->data().toInt());
	emit expressionCalculationUpdated(0);
}
void KeypadWidget::assumptionsSignActivated() {
	QAction *action = qobject_cast<QAction*>(sender());
	Variable *v = CALCULATOR->getVariableById(action->property(BUTTON_DATA).toInt());
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	if(!uv->assumptions()) uv->setAssumptions(new Assumptions());
	uv->assumptions()->setSign((AssumptionSign) action->data().toInt());
	emit expressionCalculationUpdated(0);
}
void KeypadWidget::setKeypadType(int i) {
	if(i < 0 || i > KEYPAD_NUMBERPAD) i = 0;
	createKeypad(i);
	hideNumpad(settings->hide_numpad);
	if(leftStack->currentIndex() == KEYPAD_PROGRAMMING && settings->programming_base_changed) {
		settings->programming_base_changed = false;
		emit baseClicked(BASE_DECIMAL, true, false);
	}
	leftStack->setCurrentIndex(i);
}
void KeypadWidget::hideNumpad(bool b) {
	if(!b) createNumpad(numpad, 0);
	numpad->setVisible(!b);
}
void KeypadWidget::updateStretch() {
	int left_size = 5;
	if(settings->separate_keypad_menu_buttons) left_size++;
	if(settings->custom_button_columns > left_size) left_size = settings->custom_button_columns;
	((QBoxLayout*) layout())->setStretchFactor(leftStack, (left_size * 2) + 1);
	((QBoxLayout*) layout())->setStretchFactor(numpad, 12);
}
void KeypadWidget::showSeparateKeypadMenuButtons(bool b) {
	QList<KeypadButton*> buttons = findChildren<KeypadButton*>();
	for(int i = 0; i < buttons.count(); i++) {
		if(buttons.at(i) != customEditButton && buttons.at(i)->menu()) {
			buttons.at(i)->setPopupMode(b ? QToolButton::MenuButtonPopup : QToolButton::DelayedPopup);
		}
		buttons.at(i)->updateSize();
	}
	updateStretch();
	QRect r = geometry(); r.moveTo(0, 0); repaint(r);
}
void KeypadWidget::updateBase() {
	if(!binButton) return;
	binButton->setChecked(settings->printops.base == 2 && settings->evalops.parse_options.base == 2);
	octButton->setChecked(settings->printops.base == 8 && settings->evalops.parse_options.base == 8);
	decButton->setChecked(settings->printops.base == 10 && settings->evalops.parse_options.base == 10);
	hexButton->setChecked(settings->printops.base == 16 && settings->evalops.parse_options.base == 16);
	int base = settings->evalops.parse_options.base;
	if(base == BASE_CUSTOM) base = CALCULATOR->customOutputBase().intValue();
	else if(base < 2 || base > 36) base = 10;
	aButton->setEnabled(base > 10);
	bButton->setEnabled(base > 11);
	cButton->setEnabled(base > 12);
	dButton->setEnabled(base > 13);
	eButton->setEnabled(base > 14);
	fButton->setEnabled(base > 15);
}
void KeypadWidget::updateSymbols() {
	for(size_t i = 0; i < 2; i++) {
		if(!dotButton[i]) continue;
		multiplicationButton[i]->setText(settings->multiplicationSign());
		multiplicationButton[i]->setText(settings->multiplicationSign());
		multiplicationButton[i]->setProperty(BUTTON_DATA, settings->multiplicationSign());
		divisionButton[i]->setText(settings->divisionSign());
		divisionButton[i]->setProperty(BUTTON_DATA, settings->divisionSign(false));
		commaButton[i]->setText(QString::fromStdString(CALCULATOR->getComma()));
		commaButton[i]->setProperty(BUTTON_DATA, QString::fromStdString(CALCULATOR->getComma()));
		dotButton[i]->setText(QString::fromStdString(CALCULATOR->getDecimalPoint()));
		dotButton[i]->setProperty(BUTTON_DATA, QString::fromStdString(CALCULATOR->getDecimalPoint()));
	}
	if(imaginaryButton) imaginaryButton->setText(CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0 ? "j" : "i");
}
void KeypadWidget::changeEvent(QEvent *e) {
	if(e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) {
		for(size_t i = 0; i < 2; i++) {
			if(!acButton[i]) continue;
			acButton[i]->setIcon(LOAD_ICON("edit-clear"));
			delButton[i]->setIcon(LOAD_ICON("edit-delete"));
		}
		if(backButton) backButton->setIcon(LOAD_ICON("go-back"));
		if(forwardButton) forwardButton->setIcon(LOAD_ICON("go-forward"));
		if(customEditButton) customEditButton->setIcon(LOAD_ICON("document-edit"));
	} else if(e->type() == QEvent::FontChange || e->type() == QEvent::ApplicationFontChange) {
		QList<KeypadButton*> buttons = findChildren<KeypadButton*>();
		for(int i = 0; i < buttons.count(); i++) {
			buttons.at(i)->updateSize();
		}
	}
	QWidget::changeEvent(e);
}
void KeypadWidget::onHypToggled(bool b) {
	if(b) {
		SET_ITEM_BUTTON2(sinButton, CALCULATOR->getFunctionById(FUNCTION_ID_SINH), CALCULATOR->getFunctionById(FUNCTION_ID_ASINH));
		SET_ITEM_BUTTON2(cosButton, CALCULATOR->getFunctionById(FUNCTION_ID_COSH), CALCULATOR->getFunctionById(FUNCTION_ID_ACOSH));
		SET_ITEM_BUTTON2(tanButton, CALCULATOR->getFunctionById(FUNCTION_ID_TANH), CALCULATOR->getFunctionById(FUNCTION_ID_ATANH));
	} else {
		SET_ITEM_BUTTON2(sinButton, CALCULATOR->getFunctionById(FUNCTION_ID_SIN), CALCULATOR->getFunctionById(FUNCTION_ID_ASIN));
		SET_ITEM_BUTTON2(cosButton, CALCULATOR->getFunctionById(FUNCTION_ID_COS), CALCULATOR->getFunctionById(FUNCTION_ID_ACOS));
		SET_ITEM_BUTTON2(tanButton, CALCULATOR->getFunctionById(FUNCTION_ID_TAN), CALCULATOR->getFunctionById(FUNCTION_ID_ATAN));
	}
}
void KeypadWidget::onSymbolButtonClicked() {
	emit symbolClicked(sender()->property(BUTTON_DATA).toString());
}
void KeypadWidget::onSymbolButtonClicked2() {
	emit symbolClicked(sender()->property(BUTTON_DATA2).toString());
}
void KeypadWidget::onSymbolButtonClicked3() {
	emit symbolClicked(sender()->property(BUTTON_DATA3).toString());
}
void KeypadWidget::onOperatorButtonClicked() {
	emit operatorClicked(sender()->property(BUTTON_DATA).toString());
}
void KeypadWidget::onOperatorButtonClicked2() {
	emit operatorClicked(sender()->property(BUTTON_DATA2).toString());
}
void KeypadWidget::onOperatorButtonClicked3() {
	emit operatorClicked(sender()->property(BUTTON_DATA3).toString());
}
void KeypadWidget::onBaseButtonClicked() {
	settings->programming_base_changed = settings->programming_base_changed || (settings->printops.base == BASE_DECIMAL && settings->evalops.parse_options.base == BASE_DECIMAL);
	emit baseClicked(sender()->property(BUTTON_DATA).toInt(), true, true);
}
void KeypadWidget::onBaseButtonClicked2() {
	settings->programming_base_changed = settings->programming_base_changed || (settings->printops.base == BASE_DECIMAL && settings->evalops.parse_options.base == BASE_DECIMAL);
	emit baseClicked(sender()->property(BUTTON_DATA).toInt(), false, true);
}
void KeypadWidget::onItemButtonClicked() {
	ExpressionItem *item = (ExpressionItem*) sender()->property(BUTTON_DATA).value<void*>();
	if(item->type() == TYPE_FUNCTION) emit functionClicked((MathFunction*) item);
	else if(item->type() == TYPE_VARIABLE) emit variableClicked((Variable*) item);
	else if(item->type() == TYPE_UNIT) emit unitClicked((Unit*) item);
}
void KeypadWidget::onPrefixItemClicked() {
	emit prefixClicked((Prefix*) sender()->property(BUTTON_DATA).value<void*>());
}
void KeypadWidget::onUnitItemClicked() {
	Unit *u = (Unit*) sender()->property(BUTTON_DATA).value<void*>();
	emit unitClicked(u);
	if(unicode_length(u->print(false, true, true)) <= 3) {
		settings->latest_button_unit = u->print(false, true, true);
		unitButton->setText(QString::fromStdString(settings->latest_button_unit));
		Prefix *p1 = CALCULATOR->getExactDecimalPrefix(-3), *p2 = CALCULATOR->getExactDecimalPrefix(3);
		unitButton->setToolTip(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs)), p1 ? QString::fromStdString(p1->longName()) : QString(), p2 ? QString::fromStdString(p2->longName()) : QString());
		unitButton->setProperty(BUTTON_DATA, QVariant::fromValue((void*) u));
	}
}
void KeypadWidget::onUnitButtonClicked2() {
	emit prefixClicked((Prefix*) sender()->property(BUTTON_DATA2).value<void*>());
}
void KeypadWidget::onUnitButtonClicked3() {
	emit prefixClicked((Prefix*) sender()->property(BUTTON_DATA3).value<void*>());
}
void KeypadWidget::onItemButtonClicked2() {
	ExpressionItem *item = (ExpressionItem*) sender()->property(BUTTON_DATA2).value<void*>();
	if(item->type() == TYPE_FUNCTION) emit functionClicked((MathFunction*) item);
	else if(item->type() == TYPE_VARIABLE) emit variableClicked((Variable*) item);
	else if(item->type() == TYPE_UNIT) emit unitClicked((Unit*) item);
}
void KeypadWidget::onItemButtonClicked3() {
	ExpressionItem *item = (ExpressionItem*) sender()->property(BUTTON_DATA3).value<void*>();
	if(item->type() == TYPE_FUNCTION) emit functionClicked((MathFunction*) item);
	else if(item->type() == TYPE_VARIABLE) emit variableClicked((Variable*) item);
	else if(item->type() == TYPE_UNIT) emit unitClicked((Unit*) item);
}

KeypadButton::KeypadButton(const QString &text, QWidget *parent, bool autorepeat) : QToolButton(parent), longPressTimer(NULL), b_longpress(false), b_autorepeat(autorepeat) {
	setFocusPolicy(Qt::TabFocus);
	if(text.contains("</")) richtext = text;
	else setText(text);
	updateSize();
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}
KeypadButton::KeypadButton(const QIcon &icon, QWidget *parent, bool autorepeat) : QToolButton(parent), longPressTimer(NULL), b_longpress(false), b_autorepeat(autorepeat) {
	setIcon(icon);
	setFocusPolicy(Qt::TabFocus);
	updateSize();
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}
KeypadButton::~KeypadButton() {}
void KeypadButton::updateSize() {
	QFontMetrics fm(font());
	QSize size = fm.boundingRect("STO").size();
	size.setWidth((size.width() * 1.5) + (settings->separate_keypad_menu_buttons && menu() ? 10 : 0)); size.setHeight(size.height() * 1.6);
	setMinimumSize(size);
}
void KeypadButton::setRichText(const QString &text) {
	richtext = text;
	setText(QString());
}
const QString &KeypadButton::richText() const {
	return richtext;
}
void KeypadButton::menuSet() {
	if(settings->separate_keypad_menu_buttons) updateSize();
	QString str = toolTip();
	if(!str.isEmpty()) {
		str.replace(tr("<i>Right-click/long press</i>: %1").arg(QString()), tr("<i>Right-click</i>: %1").arg(QString()));
		str += "<br>";
	}
	str += tr("<i>Long press</i>: %1").arg(tr("Open menu"));
	QToolButton::setToolTip(str);
	connect(menu(), SIGNAL(aboutToShow()), this, SLOT(menuOpened()));
	connect(menu(), SIGNAL(aboutToHide()), this, SLOT(menuClosed()));
}
void KeypadButton::menuOpened() {
	if(longPressTimer && longPressTimer->isActive()) longPressTimer->stop();
}
void KeypadButton::menuClosed() {
}
void KeypadButton::paintEvent(QPaintEvent *p) {
	QToolButton::paintEvent(p);
	if(!richtext.isEmpty()) {
		QPainter painter(this);
		QTextDocument doc;
		doc.setHtml(richtext);
		QFont f = font();
		doc.setDefaultFont(f);
		QPointF point = p->rect().center();
		bool b_menu = (menu() && settings->separate_keypad_menu_buttons);
		point.setY(point.y() - doc.size().height() / 2.0 + 2.0);
		point.setX((point.x() - (b_menu ? 6.0 : 0.0)) - doc.size().width() / 2.0 + 2.0);
		painter.translate(point);
		painter.save();
		doc.drawContents(&painter);
		painter.restore();
	}
}
void KeypadButton::mousePressEvent(QMouseEvent *e) {
	b_longpress = false;
	if(e->button() == Qt::LeftButton && (!menu() || popupMode() != QToolButton::DelayedPopup)) {
		if(!longPressTimer) {
			longPressTimer = new QTimer(this);
			longPressTimer->setSingleShot(!b_autorepeat);
			connect(longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
		}
		longPressTimer->start(b_autorepeat ? 250 : 500);
	}
	QToolButton::mousePressEvent(e);
}
void KeypadButton::longPressTimeout() {
	b_longpress = true;
	if(menu()) {
		showMenu();
	} else if(b_autorepeat) {
		emit clicked();
	} else {
		emit clicked2();
	}
}
void KeypadButton::mouseReleaseEvent(QMouseEvent *e) {
	if(e->button() == Qt::RightButton) {
		emit clicked2();
	} else if(e->button() == Qt::MiddleButton) {
		emit clicked3();
	} else {
		if(longPressTimer && longPressTimer->isActive() && e->button() == Qt::LeftButton) {
			longPressTimer->stop();
		} else if(b_longpress && e->button() == Qt::LeftButton) {
			b_longpress = false;
			blockSignals(true);
			QToolButton::mouseReleaseEvent(e);
			blockSignals(false);
			return;
		}
		QToolButton::mouseReleaseEvent(e);
	}
}
void KeypadButton::setToolTip(const QString &s1, const QString &s2, const QString &s3) {
	QString str;
	if(!s1.isEmpty()) str += s1;
	if(!s2.isEmpty()) {
		if(!str.isEmpty()) str += "<br>";
		if(!b_autorepeat) str += tr("<i>Right-click/long press</i>: %1").arg(s2);
		else str += tr("<i>Right-click</i>: %1").arg(s2);
	}
	if(!s3.isEmpty()) {
		if(!str.isEmpty()) str += "<br>";
		str += tr("<i>Middle-click</i>: %1").arg(s3);
	}
	QToolButton::setToolTip(str);
}


