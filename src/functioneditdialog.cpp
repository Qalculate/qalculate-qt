/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLineEdit>
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
#include <QMap>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QHeaderView>
#include <QAction>
#include <QDoubleSpinBox>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#	include <QScreen>
#else
#	include <QDesktopWidget>
#endif
#include <QApplication>
#include <QDebug>

#include "functioneditdialog.h"
#include "qalculateqtsettings.h"

SmallTextEdit::SmallTextEdit(int r, QWidget *parent) : QPlainTextEdit(parent), i_rows(r) {}
SmallTextEdit::~SmallTextEdit() {}

QSize SmallTextEdit::sizeHint() const {
	QSize size = QPlainTextEdit::sizeHint();
	QFontMetrics fm(font());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	size.setHeight(fm.lineSpacing() * i_rows + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + document()->documentMargin() * 2 + viewportMargins().bottom() + viewportMargins().top());
#else
	size.setHeight(fm.lineSpacing() * i_rows + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + document()->documentMargin());
#endif
	return size;
}

MathTextEdit::MathTextEdit(QWidget *parent) : QPlainTextEdit(parent) {
#ifndef _WIN32
	setAttribute(Qt::WA_InputMethodEnabled, settings->enable_input_method);
#endif
}
MathTextEdit::~MathTextEdit() {}

QSize MathTextEdit::sizeHint() const {
	QSize size = QPlainTextEdit::sizeHint();
	QFontMetrics fm(font());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
	size.setHeight(fm.lineSpacing() * 3 + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + document()->documentMargin() * 2 + viewportMargins().bottom() + viewportMargins().top());
#else
	size.setHeight(fm.lineSpacing() * 3 + frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom() + document()->documentMargin());
#endif
	return size;
}
void MathTextEdit::keyPressEvent(QKeyEvent *event) {
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

ExpandingMathLineEdit::ExpandingMathLineEdit(QWidget *parent) : QLineEdit(parent), originalWidth(-1), widgetOwnsGeometry(false) {
	connect(this, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()));
	updateMinimumWidth();
}

void ExpandingMathLineEdit::setWidgetOwnsGeometry(bool value) {
	widgetOwnsGeometry = value;
}

void ExpandingMathLineEdit::changeEvent(QEvent *e) {
	switch(e->type()) {
		case QEvent::FontChange:
		case QEvent::StyleChange:
		case QEvent::ContentsRectChange:
			updateMinimumWidth();
			break;
		default:
			break;
	}

	QLineEdit::changeEvent(e);
}

void ExpandingMathLineEdit::keyPressEvent(QKeyEvent *event) {
	if(event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::GroupSwitchModifier || event->modifiers() == Qt::ShiftModifier || event->modifiers() == Qt::KeypadModifier) {
		switch(event->key()) {
			case Qt::Key_Asterisk: {
				insert(settings->multiplicationSign(false));
				return;
			}
			case Qt::Key_Minus: {
				insert(SIGN_MINUS);
				return;
			}
			case Qt::Key_Dead_Circumflex: {
				insert(settings->caret_as_xor ? " xor " : "^");
				return;
			}
			case Qt::Key_Dead_Tilde: {
				insert("~");
				return;
			}
			case Qt::Key_AsciiCircum: {
				if(settings->caret_as_xor) {
					insert(" xor ");
					return;
				}
				break;
			}
		}
	}
	if(event->key() == Qt::Key_Asterisk && (event->modifiers() == Qt::ControlModifier || event->modifiers() == (Qt::ControlModifier | Qt::KeypadModifier) || event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))) {
		insert("^");
		return;
	}
	QLineEdit::keyPressEvent(event);
	if(event->key() == Qt::Key_Return) event->accept();
}

void ExpandingMathLineEdit::updateMinimumWidth() {

	const QMargins tm = textMargins();
	const QMargins cm = contentsMargins();
	const int width = tm.left() + tm.right() + cm.left() + cm.right() + 4;

	QStyleOptionFrame opt;
	initStyleOption(&opt);

	int minWidth = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(width, 0), this).width();
	setMinimumWidth(minWidth);
}

void ExpandingMathLineEdit::resizeToContents() {
	int oldWidth = width();
	if(originalWidth == -1)
		originalWidth = oldWidth;
	if(QWidget *parent = parentWidget()) {
		QPoint position = pos();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
		int hintWidth = minimumWidth() + fontMetrics().horizontalAdvance(displayText());
#else
		int hintWidth = minimumWidth() + fontMetrics().boundingRect(displayText()).width();
#endif
		int parentWidth = parent->width();
		int maxWidth = isRightToLeft() ? position.x() + oldWidth : parentWidth - position.x();
		int newWidth = qBound(originalWidth, hintWidth, maxWidth);
		if(widgetOwnsGeometry) setMaximumWidth(newWidth);
		if(isRightToLeft()) move(position.x() - newWidth + oldWidth, position.y());
		resize(newWidth, height());
	}
}

class SubfunctionDelegate : public QStyledItemDelegate {
	public:
		SubfunctionDelegate(QWidget *parent) : QStyledItemDelegate(parent) {}
		QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem&, const QModelIndex&) const override {
			return new ExpandingMathLineEdit(parent);
		}
		void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
			if(!editor) return;
			Q_ASSERT(index.isValid());
			QStyleOptionViewItem opt = option;
			initStyleOption(&opt, index);
			opt.showDecorationSelected = editor->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, nullptr, editor);
			QStyle *style = QApplication::style();
			QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, (QWidget*) parent());
			editor->setGeometry(geom);
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

NamesEditDialog::NamesEditDialog(int type, QWidget *parent, bool read_only) : QDialog(parent), i_type(type) {
	o_item = NULL;
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	namesView = new QTreeView(this);
	namesModel = new QStandardItemModel(this);
	namesView->setSelectionMode(QAbstractItemView::SingleSelection);
	namesView->setRootIsDecorated(false);
	namesView->setStyle(new CenteredBoxProxy());
	if(i_type < 0) {
		namesModel->setColumnCount(2);
		namesModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Name")));
		namesModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Reference")));
	} else {
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#	if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
		QScreen *scr = screen();
#	else
		QScreen *scr = QGuiApplication::screenAt(pos);
#	endif
		if(!scr) scr = QGuiApplication::primaryScreen();
		QRect screen = scr->geometry();
#else
		QRect screen = QApplication::desktop()->screenGeometry(this);
#endif
		if(screen.width() > 800) namesView->setMinimumWidth(800);
	}
	namesView->setModel(namesModel);
	namesView->header()->setStretchLastSection(false);
	namesView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	namesView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	if(i_type >= 0 && i_type != TYPE_UNIT) namesView->header()->hideSection(2);
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
	estr.reference = true;
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
		if(!read_only && i == 1 && !item->text().isEmpty() && (!o || QString::fromStdString(ename->name) != item->text())) nameChanged(item);
	}
}
void NamesEditDialog::setName(const QString &str) {
	if(str.trimmed().isEmpty()) return;
	QStandardItem *item = namesModel->item(0, 0);
	if(!item) {
		item = new QStandardItem();
		QList<QStandardItem *> items;
		items.append(item);
		for(int i = 1; i < namesModel->columnCount(); i++) {
			QStandardItem *item2 = new QStandardItem(); item2->setEditable(false); item2->setCheckable(true); item2->setCheckState(Qt::Unchecked); item2->setTextAlignment(Qt::AlignCenter); items.append(item2);
		}
		namesModel->appendRow(items);
	}
	if(str.trimmed() != item->text()) item->setText(str.trimmed());
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
	if(o->countNames() == 0) {
		ExpressionName ename(str.trimmed().toStdString());
		ename.reference = true;
		o->addName(ename);
	}
}
void NamesEditDialog::modifyName(ExpressionItem *o, const QString &str) {
	if(o->countNames() == 0) {
		ExpressionName ename(str.trimmed().toStdString());
		ename.reference = true;
		o->addName(ename);
	} else {
		o->setName(str.trimmed().toStdString());
	}
}
void NamesEditDialog::setNames(DataProperty *o, const QString &str) {
	bool read_only = !addButton->isEnabled();
	for(size_t i = 1; i == 1 || (o && i <= o->countNames()); i++) {
		QList<QStandardItem *> items;
		QStandardItem *item = new QStandardItem(i == 1 && (!o || !str.trimmed().isEmpty()) ? str.trimmed() : QString::fromStdString(o->getName(i)));
		item->setEditable(!read_only);
		items.append(item);
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState((!o || o->nameIsReference(i)) ? Qt::Checked : Qt::Unchecked); if(read_only) {item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);} item->setTextAlignment(Qt::AlignCenter); items.append(item);
		namesModel->appendRow(items);
	}
}
void NamesEditDialog::modifyNames(DataProperty *o, const QString &str) {
	o->clearNames();
	for(int i = 0; i < namesModel->rowCount(); i++) {
		if(i == 0) o->addName(str.trimmed().toStdString(), namesModel->item(i, 1)->checkState() == Qt::Checked);
		else o->addName(namesModel->item(i, 0)->text().trimmed().toStdString(), namesModel->item(i, 1)->checkState() == Qt::Checked);
	}
	if(o->countNames() == 0) {
		o->addName(str.trimmed().toStdString(), true);
	}
}
void NamesEditDialog::modifyName(DataProperty *o, const QString &str) {
	if(o->countNames() == 0) {
		o->addName(str.trimmed().toStdString(), true);
	} else {
		std::vector<std::string> names;
		std::vector<bool> name_refs;
		for(size_t i = 1; i <= o->countNames(); i++) {
			if(i == 1) names.push_back(str.trimmed().toStdString());
			else names.push_back(o->getName(i));
			name_refs.push_back(o->nameIsReference(i));
		}
		o->clearNames();
		for(size_t i = 0; i < names.size(); i++) {
			o->addName(names[i], name_refs[i]);
		}
	}
}
void NamesEditDialog::addClicked() {
	QList<QStandardItem *> items;
	QStandardItem *item = new QStandardItem();
	QStandardItem *edit_item = item;
	items.append(item);
	for(int i = 1; i < namesModel->columnCount(); i++) {
		item = new QStandardItem(); item->setEditable(false); item->setCheckable(true); item->setCheckState(Qt::Unchecked); item->setTextAlignment(Qt::AlignCenter); items.append(item);
	}
	namesModel->appendRow(items);
	namesView->selectionModel()->setCurrentIndex(edit_item->index(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear | QItemSelectionModel::Rows);
	namesView->scrollTo(edit_item->index());
	namesView->edit(edit_item->index());
}
void NamesEditDialog::editClicked() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	namesView->edit(namesView->selectionModel()->currentIndex().siblingAtColumn(0));
#else
	namesView->edit(namesView->selectionModel()->currentIndex().sibling(namesView->selectionModel()->currentIndex().row(), 0));
#endif
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
	if(i_type < 0 || item->column() != 0 || item->text().trimmed().isEmpty()) return;
	if((i_type == TYPE_FUNCTION && !CALCULATOR->functionNameIsValid(item->text().trimmed().toStdString())) || (i_type == TYPE_VARIABLE && !CALCULATOR->variableNameIsValid(item->text().trimmed().toStdString())) || (i_type == TYPE_UNIT && !CALCULATOR->unitNameIsValid(item->text().trimmed().toStdString()))) {
		namesModel->blockSignals(true);
		if(i_type == TYPE_FUNCTION) item->setText(QString::fromStdString(CALCULATOR->convertToValidFunctionName(item->text().trimmed().toStdString())));
		else if(i_type == TYPE_VARIABLE) item->setText(QString::fromStdString(CALCULATOR->convertToValidVariableName(item->text().trimmed().toStdString())));
		else if(i_type == TYPE_UNIT) item->setText(QString::fromStdString(CALCULATOR->convertToValidUnitName(item->text().trimmed().toStdString())));
		namesModel->blockSignals(false);
		QMessageBox::warning(this, tr("Warning"), tr("Illegal name"));
	} else if(i_type == TYPE_FUNCTION && CALCULATOR->functionNameTaken(item->text().trimmed().toStdString(), (MathFunction*) o_item)) {
		MathFunction *f = CALCULATOR->getActiveFunction(item->text().trimmed().toStdString(), true);
		if(!f || f->category() != CALCULATOR->temporaryCategory()) QMessageBox::warning(this, tr("Warning"), tr("A function with the same name already exists."));
	} else if(i_type == TYPE_VARIABLE && CALCULATOR->variableNameTaken(item->text().trimmed().toStdString(), (Variable*) o_item)) {
		Variable *var = CALCULATOR->getActiveVariable(item->text().trimmed().toStdString(), true);
		if(!var || var->category() != CALCULATOR->temporaryCategory()) QMessageBox::warning(this, tr("Warning"), tr("A unit or variable with the same name already exists."));
	} else if(i_type == TYPE_UNIT && CALCULATOR->unitNameTaken(item->text().trimmed().toStdString(), (Unit*) o_item)) {
		Unit *u = CALCULATOR->getActiveUnit(item->text().trimmed().toStdString(), true);
		if(!u || u->category() != CALCULATOR->temporaryCategory()) QMessageBox::warning(this, tr("Warning"), tr("A unit or variable with the same name already exists."));
	}
}

ArgumentEditDialog::ArgumentEditDialog(QWidget *parent, bool read_only) : QDialog(parent) {
	QVBoxLayout *topbox = new QVBoxLayout(this);
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Name:"), this), 0, 0);
	nameEdit = new QLineEdit(this);
	nameEdit->setReadOnly(read_only);
	grid->addWidget(nameEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Type:"), this), 1, 0);
	typeCombo = new QComboBox(this);
	typeCombo->setEditable(false);
	typeCombo->addItem(tr("Free"), ARGUMENT_TYPE_FREE);
	typeCombo->addItem(tr("Number"), ARGUMENT_TYPE_NUMBER);
	typeCombo->addItem(tr("Integer"), ARGUMENT_TYPE_INTEGER);
	typeCombo->addItem(tr("Symbol"), ARGUMENT_TYPE_SYMBOLIC);
	typeCombo->addItem(tr("Text"), ARGUMENT_TYPE_TEXT);
	typeCombo->addItem(tr("Date"), ARGUMENT_TYPE_DATE);
	typeCombo->addItem(tr("Vector"), ARGUMENT_TYPE_VECTOR);
	typeCombo->addItem(tr("Matrix"), ARGUMENT_TYPE_MATRIX);
	typeCombo->addItem(tr("Boolean"), ARGUMENT_TYPE_BOOLEAN);
	typeCombo->addItem(tr("Angle"), ARGUMENT_TYPE_ANGLE);
	typeCombo->addItem(tr("Object"), ARGUMENT_TYPE_EXPRESSION_ITEM);
	typeCombo->addItem(tr("Function"), ARGUMENT_TYPE_FUNCTION);
	typeCombo->addItem(tr("Unit"), ARGUMENT_TYPE_UNIT);
	typeCombo->addItem(tr("Variable"), ARGUMENT_TYPE_VARIABLE);
	typeCombo->addItem(tr("File"), ARGUMENT_TYPE_FILE);
	grid->addWidget(typeCombo, 1, 1);
	testBox = new QCheckBox(tr("Enable rules and type test"), this);
	grid->addWidget(testBox, 2, 0, 1, 2);
	grid->addWidget(new QLabel(tr("Custom condition:"), this), 3, 0);
	conditionEdit = new MathLineEdit(this);
	conditionEdit->setToolTip("<div>" + tr("For example if argument is a matrix that must have equal number of rows and columns: rows(\\x) = columns(\\x)") + "</div>");
	grid->addWidget(conditionEdit, 3, 1);
	matrixBox = new QCheckBox(tr("Allow matrix"), this);
	grid->addWidget(matrixBox, 4, 0, 1, 2);
	zeroBox = new QCheckBox(tr("Forbid zero"), this);
	grid->addWidget(zeroBox, 5, 0, 1, 2);
	vectorBox = new QCheckBox(tr("Handle vector"), this);
	vectorBox->setToolTip("<div>" + tr("Calculate function for each separate element in vector.") + "</div>");
	grid->addWidget(vectorBox, 6, 0, 1, 2);
	minBox = new QCheckBox(tr("Min"), this);
	grid->addWidget(minBox, 7, 0);
	minEdit = new QDoubleSpinBox(this);
	grid->addWidget(minEdit, 7, 1);
	includeMinBox = new QCheckBox(tr("Include equals"), this);
	grid->addWidget(includeMinBox, 8, 1, Qt::AlignRight);
	maxBox = new QCheckBox(tr("Max"), this);
	grid->addWidget(maxBox, 9, 0);
	maxEdit = new QDoubleSpinBox(this);
	minEdit->setDecimals(8);
	minEdit->setRange(INT_MIN, INT_MAX);
	maxEdit->setDecimals(8);
	maxEdit->setRange(INT_MIN, INT_MAX);
	grid->addWidget(maxEdit, 9, 1);
	includeMaxBox = new QCheckBox(tr("Include equals"), this);
	grid->addWidget(includeMaxBox, 10, 1, Qt::AlignRight);
	topbox->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
	buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));
	connect(typeCombo, SIGNAL(activated(int)), this, SLOT(typeChanged(int)));
	connect(minBox, SIGNAL(toggled(bool)), this, SLOT(minToggled(bool)));
	connect(maxBox, SIGNAL(toggled(bool)), this, SLOT(maxToggled(bool)));
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
ArgumentEditDialog::~ArgumentEditDialog() {}

void ArgumentEditDialog::setArgument(Argument *arg) {
	testBox->setChecked(!arg || arg->tests());
	matrixBox->setChecked(!arg || arg->matrixAllowed() || arg->type() == ARGUMENT_TYPE_FREE || arg->type() == ARGUMENT_TYPE_MATRIX);
	matrixBox->setEnabled(arg && arg->type() != ARGUMENT_TYPE_FREE && arg->type() != ARGUMENT_TYPE_MATRIX);
	if(arg) {
		std::string str = settings->localizeExpression(arg->getCustomCondition());
		conditionEdit->setText(QString::fromStdString(str));
	} else {
		conditionEdit->clear();
	}
	nameEdit->setText(arg ? QString::fromStdString(arg->name()) : QString());
	zeroBox->setChecked(arg && arg->zeroForbidden());
	vectorBox->setChecked(arg && arg->handlesVector());
	switch(arg ? arg->type() : ARGUMENT_TYPE_FREE) {
		case ARGUMENT_TYPE_NUMBER: {
			NumberArgument *farg = (NumberArgument*) arg;
			minBox->setEnabled(true);
			minBox->setChecked(farg->min() != NULL);
			includeMinBox->setChecked(farg->includeEqualsMin());
			includeMinBox->setEnabled(true);
			minEdit->setEnabled(farg->min() != NULL);
			minEdit->setDecimals(8);
			if(farg->min()) {
				minEdit->setValue(farg->min()->floatValue());
			} else {
				minEdit->setValue(0.0);
			}
			maxBox->setEnabled(true);
			maxBox->setChecked(farg->max() != NULL);
			includeMaxBox->setChecked(farg->includeEqualsMax());
			includeMaxBox->setEnabled(true);
			maxEdit->setEnabled(farg->max() != NULL);
			maxEdit->setDecimals(8);
			if(farg->max()) {
				maxEdit->setValue(farg->max()->floatValue());
			} else {
				maxEdit->setValue(0.0);
			}
			break;
		}
		case ARGUMENT_TYPE_INTEGER: {
			NumberArgument *farg = (NumberArgument*) arg;
			minBox->setEnabled(true);
			minBox->setChecked(farg->min() != NULL);
			includeMinBox->setChecked(true);
			includeMinBox->setEnabled(false);
			minEdit->setEnabled(farg->min() != NULL);
			minEdit->setDecimals(0);
			if(farg->min()) {
				minEdit->setValue(farg->min()->intValue());
			} else {
				minEdit->setValue(0.0);
			}
			maxBox->setEnabled(true);
			maxBox->setChecked(farg->max() != NULL);
			includeMaxBox->setChecked(true);
			includeMaxBox->setEnabled(false);
			maxEdit->setEnabled(farg->max() != NULL);
			maxEdit->setDecimals(0);
			if(farg->max()) {
				maxEdit->setValue(farg->max()->intValue());
			} else {
				maxEdit->setValue(0.0);
			}
			break;
		}
		default: {
			minBox->setChecked(false);
			minEdit->setValue(0.0);
			minEdit->setEnabled(false);
			includeMinBox->setEnabled(false);
			includeMinBox->setChecked(true);
			minBox->setEnabled(false);
			maxBox->setChecked(false);
			maxEdit->setValue(0.0);
			maxEdit->setEnabled(false);
			includeMaxBox->setEnabled(false);
			includeMaxBox->setChecked(true);
			maxBox->setEnabled(false);
		}
	}
	typeCombo->setCurrentIndex(typeCombo->findData(arg ? arg->type() : 0));
}
Argument *ArgumentEditDialog::createArgument() {
	Argument *arg = NULL;
	switch(typeCombo->currentData().toInt()) {
		case ARGUMENT_TYPE_NUMBER: {
			arg = new NumberArgument();
			if(minBox->isChecked()) {
				QString str = minEdit->cleanText();
				int i = str.indexOf(",");
				if(i > 0) {
					if(str.indexOf(".", i) > 0) {
						str.remove(",");
					} else {
						str.remove(".");
						str.replace(",", ".");
					}
				}
				Number nr(str.toStdString());
				((NumberArgument*) arg)->setMin(&nr);
				((NumberArgument*) arg)->setIncludeEqualsMin(includeMinBox->isChecked());
			}
			if(maxBox->isChecked()) {
				QString str = minEdit->cleanText();
				int i = str.indexOf(",");
				if(i > 0) {
					if(str.indexOf(".", i) > 0) {
						str.remove(",");
					} else {
						str.remove(".");
						str.replace(",", ".");
					}
				}
				Number nr(str.toStdString());
				((NumberArgument*) arg)->setMax(&nr);
				((NumberArgument*) arg)->setIncludeEqualsMax(includeMaxBox->isChecked());
			}
			break;
		}
		case ARGUMENT_TYPE_INTEGER: {
			arg = new IntegerArgument();
			if(minBox->isChecked()) {
				QString str = minEdit->cleanText();
				str.remove(",");
				str.remove(".");
				Number nr(str.toStdString());
				((IntegerArgument*) arg)->setMin(&nr);
			}
			if(maxBox->isChecked()) {
				QString str = maxEdit->cleanText();
				str.remove(",");
				str.remove(".");
				Number nr(str.toStdString());
				((IntegerArgument*) arg)->setMax(&nr);
			}
			break;
		}
		case ARGUMENT_TYPE_SYMBOLIC: {arg = new SymbolicArgument(); break;}
		case ARGUMENT_TYPE_TEXT: {arg = new TextArgument(); break;}
		case ARGUMENT_TYPE_DATE: {arg = new DateArgument(); break;}
		case ARGUMENT_TYPE_FILE: {arg = new FileArgument(); break;}
		case ARGUMENT_TYPE_VECTOR: {arg = new VectorArgument(); break;}
		case ARGUMENT_TYPE_MATRIX: {arg = new MatrixArgument(); break;}
		case ARGUMENT_TYPE_EXPRESSION_ITEM: {arg = new ExpressionItemArgument(); break;}
		case ARGUMENT_TYPE_FUNCTION: {arg = new FunctionArgument(); break;}
		case ARGUMENT_TYPE_UNIT: {arg = new UnitArgument(); break;}
		case ARGUMENT_TYPE_VARIABLE: {arg = new VariableArgument(); break;}
		case ARGUMENT_TYPE_BOOLEAN: {arg = new BooleanArgument(); break;}
		case ARGUMENT_TYPE_ANGLE: {arg = new AngleArgument(); break;}
		default: {
			arg = new Argument();
		}
	}
	arg->setName(nameEdit->text().trimmed().toStdString());
	std::string str = settings->unlocalizeExpression(conditionEdit->text().trimmed().toStdString());
	arg->setCustomCondition(str);
	arg->setTests(testBox->isChecked());
	arg->setAlerts(testBox->isChecked());
	arg->setZeroForbidden(zeroBox->isChecked());
	arg->setMatrixAllowed(matrixBox->isChecked());
	return arg;
}
void ArgumentEditDialog::typeChanged(int index) {
	int i = typeCombo->itemData(index).toInt();
	minEdit->setValue(0.0);
	maxEdit->setValue(0.0);
	minEdit->setEnabled(false);
	maxEdit->setEnabled(false);
	minBox->setChecked(false);
	maxBox->setChecked(false);
	if(i == ARGUMENT_TYPE_INTEGER) minEdit->setDecimals(0);
	else if(i == ARGUMENT_TYPE_NUMBER) minEdit->setDecimals(8);
	if(i == ARGUMENT_TYPE_INTEGER) maxEdit->setDecimals(0);
	else if(i == ARGUMENT_TYPE_NUMBER) maxEdit->setDecimals(8);
	includeMaxBox->setChecked(true);
	includeMinBox->setChecked(true);
	includeMaxBox->setEnabled(i == ARGUMENT_TYPE_NUMBER);
	includeMinBox->setEnabled(i == ARGUMENT_TYPE_NUMBER);
	minBox->setEnabled(i == ARGUMENT_TYPE_NUMBER || i == ARGUMENT_TYPE_INTEGER);
	maxBox->setEnabled(i == ARGUMENT_TYPE_NUMBER || i == ARGUMENT_TYPE_INTEGER);
	vectorBox->setChecked(i == ARGUMENT_TYPE_NUMBER || i == ARGUMENT_TYPE_INTEGER);
	matrixBox->setEnabled(i != ARGUMENT_TYPE_FREE && i != ARGUMENT_TYPE_MATRIX);
	matrixBox->setChecked(i == ARGUMENT_TYPE_FREE || i == ARGUMENT_TYPE_MATRIX);
}
void ArgumentEditDialog::minToggled(bool b) {
	minEdit->setEnabled(b);
	includeMinBox->setEnabled(typeCombo->currentData().toInt() == ARGUMENT_TYPE_NUMBER);
}
void ArgumentEditDialog::maxToggled(bool b) {
	maxEdit->setEnabled(b);
	includeMaxBox->setEnabled(typeCombo->currentData().toInt() == ARGUMENT_TYPE_NUMBER);
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
	expressionEdit->setToolTip(tr("Use x, y, and z (e.g. \"(x+y)/2\"), or\n\\x, \\y, \\z, \\a, \\b, … (e.g. \"(\\x+\\y)/2\")"));
	grid->addWidget(expressionEdit, 2, 0, 1, 2);
	QHBoxLayout *box = new QHBoxLayout();
	QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
	grid = new QGridLayout(w2);
	grid->addWidget(new QLabel(tr("Category:"), this), 0, 0);
	categoryEdit = new QComboBox(this);
	QMap<std::string, bool> hash;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(!CALCULATOR->functions[i]->category().empty()) {
			if(hash.find(CALCULATOR->functions[i]->category()) == hash.end()) {
				hash[CALCULATOR->functions[i]->category()] = true;
			}
		}
	}
	for(QMap<std::string, bool>::const_iterator it = hash.constBegin(); it != hash.constEnd(); ++it) {
		categoryEdit->addItem(QString::fromStdString(it.key()));
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
	exampleEdit = new MathLineEdit(this);
	grid->addWidget(exampleEdit, 3, 1);
	grid->addWidget(new QLabel(tr("Description:"), this), 4, 0, 1, 2);
	descriptionEdit = new QPlainTextEdit(this);
	grid->addWidget(descriptionEdit, 5, 0, 1, 2);
	grid = new QGridLayout(w3);
	grid->addWidget(new QLabel(tr("Condition:"), this), 0, 0);
	conditionEdit = new MathLineEdit(this);
	conditionEdit->setToolTip(tr("Condition that must be true for the function (e.g. if the second argument must be greater than the first: \"\\y > \\x\")"));
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
	SubfunctionDelegate* delegate = new SubfunctionDelegate(subfunctionsView);
	subfunctionsView->setItemDelegateForColumn(0, delegate);
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
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onFunctionChanged()));
	connect(descriptionEdit, SIGNAL(textChanged()), this, SLOT(onFunctionChanged()));
	connect(conditionEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onFunctionChanged()));
	connect(hideBox, SIGNAL(clicked()), this, SLOT(onFunctionChanged()));
	connect(titleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onFunctionChanged()));
	connect(exampleEdit, SIGNAL(textEdited(const QString&)), this, SLOT(onFunctionChanged()));
	connect(categoryEdit, SIGNAL(currentTextChanged(const QString&)), this, SLOT(onFunctionChanged()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(subfunctionsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedSubfunctionChanged(const QModelIndex&, const QModelIndex&)));
	connect(argumentsView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectedArgumentChanged(const QModelIndex&, const QModelIndex&)));
	connect(argumentsView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(argumentActivated(const QModelIndex&)));
	connect(argumentsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(argumentChanged(QStandardItem*)));
	connect(subfunctionsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onFunctionChanged()));
	connect(this, SIGNAL(rejected()), this, SLOT(onRejected()));
	okButton->setEnabled(false);
	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}
FunctionEditDialog::~FunctionEditDialog() {}

void FunctionEditDialog::editNames() {
	if(!namesEditDialog) {
		namesEditDialog = new NamesEditDialog(TYPE_FUNCTION, this, nameEdit->isReadOnly());
		namesEditDialog->setNames(o_function, nameEdit->text());
	}
	namesEditDialog->exec();
	nameEdit->setText(namesEditDialog->firstName());
	name_edited = false;
	onFunctionChanged();
}

void fix_expression(std::string &str) {
	if(str.empty()) return;
	size_t i = 0;
	bool b = false;
	while(true) {
		i = str.find("\\", i);
		if(i == std::string::npos || i == str.length() - 1) break;
		if((str[i + 1] >= 'a' && str[i + 1] <= 'z') || (str[i + 1] >= 'A' && str[i + 1] <= 'Z') || (str[i + 1] >= '1' && str[i + 1] <= '9')) {
			b = true;
			break;
		}
		i++;
	}
	if(!b) {
		gsub("x", "\\x", str);
		gsub("y", "\\y", str);
		gsub("z", "\\z", str);
	}
	CALCULATOR->parseSigns(str);
}

UserFunction *FunctionEditDialog::createFunction(MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	MathFunction *func = NULL;
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString())) {
		func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString(), true);
		if(name_edited && (!func || func->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return NULL;
		}
		if(replaced_item) {
			*replaced_item = func;
		}
	}
	UserFunction *f;
	if(func && func->isLocal() && func->subtype() == SUBTYPE_USER_FUNCTION) {
		f = (UserFunction*) func;
		f->clearNames();
		f->setApproximate(false);
		if(!modifyFunction(f)) return NULL;
		return f;
	}
	f = new UserFunction("", "", "");
	std::string str;
	ParseOptions pa = settings->evalops.parse_options;
	pa.base = 10;
	f->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	f->setTitle(titleEdit->text().trimmed().toStdString());
	f->setCategory(categoryEdit->currentText().trimmed().toStdString());
	f->setHidden(hideBox->isChecked());
	str = settings->unlocalizeExpression(exampleEdit->text().trimmed().toStdString());
	f->setExample(str);
	str = CALCULATOR->unlocalizeExpression(conditionEdit->text().trimmed().toStdString(), pa);
	fix_expression(str);
	f->setCondition(str);
	for(int i = 0; i < argumentsModel->rowCount(); i++) {
		Argument *arg = (Argument*) argumentsModel->item(i, 0)->data().value<void*>();
		if(arg) f->setArgumentDefinition(i + 1, arg);
	}
	for(int i = 0; i < subfunctionsModel->rowCount(); i++) {
		str = CALCULATOR->unlocalizeExpression(subfunctionsModel->item(i, 0)->text().trimmed().toStdString(), pa);
		fix_expression(str);
		f->addSubfunction(str, subfunctionsModel->item(i, 0)->checkState() == Qt::Checked);
	}
	if(namesEditDialog) namesEditDialog->modifyNames(f, nameEdit->text());
	else NamesEditDialog::modifyName(f, nameEdit->text());
	str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), settings->evalops.parse_options);
	fix_expression(str);
	((UserFunction*) f)->setFormula(str);
	CALCULATOR->addFunction(f);
	return f;
}
bool FunctionEditDialog::modifyFunction(MathFunction *f, MathFunction **replaced_item) {
	if(replaced_item) *replaced_item = NULL;
	if(CALCULATOR->functionNameTaken(nameEdit->text().trimmed().toStdString(), f)) {
		MathFunction *func = CALCULATOR->getActiveFunction(nameEdit->text().trimmed().toStdString(), true);
		if(name_edited && (!func || func->category() != CALCULATOR->temporaryCategory()) && QMessageBox::question(this, tr("Question"), tr("A function with the same name already exists.\nDo you want to overwrite the function?")) != QMessageBox::Yes) {
			nameEdit->setFocus();
			return false;
		}
		if(replaced_item) {
			*replaced_item = func;
		}
	}
	f->setLocal(true);
	if(namesEditDialog) namesEditDialog->modifyNames(f, nameEdit->text());
	else NamesEditDialog::modifyName(f, nameEdit->text());
	ParseOptions pa = settings->evalops.parse_options;
	pa.base = 10;
	std::string str;
	f->setDescription(descriptionEdit->toPlainText().trimmed().toStdString());
	f->setTitle(titleEdit->text().trimmed().toStdString());
	f->setCategory(categoryEdit->currentText().trimmed().toStdString());
	f->setHidden(hideBox->isChecked());
	str = settings->unlocalizeExpression(exampleEdit->text().trimmed().toStdString());
	f->setExample(str);
	str = CALCULATOR->unlocalizeExpression(conditionEdit->text().trimmed().toStdString(), pa);
	fix_expression(str);
	f->setCondition(str);
	f->clearArgumentDefinitions();
	for(int i = 0; i < argumentsModel->rowCount(); i++) {
		Argument *arg = (Argument*) argumentsModel->item(i, 0)->data().value<void*>();
		if(arg) f->setArgumentDefinition(i + 1, arg);
	}
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		((UserFunction*) f)->clearSubfunctions();
		for(int i = 0; i < subfunctionsModel->rowCount(); i++) {
			str = CALCULATOR->unlocalizeExpression(subfunctionsModel->item(i, 0)->text().trimmed().toStdString(), pa);
			fix_expression(str);
			((UserFunction*) f)->addSubfunction(str, subfunctionsModel->item(i, 0)->checkState() == Qt::Checked);
		}
		str = CALCULATOR->unlocalizeExpression(expressionEdit->toPlainText().trimmed().toStdString(), pa);
		fix_expression(str);
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
	onFunctionChanged();
}
void FunctionEditDialog::subEditClicked() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
	subfunctionsView->edit(subfunctionsView->selectionModel()->currentIndex().siblingAtColumn(0));
#else
	subfunctionsView->edit(subfunctionsView->selectionModel()->currentIndex().sibling(subfunctionsView->selectionModel()->currentIndex().row(), 0));
#endif
	onFunctionChanged();
}
void FunctionEditDialog::subDelClicked() {
	subfunctionsModel->removeRow(subfunctionsView->selectionModel()->currentIndex().row());
	onFunctionChanged();
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
	if(i <= 3) refstr += (char) ('x' + (i - 1));
	else refstr += (char) ('a' + (i - 4));
	item = new QStandardItem(refstr);
	item->setData(QVariant::fromValue((void*) NULL));
	item->setEditable(false);
	item->setTextAlignment(Qt::AlignCenter);
	items.append(item);
	argumentsModel->appendRow(items);
	argumentsView->selectionModel()->setCurrentIndex(edit_item->index(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear | QItemSelectionModel::Rows);
	argumentsView->scrollTo(edit_item->index());
	argEditClicked();
}
void FunctionEditDialog::argumentActivated(const QModelIndex &index) {
	if(!nameEdit->isReadOnly() && index.column() != 0) argEditClicked();
}
void FunctionEditDialog::argumentChanged(QStandardItem *item) {
	if(nameEdit->isReadOnly() || item->column() != 0) return;
	Argument *arg = (Argument*) item->data().value<void*>();
	if(arg) {
		arg->setName(item->text().trimmed().toStdString());
	} else if(!item->text().trimmed().isEmpty()) {
		arg = new Argument(item->text().trimmed().toStdString());
		item->setData(QVariant::fromValue((void*) arg));
	}
	onFunctionChanged();
}
void FunctionEditDialog::argEditClicked() {
	int r = argumentsView->selectionModel()->currentIndex().row();
	QStandardItem *item = argumentsModel->item(r, 0);
	if(!item) return;
	Argument *arg = (Argument*) item->data().value<void*>();
	ArgumentEditDialog *d = new ArgumentEditDialog(this, nameEdit->isReadOnly());
	d->setArgument(arg);
	d->exec();
	if(!nameEdit->isReadOnly()) {
		if(arg) delete arg;
		arg = d->createArgument();
		item->setData(QVariant::fromValue((void*) arg));
		item->setText(QString::fromStdString(arg->name()));
		item->setData("<p>" + item->text() + "</p>", Qt::ToolTipRole);
		item = argumentsModel->item(r, 1);
		item->setText(QString::fromStdString(arg->printlong()));
		item->setData("<p>" + item->text() + "</p>", Qt::ToolTipRole);
		onFunctionChanged();
	}
	d->deleteLater();
}
void FunctionEditDialog::argDelClicked() {
	argumentsModel->removeRow(argumentsView->selectionModel()->currentIndex().row());
	onFunctionChanged();
	for(int i = 0; i < argumentsModel->rowCount(); i++) {
		QStandardItem *item = argumentsModel->item(i, 2);
		if(item) {
			QString refstr = "\\";
			if(i <= 3) refstr += (char) ('x' + (i - 1));
			else refstr += (char) ('a' + (i - 4));
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
			if(arg && !nameEdit->isReadOnly()) delete arg;
		}
	}
}
void FunctionEditDialog::setFunction(MathFunction *f) {
	o_function = f;
	name_edited = false;
	bool read_only = !f->isLocal();
	nameEdit->setText(QString::fromStdString(f->getName(1).name));
	if(namesEditDialog) namesEditDialog->setNames(f, nameEdit->text());
	if(f->subtype() == SUBTYPE_USER_FUNCTION) {
		expressionEdit->setEnabled(true);
		std::string str = settings->localizeExpression(((UserFunction*) f)->formula());
		expressionEdit->setPlainText(QString::fromStdString(str));
		for(size_t i = 1; i <= ((UserFunction*) f)->countSubfunctions(); i++) {
			QList<QStandardItem *> items;
			str = settings->localizeExpression(((UserFunction*) f)->getSubfunction(i));
			QStandardItem *item = new QStandardItem(QString::fromStdString(str));
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
	} else {
		read_only = true;
		expressionEdit->setEnabled(false);
		expressionEdit->clear();
		subfunctionsView->setEnabled(false);
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
		if(i <= 3) refstr += (char) ('x' + (i - 1));
		else refstr += (char) ('a' + (i - 4));
		item = new QStandardItem(refstr);
		item->setData(QVariant::fromValue((void*) arg));
		item->setEditable(false);
		item->setTextAlignment(Qt::AlignCenter);
		items.append(item);
		argumentsModel->appendRow(items);
	}
	descriptionEdit->blockSignals(true);
	descriptionEdit->setPlainText(QString::fromStdString(f->description()));
	descriptionEdit->blockSignals(false);
	std::string str = settings->localizeExpression(f->example(true));
	exampleEdit->setText(QString::fromStdString(str));
	str = settings->localizeExpression(f->condition());
	conditionEdit->setText(QString::fromStdString(str));
	titleEdit->setText(QString::fromStdString(f->title(false)));
	categoryEdit->blockSignals(true);
	categoryEdit->setCurrentText(QString::fromStdString(f->category()));
	categoryEdit->blockSignals(false);
	descriptionEdit->setReadOnly(read_only);
	exampleEdit->setReadOnly(read_only);
	titleEdit->setReadOnly(read_only);
	hideBox->setChecked(f->isHidden());
	okButton->setEnabled(false);
	nameEdit->setReadOnly(read_only);
	subEditButton->setEnabled(false);
	subAddButton->setEnabled(!read_only);
	subDelButton->setEnabled(false);
	argEditButton->setEnabled(false);
	argAddButton->setEnabled(!read_only);
	argDelButton->setEnabled(false);
	conditionEdit->setReadOnly(read_only);
	expressionEdit->setReadOnly(read_only);
}
void FunctionEditDialog::onNameEdited(const QString &str) {
	if(!str.trimmed().isEmpty() && !CALCULATOR->functionNameIsValid(str.trimmed().toStdString())) {
		nameEdit->setText(QString::fromStdString(CALCULATOR->convertToValidFunctionName(str.trimmed().toStdString())));
	}
	name_edited = true;
	onFunctionChanged();
}
void FunctionEditDialog::onFunctionChanged() {
	okButton->setEnabled(!nameEdit->isReadOnly() && (!expressionEdit->document()->isEmpty() || !expressionEdit->isEnabled()) && !nameEdit->text().trimmed().isEmpty());
}
void FunctionEditDialog::setExpression(const QString &str) {
	expressionEdit->setPlainText(str);
	onFunctionChanged();
}
void FunctionEditDialog::setName(const QString &str) {
	nameEdit->setText(str);
	onNameEdited(str);
}
QString FunctionEditDialog::expression() const {
	return expressionEdit->toPlainText();
}
bool FunctionEditDialog::editFunction(QWidget *parent, MathFunction *f, MathFunction **replaced_item) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
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
UserFunction *FunctionEditDialog::newFunction(QWidget *parent, MathFunction **replaced_item) {
	FunctionEditDialog *d = new FunctionEditDialog(parent);
	d->setWindowTitle(tr("New Function"));
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

