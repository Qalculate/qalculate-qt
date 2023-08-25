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
#include <QToolButton>
#include <QVector>

#include <libqalculate/qalculate.h>

class QTimer;
class QStackedLayout;
class QLineEdit;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QDialog;
class QGridLayout;
class QAction;
class QLabel;
class QPushButton;

class KeypadButton : public QToolButton {

	Q_OBJECT

	public:

		KeypadButton(const QString &text, QWidget *parent = NULL, bool autorepeat = false);
		KeypadButton(const QIcon &icon, QWidget *parent = NULL, bool autorepeat = false);
		~KeypadButton();

		void setToolTip(const QString &s1, const QString &s2 = QString(), const QString &s3 = QString());
		void setRichText(const QString &text);
		const QString &richText() const;
		void menuSet();

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
		void menuOpened();
		void menuClosed();

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

		KeypadButton *sinButton, *cosButton, *tanButton, *delButton, *acButton, *backButton, *forwardButton, *dotButton, *commaButton, *multiplicationButton, *divisionButton, *imaginaryButton, *binButton, *octButton, *decButton, *hexButton, *aButton, *bButton, *cButton, *dButton, *eButton, *fButton, *unitButton, *storeButton;
		QPushButton *customOKButton;
		QToolButton *customEditButton;
		QVector<QVector<KeypadButton*> > customButtons;
		QStackedLayout *leftStack;
		QGridLayout *customGrid;
		QLineEdit *labelEdit;
		QComboBox *valueEdit;
		QLabel *valueLabel;
		QListWidget *actionList;
		QWidget *numpad;
		QDialog *customActionDialog;
		QAction *addRowAction, *addColumnAction, *removeRowAction, *removeColumnAction;
		bool b_edit;
		void changeEvent(QEvent *e);
		void editCustomAction(KeypadButton*, int);

	protected slots:

		void onSymbolButtonClicked();
		void onOperatorButtonClicked();
		void onItemButtonClicked();
		void onUnitItemClicked();
		void onUnitButtonClicked2();
		void onUnitButtonClicked3();
		void onSymbolButtonClicked2();
		void onOperatorButtonClicked2();
		void onItemButtonClicked2();
		void onSymbolButtonClicked3();
		void onOperatorButtonClicked3();
		void onItemButtonClicked3();
		void onBaseButtonClicked();
		void onBaseButtonClicked2();
		void onCustomButtonClicked();
		void onCustomButtonClicked2();
		void onCustomButtonClicked3();
		void onHypToggled(bool);
		void assumptionsTypeActivated();
		void assumptionsSignActivated();
		void defaultAssumptionsActivated();
		void updateAssumptions();
		void onCustomEditClicked(bool);
		void removeCustomRow();
		void addCustomRow();
		void removeCustomColumn();
		void addCustomColumn();
		void updateCustomActionOK();
		void customActionOKClicked();
		void currentCustomActionChanged(QListWidgetItem*, QListWidgetItem*);

	public slots:

		void updateBase();
		void updateSymbols();
		void setKeypadType(int);
		void hideNumpad(bool);
		void showSeparateKeypadMenuButtons(bool);
		void updateVariables();

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
		void storeClicked();
		void newFunctionClicked();
		void backspaceClicked();
		void answerClicked();
		void baseClicked(int, bool);
		void factorizeClicked();
		void expandClicked();
		void expandPartialFractionsClicked();
		void expressionCalculationUpdated(int);
		void shortcutClicked(int, const QString&);
		void openVariablesRequest();
		void openUnitsRequest();

};

#endif //KEYPAD_WIDGET_H
