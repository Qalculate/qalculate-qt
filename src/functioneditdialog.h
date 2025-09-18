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
#include <QPlainTextEdit>
#include <QLineEdit>

#include <libqalculate/qalculate.h>

class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QTreeView;
class QStandardItemModel;
class QStandardItem;

class NamesEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QTreeView *namesView;
		QStandardItemModel *namesModel;
		QPushButton *addButton, *editButton, *delButton;
		ExpressionItem *o_item;
		int i_type;

	protected slots:

		void addClicked();
		void delClicked();
		void editClicked();
		void selectedNameChanged(const QModelIndex&, const QModelIndex&);
		void nameChanged(QStandardItem*);

	public:

		NamesEditDialog(int type, QWidget *parent = NULL, bool = false);
		virtual ~NamesEditDialog();

		void modifyNames(ExpressionItem*, const QString&);
		static void modifyName(ExpressionItem*, const QString&);
		void setNames(ExpressionItem*, const QString&);
		void modifyNames(DataProperty*, const QString&);
		static void modifyName(DataProperty*, const QString&);
		void setNames(DataProperty*, const QString&);
		void setName(const QString&);
		QString firstName();

};

class MathTextEdit : public QPlainTextEdit {

	public:

		MathTextEdit(QWidget *parent);
		~MathTextEdit();

		QSize sizeHint() const override;
		void insertText(const QString&);

	protected:

		void keyPressEvent(QKeyEvent *event) override;

};

class SmallTextEdit : public QPlainTextEdit {

	public:

		SmallTextEdit(int r, QWidget *parent);
		~SmallTextEdit();

		QSize sizeHint() const;

	protected:

		int i_rows;

};

class ExpandingMathLineEdit : public QLineEdit {

	Q_OBJECT

	public:
		ExpandingMathLineEdit(QWidget *parent);

		void setWidgetOwnsGeometry(bool value);

	protected:
		void changeEvent(QEvent *e) override;
		void keyPressEvent(QKeyEvent*) override;

	public slots:
		void resizeToContents();

	private:
		void updateMinimumWidth();

		int originalWidth;
		bool widgetOwnsGeometry;
};

class ArgumentEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QComboBox *typeCombo;
		QLineEdit *nameEdit, *conditionEdit;
		QDoubleSpinBox *minEdit, *maxEdit;
		QCheckBox *maxBox, *minBox, *vectorBox, *matrixBox, *zeroBox, *includeMinBox, *includeMaxBox, *testBox;

	protected slots:

		void typeChanged(int);
		void minToggled(bool);
		void maxToggled(bool);

	public:

		ArgumentEditDialog(QWidget *parent = NULL, bool = false);
		virtual ~ArgumentEditDialog();

		Argument *createArgument();
		void setArgument(Argument*);

};

class FunctionEditDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *nameEdit, *titleEdit, *conditionEdit, *exampleEdit;
		QComboBox *categoryEdit;
		QCheckBox *hideBox;
		QPlainTextEdit *expressionEdit, *descriptionEdit;
		QPushButton *okButton, *argDelButton, *argAddButton, *argEditButton, *subDelButton, *subAddButton, *subEditButton;
		QTreeView *subfunctionsView, *argumentsView;
		QStandardItemModel *subfunctionsModel, *argumentsModel;
		NamesEditDialog *namesEditDialog;
		MathFunction *o_function;
		bool name_edited;

	protected slots:

		void onNameEdited(const QString&);
		void onFunctionChanged();
		void subAddClicked();
		void subEditClicked();
		void subDelClicked();
		void argAddClicked();
		void argEditClicked();
		void argDelClicked();
		void argumentChanged(QStandardItem *item);
		void argumentActivated(const QModelIndex&);
		void selectedSubfunctionChanged(const QModelIndex&, const QModelIndex&);
		void selectedArgumentChanged(const QModelIndex&, const QModelIndex&);
		void onRejected();
		void editNames();

	public:

		FunctionEditDialog(QWidget *parent = NULL);
		virtual ~FunctionEditDialog();

		UserFunction *createFunction(MathFunction **replaced_item = NULL);
		bool modifyFunction(MathFunction *f, MathFunction **replaced_item = NULL);
		void setFunction(MathFunction *f);
		void setExpression(const QString&);
		QString expression() const;
		void setName(const QString&);

		static bool editFunction(QWidget *parent, MathFunction *f, MathFunction **replaced_item = NULL);
		static UserFunction *newFunction(QWidget *parent, MathFunction **replaced_item = NULL);

};

#endif //FUNCTION_EDIT_DIALOG_H

