/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QFontMetrics>
#include <QKeyEvent>

#include <libqalculate/qalculate.h>

#include "expressionedit.h"
#include "qalculateqtsettings.h"

ExpressionEdit::ExpressionEdit(QWidget *parent) : QTextEdit(parent) {
}
ExpressionEdit::~ExpressionEdit() {}

void ExpressionEdit::setExpression(std::string str) {
	setPlainText(QString::fromStdString(str));
}
std::string ExpressionEdit::expression() const {
	return toPlainText().toStdString();
}
QSize ExpressionEdit::sizeHint() const {
	QSize size = QTextEdit::sizeHint();
	QFontMetrics fm(font());
	size.setHeight(fm.boundingRect("Äy").height() * 3);
	return size;
}
void ExpressionEdit::keyPressEvent(QKeyEvent *event) {
	if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
		switch(event->key()) {
			case Qt::Key_Asterisk: {
				insertPlainText(SIGN_MULTIPLICATION);
				return;
			}
			case Qt::Key_Minus: {
				insertPlainText(SIGN_MINUS);
				return;
			}
			case Qt::Key_Dead_Circumflex: {
				insertPlainText("^");
				return;
			}
			case Qt::Key_Return: {}
			case Qt::Key_Enter: {
				emit returnPressed();
				return;
			}
		}
	}
	if(event->key() == Qt::Key_Asterisk && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::KeypadModifier) || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
		insertPlainText("^");
		return;
	}
	QTextEdit::keyPressEvent(event);
}

