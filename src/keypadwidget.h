/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef KEYPAD_WIDGET_H
#define KEYPAD_WIDGET_H

#include <QWidget>
#include <QPushButton>

#include <libqalculate/qalculate.h>

class QTimer;
class QStackedLayout;

class KeypadButton : public QPushButton {

	Q_OBJECT

	public:

		KeypadButton(const QString &text, QWidget *parent = NULL, bool autorepeat = false);
		KeypadButton(const QIcon &icon, QWidget *parent = NULL, bool autorepeat = false);
		~KeypadButton();

		void setToolTip(const QString &s1, const QString &s2 = QString(), const QString &s3 = QString());
		void setRichText(const QString &text);

	protected:

		QString richtext;
		QTimer *longPressTimer;
		bool b_longpress;
		bool b_autorepeat;

		void paintEvent(QPaintEvent*) override;
		void mouseReleaseEvent(QMouseEvent*) override;
		void mousePressEvent(QMouseEvent*) override;

	protected slots:

		void longPressTimeout();

	signals:

		void clicked2();
		void clicked3();

};

enum {
	KEYPAD_GENERAL = 0,
	KEYPAD_PROGRAMMING = 1,
	KEYPAD_ALGEBRA = 2,
	KEYPAD_CUSTOM = 3
};

class KeypadWidget : public QWidget {

	Q_OBJECT

	public:

		KeypadWidget(QWidget *parent = NULL);
		virtual ~KeypadWidget();

	protected:

		KeypadButton *sinButton, *cosButton, *tanButton, *delButton, *acButton, *backButton, *forwardButton, *dotButton, *commaButton, *multiplicationButton, *divisionButton, *imaginaryButton, *binButton, *octButton, *decButton, *hexButton, *aButton, *bButton, *cButton, *dButton, *eButton, *fButton;
		QStackedLayout *leftStack;
		void changeEvent(QEvent *e);

	protected slots:

		void onSymbolButtonClicked();
		void onOperatorButtonClicked();
		void onItemButtonClicked();
		void onSymbolButtonClicked2();
		void onOperatorButtonClicked2();
		void onItemButtonClicked2();
		void onSymbolButtonClicked3();
		void onOperatorButtonClicked3();
		void onItemButtonClicked3();
		void onBaseButtonClicked();
		void onBaseButtonClicked2();
		void onHypToggled(bool);

	public slots:

		void updateBase();
		void updateSymbols();
		void setKeypadType(int);

	signals:

		void operatorClicked(const QString&);
		void symbolClicked(const QString&);
		void functionClicked(MathFunction *f);
		void variableClicked(Variable *v);
		void unitClicked(Unit *u);
		void delClicked();
		void clearClicked();
		void equalsClicked();
		void parenthesesClicked();
		void bracketsClicked();
		void leftClicked();
		void rightClicked();
		void endClicked();
		void startClicked();
		void MSClicked();
		void MCClicked();
		void MRClicked();
		void MPlusClicked();
		void MMinusClicked();
		void backspaceClicked();
		void answerClicked();
		void baseClicked(int, bool);
		void factorizeClicked();
		void expandClicked();

};

#endif //KEYPAD_WIDGET_H
