/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPRESSION_EDIT_H
#define EXPRESSION_EDIT_H

#include <QTextEdit>

class ExpressionEdit : public QTextEdit {

	Q_OBJECT

	public:

		ExpressionEdit(QWidget *parent = NULL);
		virtual ~ExpressionEdit();

		std::string expression() const;
		QSize sizeHint() const;

	protected slots:

		void keyPressEvent(QKeyEvent*);

	public slots:

		void setExpression(std::string);

	signals:

		void returnPressed();

};

#endif //EXPRESSION_EDIT_H
