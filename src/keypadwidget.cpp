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
#include <QTextDocument>
#include <QDebug>

#include "keypadwidget.h"
#include "qalculateqtsettings.h"

#define BUTTON_DATA "QALCULATE DATA1"
#define BUTTON_DATA2 "QALCULATE DATA2"
#define BUTTON_DATA3 "QALCULATE DATA3"

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

#define ITEM_BUTTON3(o1, o2, o3, x, r, c)	button = new KeypadButton(x, this); \
						button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) o1)); \
						button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) o2)); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o3)); \
						button->setToolTip(QString::fromStdString(o1->title(true)), o1 == o2 ? QString() : QString::fromStdString(o2->title(true)), o2 == o3 ? QString() : QString::fromStdString(o3->title(true))); \
						connect(button, SIGNAL(clicked()), this, SLOT(onItemButtonClicked())); \
						connect(button, SIGNAL(clicked2()), this, SLOT(onItemButtonClicked2())); \
						connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
						grid->addWidget(button, r, c, 1, 1);

#define ITEM_OPERATOR_ITEM_BUTTON(o1, o2, o3, x, r, c)	button = new KeypadButton(x, this); \
							button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) o1)); \
							button->setProperty(BUTTON_DATA2, o2); \
							button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o3)); \
							button->setToolTip(QString::fromStdString(o1->title(true)), o2, QString::fromStdString(o3->title(true))); \
							connect(button, SIGNAL(clicked()), this, SLOT(onItemButtonClicked())); \
							connect(button, SIGNAL(clicked2()), this, SLOT(onOperatorButtonClicked2())); \
							connect(button, SIGNAL(clicked3()), this, SLOT(onItemButtonClicked3())); \
							grid->addWidget(button, r, c, 1, 1);

#define SET_ITEM_BUTTON2(button, o1, o2)	button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) o1)); \
						button->setProperty(BUTTON_DATA2, QVariant::fromValue((void*) o2)); \
						button->setProperty(BUTTON_DATA3, QVariant::fromValue((void*) o2)); \
						button->setToolTip(QString::fromStdString(o1->title(true)), QString::fromStdString(o2->title(true)));

#define ITEM_BUTTON(o, x, r, c) 		ITEM_BUTTON3(o, o, o, x, r, c)
#define ITEM_BUTTON2(o1, o2, x, r, c) 		ITEM_BUTTON3(o1, o2, o2, x, r, c)

KeypadWidget::KeypadWidget(QWidget *parent) : QWidget(parent) {
	QHBoxLayout *box = new QHBoxLayout(this);
	QGridLayout *grid1 = new QGridLayout();
	QGridLayout *grid2 = new QGridLayout();
	box->addLayout(grid1, 2);
	box->addSpacing(box->spacing());
	box->addLayout(grid2, 3);
	QGridLayout *grid = grid1;
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
	button->setToolTip(tr("Exponentiation"), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_SQUARE)->title(true)), QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_EXP)->title(true)));
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
		button->setToolTip(QString::fromStdString(CALCULATOR->getFunctionById(FUNCTION_ID_FACTORIAL)->title(true)), QString::fromStdString(f->title(true)), QString::fromStdString(f2->title(true)));
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
	button->setToolTip(tr("Percent or remainder"), QString::fromStdString(CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE)->title(true)));
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
	button->setToolTip(QString(), "x<sup>0</sup>", QString::fromStdString(CALCULATOR->getDegUnit()->title(true)));
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
		button->setToolTip("10<sup>x</sup>", QString::fromStdString(f->title(true)), QString::fromStdString(f2->title(true)));
	} else {
		OPERATOR_BUTTON("E", 3, c);
		button->setToolTip("10<sup>x</sup>");
	}
	button->setText("EXP");
	c++;
	ITEM_BUTTON(settings->vans[0], "ANS", 3, c);
	button = new KeypadButton("ANS", this);
	button->setProperty(BUTTON_DATA, QVariant::fromValue((void*) settings->vans[0])); \
	button->setToolTip(QString::fromStdString(settings->vans[0]->title(true)), tr("Previous result (static)"));
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
		button->setToolTip(tr("Subtraction"), QString::fromStdString(f->title(true)) + " (" + QKeySequence(Qt::CTRL | Qt::Key_Minus).toString(QKeySequence::NativeText) + ")", tr("Minus"));
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
	button->setToolTip(tr("Calculate expression"));
	connect(button, SIGNAL(clicked()), this, SIGNAL(equalsClicked()));
	connect(button, SIGNAL(clicked2()), this, SIGNAL(equalsClicked()));
	connect(button, SIGNAL(clicked3()), this, SIGNAL(equalsClicked()));
	grid->addWidget(button, 3, c, 1, 1);
	grid1->setRowStretch(0, 1);
	grid1->setRowStretch(1, 1);
	grid1->setRowStretch(2, 1);
	grid1->setRowStretch(3, 1);
	grid1->setRowStretch(4, 1);
	grid2->setRowStretch(0, 1);
	grid2->setRowStretch(1, 1);
	grid2->setRowStretch(2, 1);
	grid2->setRowStretch(3, 1);
	grid1->setRowStretch(0, 1);
	grid1->setColumnStretch(0, 1);
	grid1->setColumnStretch(1, 1);
	grid1->setColumnStretch(2, 1);
	grid1->setColumnStretch(3, 1);
	grid2->setColumnStretch(0, 1);
	grid2->setColumnStretch(1, 1);
	grid2->setColumnStretch(2, 1);
	grid2->setColumnStretch(3, 1);
	grid2->setColumnStretch(4, 1);
	grid2->setColumnStretch(5, 1);
}
KeypadWidget::~KeypadWidget() {}

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


