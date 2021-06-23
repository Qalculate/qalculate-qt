/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FUNCTION_EDIT_DIALOG_H
#define FUNCTION_EDIT_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;

class FunctionEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit;
		QPlainTextEdit *expressionEdit;
		QPushButton *okButton;
		QRadioButton *ref1Button, *ref2Button;

		bool read_only;

	protected slots:

		void onNameEdited(const QString&);
		void onExpressionChanged();

	public:

		FunctionEditDialog(QWidget *parent = NULL);
		virtual ~FunctionEditDialog();

		UserFunction *createFunction();
		bool modifyFunction(MathFunction *f);
		void setFunction(MathFunction *f);
		void setExpression(const QString&);
		QString expression() const;
		void setName(const QString&);
		void setRefType(int);

		static bool editFunction(QWidget *parent, MathFunction *f);
		static UserFunction* newFunction(QWidget *parent);

};

#endif //FUNCTION_EDIT_DIALOG_H
