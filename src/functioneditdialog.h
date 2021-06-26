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

	protected slots:

		void onNameEdited(const QString&);
		void onExpressionChanged();

	public:

		FunctionEditDialog(QWidget *parent = NULL);
		virtual ~FunctionEditDialog();

		UserFunction *createFunction(MathFunction **replaced_item = NULL);
		bool modifyFunction(MathFunction *f, MathFunction **replaced_item = NULL);
		void setFunction(MathFunction *f);
		void setExpression(const QString&);
		QString expression() const;
		void setName(const QString&);
		void setRefType(int);

		static bool editFunction(QWidget *parent, MathFunction *f, MathFunction **replaced_item = NULL);
		static UserFunction* newFunction(QWidget *parent, MathFunction **replaced_item = NULL);

};

#endif //FUNCTION_EDIT_DIALOG_H
