/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QKeyEvent>
#include <QVector>
#include <QStandardItemModel>
#include <QTreeView>
#include <QHeaderView>
#include <QAction>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "functioneditdialog.h"

class MathTextEdit : public QPlainTextEdit {

	public:

		MathTextEdit(QWidget *parent) : QPlainTextEdit(parent) {
#ifndef _WIN32
			setAttribute(Qt::WA_InputMethodEnabled, settings->enable_input_method);
#endif
		}
		~MathTextEdit() {}

	protected:

		void keyPressEvent(QKeyEvent *event) override {
			if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::GroupSwitchModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
				switch(event->key()) {
					case Qt::Key_Asterisk: {
						insertPlainText(settings->multiplicationSign());
						return;
					}
					case Qt::Key_Slash: {
						insertPlainText(settings->divisionSign(false));
						return;
					}
					case Qt::Key_Minus: {
						insertPlainText(SIGN_MINUS);
						return;
					}
					case Qt::Key_Dead_Circumflex: {
						insertPlainText(settings->caret_as_xor ? " xor " : "^");
						return;
					}
					case Qt::Key_Dead_Tilde: {
						insertPlainText("~");
						return;
					}
					case Qt::Key_AsciiCircum: {
						if(settings->caret_as_xor) {
							insertPlainText(" xor ");
							return;
						}
						break;
					}
				}
			}
			if(event->key() == Qt::Key_Asterisk && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::KeypadModifier) || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
				insertPlainText("^");
				return;
			}
			QPlainTextEdit::keyPressEvent(event);
		}
};

#include <QProxyStyle>
#include <QStyleOptionViewItem>

class CenteredBoxProxy : public QProxyStyle {
	public:
		QRect subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const override {
			const QRect baseRes = QProxyStyle::subElementRect(element, option, widget);
			const QRect itemRect = option->rect;
			QRect retval = baseRes;
			QSize sz = baseRes.size();
			if(element == SE_ItemViewItemCheckIndicator) {
				int x = itemRect.x() + (itemRect.width()/2) - (baseRes.width()/2);
				retval = QRect( QPoint(x, baseRes.y()), sz);
			} else if(element == SE_ItemViewItemFocusRect) {
				sz.setWidth(baseRes.width()+baseRes.x());
				retval = QRect( QPoint(0, baseRes.y()), sz);
			}
			return retval;
		}
};

class SmallTreeView : public QTreeView {
	public:
		SmallTreeView(QWidget *parent) : QTreeView(parent) {}
		QSize sizeHint() const override {
			QSize size = QTreeView::sizeHint();
			QFontMetrics fm(font());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
			size.setHeight(fm.lineSpacing() * 4 + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + viewportMargins().bottom() + viewportMargins().top());
#else
			size.setHeight(fm.lineSpacing() * 4 + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom());
#endif
			return size;
		}
};

NamesEditDialog::NamesEditDialog(QWidget *parent, bool read_only) : QDialog(parent) {
	o_item = NULL;
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	namesView = new QTreeView(this);
	namesModel = new QStandardItemModel(this);
	namesView->setSelectionMode(QAbstractItemView::SingleSelection);
	namesView->setRootIsDecorated(false);
	namesView->setStyle(new CenteredBoxProxy());
	namesModel->setColumnCount(9);
	namesModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
	namesModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Abbreviation")));
	namesModel->setHorizontalHeaderItem(2, new QStandardItem(tr("Plural")));
	namesModel->setHorizontalHeaderItem(3, new QStandardItem(tr("Reference")));
	namesModel->setHorizontalHeaderItem(4, new QStandardItem(tr("Avoid input")));
	namesModel->setHorizontalHeaderItem(5, new QStandardItem(tr("Unicode")));
	namesModel->setHorizontalHeaderItem(6, new QStandardItem(tr("Suffix")));
	namesModel->setHorizontalHeaderItem(7, new QStandardItem(tr("Case sensitive")));
	namesModel->setHorizontalHeaderItem(8, new QStandardItem(tr("Completion only")));
	namesView->setModel(namesModel);
	namesView->header()->setStretchLastSection(false);
	namesView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	namesView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	grid->addWidget(namesView, 0, 0);
	QHBoxLayout *box = new QHBoxLayout();
	addButton = new QPushButton(tr("Add"), this); box->addWidget(addButton); connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked())); addButton->setEnabled(!read_only);
	editButton = new QPushButton(tr("Edit"), this); box->addWidget(editButton); connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked())); editButton->setEnabled(false);
	delButton = new QPushButton(tr("Remove"), this); box->addWidget(delButton); connect(delButton, SIGNAL(clicked()), this, SLOT(delClicked())); delButton->setEnabled(false);
	grid->addLayout(box, 1, 0, Qt::AlignRight);
	topbox->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(namesView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedNameChanged(const QModelIndex&, const QModelIndex&)));
	connect(namesModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(nameChanged(QStandardItem*)));
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
NamesEditDialog::~NamesEditDialog() {}

void NamesEditDialog::setNames(ExpressionItem *o, const QString &str) {
	o_item = o;
	bool read_only = !addButton->isEnabled();
	ExpressionName estr(str.trimmed().toStdString());
	for(size_t i = 1; i == 1 || (o && i <= o->countNames()); i++) {
		const ExpressionName *ename;
		if(o) ename = &o->getName(i);
		else ename = &estr;
		QList<QStandardItem *> items;
		QStandardItem *item = new QStandardItem(i == 1 && !str.trimmed().isEmpty() ? str.trimmed() : QString::fromStdString(ename->name));
		item->setEditable(!read_only);
		items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->abbreviation ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->plural ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->reference ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->avoid_input ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->unicode ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->suffix ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->case_sensitive ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(ename->completion_only ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		namesModel->appendRow(items);
	}
}
QString NamesEditDialog::firstName() {
	QStandardItem *item = namesModel->item(0, 0);
	if(!item) return QString();
	return item->text();
}
void NamesEditDialog::modifyNames(ExpressionItem *o, const QString &str) {
	o->clearNames();
	for(int i = 0; i < namesModel->rowCount(); i++) {
		ExpressionName ename;
		if(i == 0) ename.name = str.trimmed().toStdString();
		else ename.name = namesModel->item(i, 0)->text().trimmed().toStdString();
		if(!ename.name.empty()) {
			ename.abbreviation = (namesModel->item(i, 1)->checkState() == Qt::Checked);
			ename.plural = (namesModel->item(i, 2)->checkState() == Qt::Checked);
			ename.reference = (namesModel->item(i, 3)->checkState() == Qt::Checked);
			ename.avoid_input = (namesModel->item(i, 4)->checkState() == Qt::Checked);
			ename.unicode = (namesModel->item(i, 5)->checkState() == Qt::Checked);
			ename.suffix = (namesModel->item(i, 6)->checkState() == Qt::Checked);
			ename.case_sensitive = (namesModel->item(i, 7)->checkState() == Qt::Checked);
			ename.completion_only = (namesModel->item(i, 8)->checkState() == Qt::Checked);
			o->addName(ename);
		}
	}
	if(o->countNames() == 0) o->addName(str.toStdString());
}
void NamesEditDialog::addClicked() {
	QList<QStandardItem *> items;
	QStandardItem *item = new QStandardItem();
	QStandardItem *edit_item = item;
	items.append(item);
	for(int i = 0; i < 8; i++) {
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(Qt::Unchecked); item->setTextAlignment(Qt::AlignCenter); items.append(item);
	}
	namesModel->appendRow(items);
	namesView->selectionModel()->setCurrentIndex(edit_item->index(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear | QItemSelectionModel::Rows);
	namesView->scrollTo(edit_item->index());
	namesView->edit(edit_item->index());
}
void NamesEditDialog::editClicked() {
	namesView->edit(namesView->selectionModel()->currentIndex().siblingAtColumn(0));
}
void NamesEditDialog::delClicked() {
	namesModel->removeRow(namesView->selectionModel()->currentIndex().row());
}
void NamesEditDialog::selectedNameChanged(const QModelIndex &index, const QModelIndex&) {
	if(addButton->isEnabled()) {
		delButton->setEnabled(index.isValid());
		editButton->setEnabled(index.isValid());
	}
}
void NamesEditDialog::nameChanged(QStandardItem *item) {
	if(item->column() != 0 && !item->text().trimmed().isEmpty()) return;
	if((o_item->type() == TYPE_FUNCTION && !CALCULATOR->functionNameIsValid(item->text().trimmed().toStdString())) || (o_item->type() == TYPE_VARIABLE && !CALCULATOR->variableNameIsValid(item->text().trimmed().toStdString())) || (o_item->type() == TYPE_UNIT && !CALCULATOR->unitNameIsValid(item->text().trimmed().toStdString()))) {
		namesModel->blockSignals(true);
		item->setText(QString());
		namesModel->blockSignals(false);
		QMessageBox::warning(this, tr("Warning"), tr("Illegal name"));
	} else if(o_item->type() == TYPE_FUNCTION && CALCULATOR->functionNameTaken(item->text().trimmed().toStdString(), (MathFunction*) o_item)) {
		QMessageBox::warning(this, tr("Warning"), tr("A function with the same name already exists."));
	} else if(o_item->type() == TYPE_VARIABLE && CALCULATOR->variableNameTaken(item->text().trimmed().toStdString(), (Variable*) o_item)) {
		Variable *var = CALCULATOR->getActiveVariable(item->text().trimmed().toStdString());
		if(!var || var->category() != CALCULATOR->temporaryCategory()) QMessageBox::warning(this, tr("Warning"), tr("A unit or variable with the same name already exists."));
	} else if(o_item->type() == TYPE_UNIT && CALCULATOR->unitNameTaken(item->text().trimmed().toStdString(), (Unit*) o_item)) {
		QMessageBox::warning(this, tr("Warning"), tr("A unit or variable with the same name already exists."));
	}
}

FunctionEditDialog::FunctionEditDialog(QWidget *parent) : QDialog(parent) {
	namesEditDialog = NULL;
	o_function = NULL;
	name_edited = false;
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QTabWidget *tabs = new QTabWidget(this);
	tabs->setUsesScrollButtons(false);
	topbox->addWidget(tabs);
	QWidget *w1 = new QWidget(this);
	QWidget *w2 = new QWidget(this);
	QWidget *w3 = new QWidget(this);
	tabs->addTab(w1, tr("Required"));
	tabs->addTab(w3, tr("Details"));
	tabs->addTab(w2, tr("Description"));
	QGridLayout *grid = new QGridLayout(w1);
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	connect(nameEdit->addAction(LOAD_ICON("configure"), QLineEdit::TrailingPosition), SIGNAL(triggered()), this, SLOT(editNames()));
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
			nameEdit->setTextMargins(0, 0, 22, 0);
#	endif
#endif
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Expression:"), this), 1, 0, 1, 2);
	expressionEdit = new MathTextEdit(this);
	grid->addWidget(expressionEdit, 2, 0, 1, 2);
	QHBoxLayout *box = new QHBoxLayout();
	QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
	box->addWidget(new QLabel(tr("Argument references:"), this), 1);
	ref1Button = new QRadioButton(tr("x, y, z"), this); group->addButton(ref1Button, 1); box->addWidget(ref1Button);
	ref1Button->setChecked(true);
	ref2Button = new QRadioButton(tr("\\x, \\y, \\z, \\a, \\b, …"), this); group->addButton(ref2Button, 2); box->addWidget(ref2Button);
	grid->addLayout(box, 3, 0, 1, 2);
	grid = new QGridLayout(w2);
	grid->addWidget(new QLabel(tr("Category:"), this), 0, 0);
	categoryEdit = new QComboBox(this);
	std::unordered_map<std::string, bool> hash;
	QVector<MathFunction*> items;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(!CALCULATOR->functions[i]->category().empty()) {
			if(hash.find(CALCULATOR->functions[i]->category()) == hash.end()) {
				items.push_back(CALCULATOR->functions[i]);
				hash[CALCULATOR->functions[i]->category()] = true;
			}
		}
	}
	for(int i = 0; i < items.count(); i++) {
		categoryEdit->addItem(QString::fromStdString(items[i]->category()));
	}
	categoryEdit->setEditable(true);
	categoryEdit->setCurrentText(QString());
	grid->addWidget(categoryEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Descriptive name:"), this), 1, 0);
	titleEdit = new QLineEdit(this);
	grid->addWidget(titleEdit, 1, 1);
	hideBox = new QCheckBox(tr("Hide function"), this);
	grid->addWidget(hideBox, 2, 1, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("Example:"), this), 3, 0);
	exampleEdit = new QLineEdit(this);
	grid->addWidget(exampleEdit, 3, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 4, 0, 1, 2);
	descriptionEdit = new QPlainTextEdit(this);
	grid->addWidget(descriptionEdit, 5, 0, 1, 2);
	grid = new QGridLayout(w3);
	grid->addWidget(new QLabel(tr("Condition:"), this), 0, 0);
	conditionEdit = new QLineEdit(this);
	grid->addWidget(conditionEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Sub-functions:"), this), 1, 0);
	subfunctionsView = new SmallTreeView(this);
	subfunctionsModel = new QStandardItemModel(this);
	subfunctionsView->setSelectionMode(QAbstractItemView::SingleSelection);
	subfunctionsView->setRootIsDecorated(false);
	subfunctionsView->setStyle(new CenteredBoxProxy());
	subfunctionsModel->setColumnCount(3);
	subfunctionsModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Expression")));
	subfunctionsModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Precalculate")));
	subfunctionsModel->setHorizontalHeaderItem(2, new QStandardItem(tr("Reference")));
	subfunctionsView->setModel(subfunctionsModel);
	subfunctionsView->header()->setStretchLastSection(false);
	subfunctionsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	subfunctionsView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	grid->addWidget(subfunctionsView, 2, 0, 1, 2);
	box = new QHBoxLayout();
	subAddButton = new QPushButton(tr("Add"), this); box->addWidget(subAddButton); connect(subAddButton, SIGNAL(clicked()), this, SLOT(subAddClicked()));
	subEditButton = new QPushButton(tr("Edit"), this); box->addWidget(subEditButton); connect(subEditButton, SIGNAL(clicked()), this, SLOT(subEditClicked())); subEditButton->setEnabled(false);
	subDelButton = new QPushButton(tr("Remove"), this); box->addWidget(subDelButton); connect(subDelButton, SIGNAL(clicked()), this, SLOT(subDelClicked())); subDelButton->setEnabled(false);
	grid->addLayout(box, 3, 0, 1, 2, Qt::AlignRight);
	grid->addWidget(new QLabel(tr("Arguments:"), this), 4, 0);
	argumentsView = new SmallTreeView(this);
	argumentsModel = new QStandardItemModel(this);
	argumentsView->setSelectionMode(QAbstractItemView::SingleSelection);
	argumentsView->setRootIsDecorated(false);
	argumentsModel->setColumnCount(3);
	argumentsModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
	argumentsModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Type")));
	argumentsModel->setHorizontalHeaderItem(2, new QStandardItem(tr("Reference")));
	argumentsView->setModel(argumentsModel);
	argumentsView->header()->setStretchLastSection(false);
	argumentsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	argumentsView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
	argumentsView->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
	grid->addWidget(argumentsView, 5, 0, 1, 2);
	box = new QHBoxLayout();
	argAddButton = new QPushButton(tr("Add"), this); box->addWidget(argAddButton); connect(argAddButton, SIGNAL(clicked()), this, SLOT(argAddClicked()));
	argEditButton = new QPushButton(tr("Edit"), this); box->addWidget(argEditButton); connect(argEditButton, SIGNAL(clicked()), this, SLOT(argEditClicked())); argEditButton->setEnabled(false);
	argDelButton = new QPushButton(tr("Remove"), this); box->addWidget(argDelButton); connect(argDelButton, SIGNAL(clicked()), this, SLOT(argDelClicked())); argDelButton->setEnabled(false);
	grid->addLayout(box, 6, 0, 1, 2, Qt::AlignRight);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	okButton = buttonBox->button(QDialogButtonBox::Ok);
	topbox->addWidget(buttonBox);
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onNameEdited(const QString&)));
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onExpressionChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(subfunctionsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedSubfunctionChanged(const QModelIndex&, const QModelIndex&)));
	connect(argumentsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedArgumentChanged(const QModelIndex&, const QModelIndex&)));
	connect(this, SIGNAL(rejected()), this, SLOT(onRejected()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
FunctionEditDialog::~FunctionEditDialog() {}

void FunctionEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(this, expressionEdit->isReadOnly());
		namesEditDialog->setNames(o_function, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
}
UserFunction *FunctionEditDialog::createFunction(MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	MathFunction *func = NULL;
	if(name_edited && CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString())) {
		if(QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
			*replaced_item = func;
		}
	}
	UserFunction *f;
	if(func && func->isLocal() && func->subtype() == SUBTYPE_USER_FUNCTION) {
		f = (UserFunction*) func;
		if(f->countNames() > 1) f->clearNames();
		f->setHidden(false); f->setApproximate(false); f->setDescription(""); f->setCondition(""); f->setExample(""); f->clearArgumentDefinitions(); f->setTitle("");
		if(!modifyFunction(f)) return NULL;
		return f;
	}
	std::string str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
	if(ref1Button->isChecked()) {
		gsub("x", "\\x", str);
		gsub("y", "\\y", str);
		gsub("z", "\\z", str);
	}
	gsub(settings->multiplicationSign(), "*", str);
	gsub(settings->divisionSign(), "/", str);
	gsub(SIGN_MINUS, "-", str);
	f = new UserFunction("", nameEdit->text().trimmed().toStdString(), str);
	if(namesEditDialog) namesEditDialog->modifyNames(f, nameEdit->text());
	CALCULATOR->addFunction(f);
	return f;
}
bool FunctionEditDialog::modifyFunction(MathFunction *f, MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(name_edited && CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString(), f)) {
		if(QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			MathFunction *func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString());
			if(func != f) *replaced_item = func;
		}
	}
	f->setLocal(true);
	if(namesEditDialog) {
		namesEditDialog->modifyNames(f, nameEdit->text());
	} else {
		if(f->countNames() > 1 && f->getName(1).name != nameEdit->text().trimmed().toStdString()) f->clearNames();
		f->setName(nameEdit->text().trimmed().toStdString());
	}
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		std::string str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
		if(ref1Button->isChecked()) {
			gsub("x", "\\x", str);
			gsub("y", "\\y", str);
			gsub("z", "\\z", str);
		}
		gsub(settings->multiplicationSign(), "*", str);
		gsub(settings->divisionSign(), "/", str);
		gsub(SIGN_MINUS, "-", str);
		((UserFunction*) f)->setFormula(str);
	}
	return true;
}
void FunctionEditDialog::subAddClicked() {
	QList<QStandardItem *> items;
	QStandardItem *item = new QStandardItem();
	QStandardItem *edit_item = item;
	item->setEditable(true);
	items.append(item);
	item = new QStandardItem();
	item->setEditable(false);
	item->setCheckable(true);
	item->setCheckState(Qt::Unchecked);
	item->setTextAlignment(Qt::AlignCenter);
	items.append(item);
	item = new QStandardItem("\\" + QString::number(subfunctionsModel->rowCount() + 1));
	item->setEditable(false);
	item->setTextAlignment(Qt::AlignCenter);
	items.append(item);
	subfunctionsModel->appendRow(items);
	subfunctionsView->selectionModel()->setCurrentIndex(edit_item->index(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear | QItemSelectionModel::Rows);
	subfunctionsView->scrollTo(edit_item->index());
	subfunctionsView->edit(edit_item->index());
}
void FunctionEditDialog::subEditClicked() {
	subfunctionsView->edit(subfunctionsView->selectionModel()->currentIndex().siblingAtColumn(0));
}
void FunctionEditDialog::subDelClicked() {
	subfunctionsModel->removeRow(subfunctionsView->selectionModel()->currentIndex().row());
	for(int i = 0; i < subfunctionsModel->rowCount(); i++) {
		QStandardItem *item = subfunctionsModel->item(i, 2);
		if(item) {
			item->setText("\\" + QString::number(i + 1));
		}
	}
}
void FunctionEditDialog::argAddClicked() {
	QList<QStandardItem *> items;
	QStandardItem *item = new QStandardItem();
	item->setData(QVariant::fromValue((void*) NULL));
	QStandardItem *edit_item = item;
	item->setEditable(true);
	items.append(item);
	Argument defarg;
	item = new QStandardItem(QString::fromStdString(defarg.printlong()));
	item->setData(QVariant::fromValue((void*) NULL));
	item->setEditable(false);
	items.append(item);
	QString refstr = "\\";
	int i = argumentsModel->rowCount() + 1;
	if(i <= 3) refstr += 'x' + (i - 1);
	else refstr += 'a' + (i - 4);
	item = new QStandardItem(refstr);
	item->setData(QVariant::fromValue((void*) NULL));
	item->setEditable(false);
	item->setTextAlignment(Qt::AlignCenter);
	items.append(item);
	argumentsModel->appendRow(items);
	argumentsView->selectionModel()->setCurrentIndex(edit_item->index(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear | QItemSelectionModel::Rows);
	argumentsView->scrollTo(edit_item->index());
	argumentsView->edit(edit_item->index());
}
void FunctionEditDialog::argEditClicked() {
}
void FunctionEditDialog::argDelClicked() {
	argumentsModel->removeRow(argumentsView->selectionModel()->currentIndex().row());
	for(int i = 0; i < argumentsModel->rowCount(); i++) {
		QStandardItem *item = argumentsModel->item(i, 2);
		if(item) {
			QString refstr = "\\";
			if(i <= 3) refstr += 'x' + (i - 1);
			else refstr += 'a' + (i - 4);
			item->setText(refstr);
		}
	}
}
void FunctionEditDialog::selectedSubfunctionChanged(const QModelIndex &index, const QModelIndex&) {
	if(!expressionEdit->isReadOnly()) {
		subDelButton->setEnabled(index.isValid());
		subEditButton->setEnabled(index.isValid());
	}
}
void FunctionEditDialog::selectedArgumentChanged(const QModelIndex &index, const QModelIndex&) {
	if(!expressionEdit->isReadOnly()) {
		argDelButton->setEnabled(index.isValid());
		argEditButton->setEnabled(index.isValid());
	}
}
void FunctionEditDialog::onRejected() {
	if(expressionEdit->isReadOnly()) return;
	for(int i = 0; i < argumentsModel->rowCount(); i++) {
		QStandardItem *item = argumentsModel->item(i, 0);
		if(item) {
			Argument *arg = (Argument*) item->data(Qt::UserRole).value<void*>();
			if(arg) delete arg;
		}
	}
}
void FunctionEditDialog::setFunction(MathFunction *f) {
	o_function = f;
	bool read_only = !f->isLocal();
	nameEdit->setText(QString::fromStdString(f->getName(1).name));
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		expressionEdit->setEnabled(true);
		std::string str = CALCULATOR->localizeExpression(((UserFunction*) f)->formula(), settings->evalops.parse_options);
		gsub("*", settings->multiplicationSign(), str);
		gsub("/", settings->divisionSign(false), str);
		gsub("-", SIGN_MINUS, str);
		expressionEdit->setPlainText(QString::fromStdString(str));
		for(size_t i = 1; i <= ((UserFunction*) f)->countSubfunctions(); i++) {
			QList<QStandardItem *> items;
			QStandardItem *item = new QStandardItem(QString::fromStdString(((UserFunction*) f)->getSubfunction(i)));
			item->setData("<p>" + item->text() + "</p>", Qt::ToolTipRole);
			item->setEditable(!read_only);
			items.append(item);
			item = new QStandardItem();
			item->setEditable(false);
			item->setCheckable(true);
			item->setCheckState(((UserFunction*) f)->subfunctionPrecalculated(i) ? Qt::Checked : Qt::Unchecked);
			if(read_only) item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
			item->setTextAlignment(Qt::AlignCenter);
			items.append(item);
			item = new QStandardItem("\\" + QString::number(i));
			item->setEditable(false);
			item->setTextAlignment(Qt::AlignCenter);
			items.append(item);
			subfunctionsModel->appendRow(items);
		}
		int args = f->maxargs();
		if(args < 0) {
			args = f->minargs() + 1;
		}
		if((int) f->lastArgumentDefinitionIndex() > args) args = (int) f->lastArgumentDefinitionIndex();
		for(int i = 1; i <= args; i++) {
			Argument *arg = f->getArgumentDefinition(i);
			QString str, str2;
			if(arg) {
				if(!read_only) arg = arg->copy();
				str = QString::fromStdString(arg->printlong());
				str2 = QString::fromStdString(arg->name());
			} else {
				Argument defarg;
				str = QString::fromStdString(defarg.printlong());
			}
			QList<QStandardItem *> items;
			QStandardItem *item = new QStandardItem(str2);
			item->setData("<p>" + item->text() + "</p>", Qt::ToolTipRole);
			item->setData(QVariant::fromValue((void*) arg));
			item->setEditable(!read_only);
			items.append(item);
			item = new QStandardItem(str);
			item->setData("<p>" + item->text() + "</p>", Qt::ToolTipRole);
			item->setData(QVariant::fromValue((void*) arg));
			item->setEditable(false);
			items.append(item);
			QString refstr = "\\";
			if(i <= 3) refstr += 'x' + (i - 1);
			else refstr += 'a' + (i - 4);
			item = new QStandardItem(refstr);
			item->setData(QVariant::fromValue((void*) arg));
			item->setEditable(false);
			item->setTextAlignment(Qt::AlignCenter);
			items.append(item);
			argumentsModel->appendRow(items);
		}
	} else {
		read_only = true;
		expressionEdit->setEnabled(false);
		expressionEdit->clear();
		subfunctionsView->setEnabled(false);
		argumentsView->setEnabled(false);
	}
	descriptionEdit->setPlainText(QString::fromStdString(f->description()));
	exampleEdit->setText(QString::fromStdString(f->example(true)));
	titleEdit->setText(QString::fromStdString(f->title()));
	categoryEdit->setCurrentText(QString::fromStdString(f->category()));
	categoryEdit->setEnabled(!read_only);
	hideBox->setEnabled(!read_only);
	descriptionEdit->setReadOnly(read_only);
	exampleEdit->setReadOnly(read_only);
	titleEdit->setReadOnly(read_only);
	hideBox->setChecked(f->isHidden());
	okButton->setEnabled(!read_only);
	nameEdit->setReadOnly(read_only);
	subEditButton->setEnabled(false);
	subAddButton->setEnabled(!read_only);
	subDelButton->setEnabled(false);
	argEditButton->setEnabled(false);
	argAddButton->setEnabled(!read_only);
	argDelButton->setEnabled(false);
	conditionEdit->setReadOnly(read_only);
	ref1Button->setEnabled(!read_only);
	ref2Button->setEnabled(!read_only);
	expressionEdit->setReadOnly(read_only);
}
void FunctionEditDialog::onNameEdited(const QString &str) {
	okButton->setEnabled(!str.trimmed().isEmpty() && (!expressionEdit->isEnabled() || !expressionEdit->document()->isEmpty()));
	if(!str.trimmed().isEmpty() && !CALCULATOR->functionNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidFunctionName(str.trimmed().toStdString())));
	}
	name_edited = true;
}
void FunctionEditDialog::onExpressionChanged() {
	okButton->setEnabled((!expressionEdit->document()->isEmpty() || !expressionEdit->isEnabled()) && !nameEdit->text().trimmed().isEmpty());
}
void FunctionEditDialog::setExpression(const QString &str) {
	expressionEdit->setPlainText(str);
	onExpressionChanged();
}
void FunctionEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
QString FunctionEditDialog::expression() const {
	return expressionEdit->toPlainText();
}
void FunctionEditDialog::setRefType(int i) {
	if(i == 1) ref1Button->setChecked(true);
	else if(i == 2) ref2Button->setChecked(true);
}
bool FunctionEditDialog::editFunction(QWidget *parent, MathFunction *f, MathFunction **replaced_item) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
	d->setRefType(2);
	d->setWindowTitle(tr("Edit Function"));
	d->setFunction(f);
	while(d->exec() == QDialog::Accepted) {
		if(d->modifyFunction(f, replaced_item)) {
			d->deleteLater();
			return true;
		}
	}
	d->deleteLater();
	return false;
}
UserFunction* FunctionEditDialog::newFunction(QWidget *parent, MathFunction **replaced_item) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
	d->setWindowTitle(tr("New Function"));
	d->setRefType(1);
	std::string f_name;
	int i = 1;
	do {
		f_name = "f"; f_name += i2s(i);
		i++;
	} while(CALCULATOR->nameTaken(f_name));
	d->setName(QString::fromStdString(f_name));
	UserFunction *f = NULL;
	while(d->exec() == QDialog::Accepted) {
		f = d->createFunction(replaced_item);
		if(f) break;
	}
	d->deleteLater();
	return f;
}

