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
#include <QListWidget>
#include <QTextDocument>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QDialog>
#include <QDialogButtonBox>
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

#define BASE_BUTTON(x, i, r, c)			button = new KeypadButton(x); \
						button->setCheckable(true); \
						grid->addWidget(button, r, c); \
						button->setProperty(BUTTON_DATA, i); \
						connect(button, SIGNAL(clicked()), this, SLOT(onBaseButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onBaseButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onBaseButtonClicked2()));

KeypadWidget::KeypadWidget(QWidget *parent) : QWidget(parent) {
	QHBoxLayout *box = new QHBoxLayout(this);
	leftStack = new QStackedLayout();
	box->addLayout(leftStack, 4 + (settings->custom_button_columns <= 4 ? 0 : (settings->custom_button_columns - 4) * 1.5));
	box->addSpacing(box->spacing());
	numpad = new QWidget(this);
	if(settings->hide_numpad) numpad->hide();
	QGridLayout *grid2 = new QGridLayout(numpad);
	grid2->setContentsMargins(0, 0, 0, 0);
	box->addWidget(numpad, 6);
	QWidget *keypadG = new QWidget(this);
	leftStack->addWidget(keypadG);
	QGridLayout *grid = new QGridLayout(keypadG);
	grid->setContentsMargins(0, 0, 0, 0);
	KeypadButton *button;
	MathFunction *f, *f2;
	int c = 0;
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
	c++;
	button = new KeypadButton("hyp");
	button->setCheckable(true);
	grid->addWidget(button, c, 0, 1, 1);
	connect(button, SIGNAL(toggled(bool)), this, SLOT(onHypToggled(bool)));
	ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_SIN), CALCULATOR->getFunctionById(FUNCTION_ID_ASIN), tr("sin"), c, 1);
	sinButton = button;
	ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_COS), CALCULATOR->getFunctionById(FUNCTION_ID_ACOS), tr("cos"), c, 2);
	cosButton = button;
	ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_TAN), CALCULATOR->getFunctionById(FUNCTION_ID_ATAN), tr("tan"), c, 3);
	tanButton = button;
	c++;
	OPERATOR_ITEM2_BUTTON("^", CALCULATOR->getFunctionById(FUNCTION_ID_SQUARE), CALCULATOR->getFunctionById(FUNCTION_ID_EXP), c, 3);
	button->setRichText("x<sup>y</sup>");
	button->setToolTip(tr("Exponentiation"), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_SQUARE)->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_EXP)->title(true, settings->printops.use_unicode_signs)));
	ITEM_BUTTON3(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), CALCULATOR->getFunctionById(FUNCTION_ID_CBRT), CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), SIGN_SQRT, c, 2);
	f = CALCULATOR->getActiveFunction("log10"); f2 = CALCULATOR->getActiveFunction("log2");
	if(f && f2) {
		ITEM_BUTTON3(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), f, f2, "ln", c, 1);
	} else {
		ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), "ln", c, 1);
	}
	f = CALCULATOR->getActiveFunction("perm"); f2 = CALCULATOR->getActiveFunction("comb");
	if(f && f2) {
		OPERATOR_ITEM2_BUTTON("!", f, f2, c, 0);
		button->setToolTip(QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_FACTORIAL)->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)), QString::fromStdString(f2->title(true, settings->printops.use_unicode_signs)));
	} else {
		OPERATOR_BUTTON("!", c, 0);
	}
	button->setText("x!");
	c++;
	ITEM_BUTTON3(CALCULATOR->getVariableById(VARIABLE_ID_PI), CALCULATOR->getVariableById(VARIABLE_ID_E), CALCULATOR->getVariableById(VARIABLE_ID_EULER), SIGN_PI, c, 3);
	ITEM_OPERATOR_ITEM_BUTTON(CALCULATOR->getVariableById(VARIABLE_ID_I), "∠", CALCULATOR->getFunctionById(FUNCTION_ID_ARG), CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0 ? "j" : "i", c, 2);
	imaginaryButton = button;
	QFont ifont(button->font());
	ifont.setStyle(QFont::StyleItalic);
	button->setFont(ifont);
	SYMBOL_BUTTON3("x", "y", "z", c, 0);
	button->setToolTip(QString(), "<i>y</i>", "<i>z</i>");
	button->setFont(ifont);
	SYMBOL_BUTTON("=", c, 1);
	button->setRichText("<i>x</i> =");
	c++;
	SYMBOL_BUTTON2("%", "‰", c, 1);
	button->setToolTip(tr("Percent or remainder"), QString::fromStdString(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE)->title(true, settings->printops.use_unicode_signs)));
	SYMBOL_ITEM2_BUTTON("±", CALCULATOR->getFunctionById(FUNCTION_ID_UNCERTAINTY), CALCULATOR->getFunctionById(FUNCTION_ID_INTERVAL), c, 0);
	button->setToolTip(tr("Uncertainty/interval"), tr("Relative error"), tr("Interval"));
	backButton = new KeypadButton(LOAD_ICON("go-back"), this, true);
	backButton->setToolTip(tr("Move cursor left"), tr("Move cursor to start"));
	connect(backButton, SIGNAL(clicked()), this, SIGNAL(leftClicked()));
	connect(backButton, SIGNAL(clicked2()), this, SIGNAL(startClicked()));
	connect(backButton, SIGNAL(clicked3()), this, SIGNAL(startClicked()));
	grid->addWidget(backButton, c, 2, 1, 1);
	forwardButton = new KeypadButton(LOAD_ICON("go-forward"), this, true);
	forwardButton->setToolTip(tr("Move cursor right"), tr("Move cursor to end"));
	connect(forwardButton, SIGNAL(clicked()), this, SIGNAL(rightClicked()));
	connect(forwardButton, SIGNAL(clicked2()), this, SIGNAL(endClicked()));
	connect(forwardButton, SIGNAL(clicked3()), this, SIGNAL(endClicked()));
	grid->addWidget(forwardButton, c, 3, 1, 1);
	for(c = 0; c < 4; c++) grid->setColumnStretch(c, 1);
	for(int r = 0; r < 5; r++) grid->setRowStretch(r, 1);

	QWidget *keypadP = new QWidget(this);
	leftStack->addWidget(keypadP);
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
	OPERATOR_BUTTON(" xor ", 2, 2);
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
	OPERATOR_BUTTON2(" mod ", " rem ", 4, 0);
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

	QWidget *keypadX = new QWidget(this);
	leftStack->addWidget(keypadX);
	grid = new QGridLayout(keypadX);
	grid->setContentsMargins(0, 0, 0, 0);
	for(size_t i = 0; i < 3; i++) {
		QToolButton *tb = new QToolButton(this);
		tb->setText(i == 0 ? "x" : (i == 1 ? "y" : "z"));
		tb->setFocusPolicy(Qt::TabFocus);
		QFontMetrics fm(tb->font());
		QSize size = fm.boundingRect("DEL").size();
		size.setWidth(size.width() + 10); size.setHeight(size.height() + 10);
		tb->setMinimumSize(size);
		tb->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		int id = 0;
		if(i == 0) id = VARIABLE_ID_X;
		else if(i == 1) id = VARIABLE_ID_Y;
		else if(i == 2) id = VARIABLE_ID_Z;
		tb->setFont(ifont);
		QMenu *menu = new QMenu(this);
		tb->setMenu(menu);
		tb->setPopupMode(QToolButton::MenuButtonPopup);
		connect(tb, SIGNAL(clicked()), this, SLOT(onSymbolToolButtonClicked()));
		grid->addWidget(tb, 0, i);
		menu->addSeparator();
		connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateAssumptions()));
		menu->setProperty(BUTTON_DATA, id);
		QAction *action = menu->addAction(tr("Default assumptions"), this, SLOT(defaultAssumptionsActivated())); action->setProperty(BUTTON_DATA, id);
		QActionGroup *group = new QActionGroup(this); group->setObjectName("group_type_" + tb->text());
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
		menu->addSeparator();
		group = new QActionGroup(this); group->setObjectName("group_sign_" + tb->text());
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
	connect(button, SIGNAL(clicked3()), this, SIGNAL(expandClicked()));
	button->setToolTip(tr("Factorize"), tr("Expand"));
	grid->addWidget(button, 3, 3);
	ITEM_BUTTON(CALCULATOR->getVariableById(VARIABLE_ID_PI), SIGN_PI, 1, 0);
	ITEM_BUTTON2(CALCULATOR->getVariableById(VARIABLE_ID_E), CALCULATOR->getFunctionById(FUNCTION_ID_EXP), "e", 1, 1);
	ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_SQRT), CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), SIGN_SQRT, 4, 0);
	ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_CBRT), CALCULATOR->getFunctionById(FUNCTION_ID_ROOT), "∛", 4, 1);
	ITEM_BUTTON2(CALCULATOR->getFunctionById(FUNCTION_ID_LOG), CALCULATOR->getFunctionById(FUNCTION_ID_LOGN), "ln", 4, 2);
	ITEM_BUTTON(CALCULATOR->getFunctionById(FUNCTION_ID_ABS), "|x|", 4, 3);
	for(c = 0; c < 4; c++) grid->setColumnStretch(c, 1);
	for(int r = 0; r < 5; r++) grid->setRowStretch(r, 1);

	QWidget *keypadC = new QWidget(this);
	leftStack->addWidget(keypadC);
	grid = new QGridLayout(keypadC);
	grid->setContentsMargins(0, 0, 0, 0);
	customGrid = grid;
	customEditButton = new QToolButton(this);
	customEditButton->setIcon(LOAD_ICON("document-edit"));
	customEditButton->setFocusPolicy(Qt::TabFocus);
	customEditButton->setCheckable(true);
	QFontMetrics fm(customEditButton->font());
	QSize size = fm.boundingRect("DEL").size();
	size.setWidth(size.width() + 10); size.setHeight(size.height() + 10);
	customEditButton->setMinimumSize(size);
	customEditButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	QMenu *menu = new QMenu(this);
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

	grid = grid2;
	c = 0;
	SYMBOL_BUTTON2("(", "[", 1, c)
	button->setToolTip(tr("Left parenthesis"), tr("Left vector bracket"));
	SYMBOL_BUTTON2(")", "]", 2, c)
	button->setToolTip(tr("Right parenthesis"), tr("Right vector bracket"));
	button = new KeypadButton("(x)", this);
	button->setToolTip(tr("Smart parentheses"), tr("Vector brackets"));
	connect(button, SIGNAL(clicked()), this, SIGNAL(parenthesesClicked()));
	connect(button, SIGNAL(clicked2()), this, SIGNAL(bracketsClicked()));
	connect(button, SIGNAL(clicked3()), this, SIGNAL(bracketsClicked()));
	grid->addWidget(button, 0, c, 1, 1);
	SYMBOL_BUTTON3(QString::fromStdString(CALCULATOR->getComma()), " ", "\n", 3, c)
	button->setToolTip(tr("Argument separator"), tr("Blank space"), tr("New line"));
	commaButton = button;
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
	dotButton = button;
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
	multiplicationButton = button;
	delButton = new KeypadButton(LOAD_ICON("edit-delete"), this, true);
	connect(delButton, SIGNAL(clicked()), this, SIGNAL(delClicked()));
	connect(delButton, SIGNAL(clicked2()), this, SIGNAL(backspaceClicked()));
	connect(delButton, SIGNAL(clicked3()), this, SIGNAL(backspaceClicked()));
	delButton->setToolTip(tr("Delete"), tr("Backspace"));
	grid->addWidget(delButton, 0, c, 1, 1);
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
	divisionButton = button;
	button->setProperty(BUTTON_DATA, settings->divisionSign(false));
	button->setToolTip(tr("Division"), tr("Bitwise OR"), tr("Bitwise NOT"));
	acButton = new KeypadButton(LOAD_ICON("edit-clear"), this);
	acButton->setToolTip(tr("Clear expression"));
	connect(acButton, SIGNAL(clicked()), this, SIGNAL(clearClicked()));
	connect(acButton, SIGNAL(clicked2()), this, SIGNAL(clearClicked()));
	connect(acButton, SIGNAL(clicked3()), this, SIGNAL(clearClicked()));
	grid->addWidget(acButton, 0, c, 1, 1);
	button = new KeypadButton("=", this);
	button->setToolTip(tr("Calculate expression"), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_SOLVE)->title(true, settings->printops.use_unicode_signs)));
	button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) CALCULATOR->getFunctionById(FUNCTION_ID_SOLVE))); \
	connect(button, SIGNAL(clicked()), this, SIGNAL(equalsClicked()));
	connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked()));
	connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked()));
	grid->addWidget(button, 3, c, 1, 1);
	for(c = 0; c < 6; c++) grid->setColumnStretch(c, 1);
	for(int r = 0; r < 4; r++) grid->setRowStretch(r, 1);
	setKeypadType(settings->keypad_type);
}
KeypadWidget::~KeypadWidget() {}

void KeypadWidget::updateCustomActionOK() {
	int i = actionList->currentRow();
	customOKButton->setEnabled(i == 0 || (i > 0 && (!labelEdit || !labelEdit->text().trimmed().isEmpty()) && (!SHORTCUT_REQUIRES_VALUE(i - 1) || !valueEdit->text().trimmed().isEmpty())));
}
void KeypadWidget::customActionOKClicked() {
	QString value = valueEdit->text().trimmed();
	if(settings->testShortcutValue(actionList->currentRow() - 1, value, customActionDialog)) {
		customActionDialog->accept();
	} else {
		valueEdit->setFocus();
	}
	valueEdit->setText(value);
}
void KeypadWidget::currentCustomActionChanged(int i) {
	valueEdit->setEnabled(SHORTCUT_REQUIRES_VALUE(i));
	if(!valueEdit->isEnabled()) valueEdit->clear();
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
	grid->addWidget(new QLabel(tr("Action:"), dialog), i == 2 ? 0 : 1, 0);
	actionList = new QListWidget(dialog);
	grid->addWidget(actionList, i == 2 ? 1 : 2, 0, 1, 2);
	int type = -1;
	if(button->property(i == 2 ? BUTTON_DATA2 : (i == 3 ? BUTTON_DATA3 : BUTTON_DATA)).isValid()) type = button->property(i == 2 ? BUTTON_DATA2 : (i == 3 ? BUTTON_DATA3 : BUTTON_DATA)).toInt();
	actionList->addItem(tr("None"));
	actionList->setCurrentRow(0);
	for(int i = SHORTCUT_TYPE_FUNCTION; i <= LAST_SHORTCUT_TYPE; i++) {
		actionList->addItem(settings->shortcutTypeText((shortcut_type) i));
		if(i == type) actionList->setCurrentRow(i + 1);
	}
	grid->addWidget(new QLabel(tr("Value:"), dialog), i == 2 ? 2 : 3, 0);
	valueEdit = new MathLineEdit(dialog);
	if(button->property(i == 2 ? BUTTON_VALUE2 : (i == 3 ? BUTTON_VALUE3 : BUTTON_VALUE)).isValid()) valueEdit->setText(button->property(i == 2 ? BUTTON_VALUE2 : (i == 3 ? BUTTON_VALUE3 : BUTTON_VALUE)).toString());
	grid->addWidget(valueEdit, i == 2 ? 2 : 3, 1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(customActionOKClicked()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	if(labelEdit) connect(labelEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateCustomActionOK()));
	connect(actionList, SIGNAL(currentRowChanged(int)), this, SLOT(updateCustomActionOK()));
	connect(actionList, SIGNAL(currentRowChanged(int)), this, SLOT(currentCustomActionChanged(int)));
	connect(valueEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateCustomActionOK()));
	customOKButton = buttonBox->button(QDialogButtonBox::Ok);
	customOKButton->setEnabled(false);
	if(labelEdit) labelEdit->setFocus();
	else actionList->setFocus();
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
		cb->type[i - 1] = actionList->currentRow() - 1;
		cb->value[i - 1] = valueEdit->text().trimmed().toStdString();
		button->setProperty(i == 2 ? BUTTON_DATA2 : (i == 3 ? BUTTON_DATA3 : BUTTON_DATA), actionList->currentRow() - 1);
		button->setProperty(i == 2 ? BUTTON_VALUE2 : (i == 3 ? BUTTON_VALUE3 : BUTTON_VALUE), valueEdit->text().trimmed());
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
	((QBoxLayout*) layout())->setStretchFactor(leftStack, 4 + (settings->custom_button_columns <= 4 ? 0 : (settings->custom_button_columns - 4) * 1.5));
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
				settings->custom_buttons.erase(settings->custom_buttons.begin() + i);
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
				settings->custom_buttons.erase(settings->custom_buttons.begin() + i);
				break;
			}
		}
		customGrid->removeWidget(customButtons[c][r]);
		customButtons[c][r]->deleteLater();
	}
	customButtons.pop_back();
	((QBoxLayout*) layout())->setStretchFactor(leftStack, 4 + (settings->custom_button_columns <= 4 ? 0 : (settings->custom_button_columns - 4) * 1.5));
	addColumnAction->setEnabled(true);
	removeColumnAction->setEnabled(settings->custom_button_columns > 1);
}
void KeypadWidget::onCustomButtonClicked() {
	KeypadButton *button = qobject_cast<KeypadButton*>(sender());
	if(b_edit || !button->property(BUTTON_DATA).isValid() || button->property(BUTTON_DATA).toInt() < 0) {
		editCustomAction(button, 1);
	} else {
		emit shortcutClicked(button->property(BUTTON_DATA).toInt(), button->property(BUTTON_VALUE).toString());
	}
}
void KeypadWidget::onCustomButtonClicked2() {
	KeypadButton *button = qobject_cast<KeypadButton*>(sender());
	if(b_edit || !button->property(BUTTON_DATA2).isValid() || button->property(BUTTON_DATA2).toInt() < 0) {
		editCustomAction(button, 2);
	} else {
		emit shortcutClicked(button->property(BUTTON_DATA2).toInt(), button->property(BUTTON_VALUE2).toString());
	}
}
void KeypadWidget::onCustomButtonClicked3() {
	KeypadButton *button = qobject_cast<KeypadButton*>(sender());
	if(b_edit || !button->property(BUTTON_DATA3).isValid() || button->property(BUTTON_DATA3).toInt() < 0) {
		editCustomAction(button, 3);
	} else {
		emit shortcutClicked(button->property(BUTTON_DATA2).toInt(), button->property(BUTTON_VALUE3).toString());
	}
}
void KeypadWidget::updateAssumptions() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	int id = menu->property(BUTTON_DATA).toInt();
	QActionGroup *group = NULL;
	Variable *v = CALCULATOR->getVariableById(id);
	if(!v || v->isKnown()) return;
	UnknownVariable *uv = (UnknownVariable*) v;
	Assumptions *ass = uv->assumptions();
	if(!ass) ass = CALCULATOR->defaultAssumptions();
	if(id == VARIABLE_ID_X) group = findChild<QActionGroup*>("group_type_x");
	else if(id == VARIABLE_ID_Y) group = findChild<QActionGroup*>("group_type_y");
	else if(id == VARIABLE_ID_Z) group = findChild<QActionGroup*>("group_type_z");
	if(!group) return;
	QList<QAction*> actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == ass->type()) {
			actions.at(i)->setChecked(true);
			break;
		}
	}
	group = NULL;
	if(id == VARIABLE_ID_X) group = findChild<QActionGroup*>("group_sign_x");
	else if(id == VARIABLE_ID_Y) group = findChild<QActionGroup*>("group_sign_y");
	else if(id == VARIABLE_ID_Z) group = findChild<QActionGroup*>("group_sign_z");
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
	if(i < 0 || i > KEYPAD_CUSTOM) i = 0;
	leftStack->setCurrentIndex(i);
}
void KeypadWidget::hideNumpad(bool b) {
	numpad->setVisible(!b);
}
void KeypadWidget::updateBase() {
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
	multiplicationButton->setText(settings->multiplicationSign());
	multiplicationButton->setText(settings->multiplicationSign());
	multiplicationButton->setProperty(BUTTON_DATA, settings->multiplicationSign());
	divisionButton->setText(settings->divisionSign());
	divisionButton->setProperty(BUTTON_DATA, settings->divisionSign(false));
	commaButton->setText(QString::fromStdString(CALCULATOR->getComma()));
	commaButton->setProperty(BUTTON_DATA, QString::fromStdString(CALCULATOR->getComma()));
	dotButton->setText(QString::fromStdString(CALCULATOR->getDecimalPoint()));
	dotButton->setProperty(BUTTON_DATA, QString::fromStdString(CALCULATOR->getDecimalPoint()));
	imaginaryButton->setText(CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") > 0 ? "j" : "i");
}
void KeypadWidget::changeEvent(QEvent *e) {
	if(e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) {
		acButton->setIcon(LOAD_ICON("edit-clear"));
		delButton->setIcon(LOAD_ICON("edit-delete"));
		backButton->setIcon(LOAD_ICON("go-back"));
		forwardButton->setIcon(LOAD_ICON("go-forward"));
		customEditButton->setIcon(LOAD_ICON("document-edit"));
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
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit symbolClicked(button->property(BUTTON_DATA).toString());
}
void KeypadWidget::onSymbolToolButtonClicked() {
	QToolButton *button = qobject_cast<QToolButton*>(sender());
	emit symbolClicked(button->text());
}
void KeypadWidget::onSymbolButtonClicked2() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit symbolClicked(button->property(BUTTON_DATA2).toString());
}
void KeypadWidget::onSymbolButtonClicked3() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit symbolClicked(button->property(BUTTON_DATA3).toString());
}
void KeypadWidget::onOperatorButtonClicked() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit operatorClicked(button->property(BUTTON_DATA).toString());
}
void KeypadWidget::onOperatorButtonClicked2() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit operatorClicked(button->property(BUTTON_DATA2).toString());
}
void KeypadWidget::onOperatorButtonClicked3() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit operatorClicked(button->property(BUTTON_DATA3).toString());
}
void KeypadWidget::onBaseButtonClicked() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit baseClicked(button->property(BUTTON_DATA).toInt(), true);
}
void KeypadWidget::onBaseButtonClicked2() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	emit baseClicked(button->property(BUTTON_DATA).toInt(), false);
}
void KeypadWidget::onItemButtonClicked() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	ExpressionItem *item = (ExpressionItem*) button->property(BUTTON_DATA).value<void*>();
	if(item->type() == TYPE_FUNCTION) emit functionClicked((MathFunction*) item);
	else if(item->type() == TYPE_VARIABLE) emit variableClicked((Variable*) item);
	else if(item->type() == TYPE_UNIT) emit unitClicked((Unit*) item);
}
void KeypadWidget::onItemButtonClicked2() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	ExpressionItem *item = (ExpressionItem*) button->property(BUTTON_DATA2).value<void*>();
	if(item->type() == TYPE_FUNCTION) emit functionClicked((MathFunction*) item);
	else if(item->type() == TYPE_VARIABLE) emit variableClicked((Variable*) item);
	else if(item->type() == TYPE_UNIT) emit unitClicked((Unit*) item);
}
void KeypadWidget::onItemButtonClicked3() {
	QPushButton *button = qobject_cast<QPushButton*>(sender());
	ExpressionItem *item = (ExpressionItem*) button->property(BUTTON_DATA3).value<void*>();
	if(item->type() == TYPE_FUNCTION) emit functionClicked((MathFunction*) item);
	else if(item->type() == TYPE_VARIABLE) emit variableClicked((Variable*) item);
	else if(item->type() == TYPE_UNIT) emit unitClicked((Unit*) item);
}

KeypadButton::KeypadButton(const QString &text, QWidget *parent, bool autorepeat) : QPushButton(text.contains("</") ? QString() : text, parent), longPressTimer(NULL), b_longpress(false), b_autorepeat(autorepeat) {
	setFocusPolicy(Qt::TabFocus);
	if(text.contains("</")) richtext = text;
	QFontMetrics fm(font());
	QSize size = fm.boundingRect("DEL").size();
	size.setWidth(size.width() + 10); size.setHeight(size.height() + 10);
	setMinimumSize(size);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}
KeypadButton::KeypadButton(const QIcon &icon, QWidget *parent, bool autorepeat) : QPushButton(icon, QString(), parent), longPressTimer(NULL), b_longpress(false), b_autorepeat(autorepeat) {
	setFocusPolicy(Qt::TabFocus);
	QFontMetrics fm(font());
	QSize size = fm.boundingRect("DEL").size();
	size.setWidth(size.width() + 10); size.setHeight(size.height() + 10);
	setMinimumSize(size);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}
KeypadButton::~KeypadButton() {}
void KeypadButton::setRichText(const QString &text) {
	richtext = text;
	setText(QString());
}
const QString &KeypadButton::richText() const {
	return richtext;
}
void KeypadButton::paintEvent(QPaintEvent *p) {
	QPushButton::paintEvent(p);
	if(!richtext.isEmpty()) {
		QPainter painter(this);
		QTextDocument doc;
		doc.setHtml(richtext);
		QFont f = font();
		doc.setDefaultFont(f);
		QPointF point = p->rect().center();
		point.setY(point.y() - doc.size().height() / 2.0 + 2.0);
		point.setX(point.x() - doc.size().width() / 2.0 + 2.0);
		painter.translate(point);
		painter.save();
		doc.drawContents(&painter);
		painter.restore();
	}
}
void KeypadButton::mousePressEvent(QMouseEvent *e) {
	if(e->button() == Qt::LeftButton) {
		if(!longPressTimer) {
			longPressTimer = new QTimer(this);
			longPressTimer->setSingleShot(!b_autorepeat);
			connect(longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
		}
		longPressTimer->start(b_autorepeat ? 250 : 500);
	}
	QPushButton::mousePressEvent(e);
}
void KeypadButton::longPressTimeout() {
	if(b_autorepeat) emit clicked();
	else emit clicked2();
}
void KeypadButton::mouseReleaseEvent(QMouseEvent *e) {
	if(e->button() == Qt::RightButton) {
		emit clicked2();
	} else if(e->button() == Qt::MiddleButton) {
		emit clicked3();
	} else {
		if(b_longpress && e->button() == Qt::LeftButton) {b_longpress = false; return;}
		if(longPressTimer && longPressTimer->isActive() && e->button() == Qt::LeftButton) {
			longPressTimer->stop();
		}
		QPushButton::mouseReleaseEvent(e);
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
	QPushButton::setToolTip(str);
}


