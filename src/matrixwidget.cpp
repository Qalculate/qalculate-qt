/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFontMetrics>
#include <QDebug>

#include "matrixwidget.h"
#include "qalculateqtsettings.h"

MatrixWidget::MatrixWidget(QWidget *parent, int r, int c) : QWidget(parent) {
	QVBoxLayout *box = new QVBoxLayout(this);
	box->setContentsMargins(0, 0, 0, 0);
	QHBoxLayout *hbox = new QHBoxLayout();
	box->addLayout(hbox);
	hbox->addStretch(1);
	rowsSpin = new QSpinBox(this);
	rowsSpin->setRange(1, 10000);
	rowsSpin->setValue(r);
	hbox->addWidget(rowsSpin);
	connect(rowsSpin, SIGNAL(valueChanged(int)), this, SLOT(matrixRowsChanged(int)));
	hbox->addWidget(new QLabel(SIGN_MULTIPLICATION));
	columnsSpin = new QSpinBox(this);
	columnsSpin->setRange(1, 10000);
	columnsSpin->setValue(c);
	hbox->addWidget(columnsSpin);
	connect(columnsSpin, SIGNAL(valueChanged(int)), this, SLOT(matrixColumnsChanged(int)));
	matrixTable = new QTableWidget(this);
	matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	QTableWidgetItem *proto = new QTableWidgetItem();
	proto->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	matrixTable->setItemPrototype(proto);
	matrixTable->setRowCount(r);
	matrixTable->setColumnCount(c);
	matrixTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	matrixTable->setFocus();
	setEditable(true);
	box->addWidget(matrixTable, 1);
	connect(matrixTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SIGNAL(matrixChanged()));
}
MatrixWidget::~MatrixWidget() {}

QSize MatrixWidget::sizeHint() const {
	QFontMetrics fm(matrixTable->font());
	return QSize(fm.boundingRect("000000").width() * 12, QWidget::sizeHint().height());
}
void MatrixWidget::matrixRowsChanged(int i) {
	matrixTable->setRowCount(i);
	matrixTable->adjustSize();
	emit dimensionChanged(i, matrixTable->columnCount());
}
void MatrixWidget::matrixColumnsChanged(int i) {
	matrixTable->setColumnCount(i);
	matrixTable->adjustSize();
	emit dimensionChanged(matrixTable->rowCount(), i);
}
void MatrixWidget::setEditable(bool b) {
	matrixTable->setEditTriggers(b ? QAbstractItemView::AllEditTriggers : QAbstractItemView::NoEditTriggers);
	rowsSpin->setReadOnly(!b);
	columnsSpin->setReadOnly(!b);
}
QString MatrixWidget::getMatrixString() const {
	QString str;
	int max_c = -1, max_r = -1;
	for(int r = 0; r < matrixTable->rowCount(); r++) {
		bool b = false;
		for(int c = 0; c < matrixTable->columnCount(); c++) {
			QTableWidgetItem *item = matrixTable->item(r, c);
			if(item && !item->text().isEmpty()) {
				b = true;
				if(c > max_c) max_c = c;
			}
		}
		if(b) max_r = r;
	}
	if(max_r < 0) {max_r = matrixTable->rowCount() - 1; max_c = matrixTable->columnCount() - 1;}
	str = "[";
	for(int r = 0; r <= max_r; r++) {
		if(r > 0) str += ", ";
		str += "[";
		for(int c = 0; c <= max_c; c++) {
			if(c > 0) str += QString::fromStdString(CALCULATOR->getComma());
			QTableWidgetItem *item = matrixTable->item(r, c);
			if(!item || item->text().isEmpty()) str += "0";
			else str += item->text();
		}
		str += "]";
	}
	str += "]";
	return str;
}
void MatrixWidget::setMatrix(const MathStructure &mstruct) {
	PrintOptions po = settings->printops;
	po.is_approximate = NULL;
	po.allow_non_usable = false;
	po.base = 10;
	QTableWidgetItem *item;
	if(mstruct.isMatrix()) {
		MathStructure m(mstruct);
		rowsSpin->setValue(m.rows());
		columnsSpin->setValue(m.columns());
		CALCULATOR->startControl(2000);
		for(size_t i = 0; i < m.rows(); i++) {
			for(size_t i2 = 0; i2 < m.columns(); i2++) {
				m.getElement(i + 1, i2 + 1)->format(po);
				item = new QTableWidgetItem(QString::fromStdString(m.getElement(i + 1, i2 + 1)->print(po)));
				item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
				matrixTable->setItem((int) i, (int) i2, item);
			}
		}
		CALCULATOR->stopControl();
	}
}
void MatrixWidget::setMatrixStrings(const MathStructure &mstruct) {
	QTableWidgetItem *item;
	rowsSpin->setValue(mstruct.rows());
	columnsSpin->setValue(mstruct.columns());
	for(size_t i = 0; i < mstruct.rows(); i++) {
		for(size_t i2 = 0; i2 < mstruct.columns(); i2++) {
			item = new QTableWidgetItem(QString::fromStdString(mstruct.getElement(i + 1, i2 + 1)->symbol()));
			item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			matrixTable->setItem((int) i, (int) i2, item);
		}
	}
}
void MatrixWidget::setMatrixString(const QString &str) {
	if(str.isEmpty()) return;
	MathStructure m;
	CALCULATOR->beginTemporaryStopMessages();
	CALCULATOR->parse(&m, CALCULATOR->unlocalizeExpression(str.toStdString(), settings->evalops.parse_options), settings->evalops.parse_options);
	CALCULATOR->endTemporaryStopMessages();
	setMatrix(m);
}
bool MatrixWidget::isEmpty() const {
	for(int r = 0; r < matrixTable->rowCount(); r++) {
		for(int c = 0; c < matrixTable->columnCount(); c++) {
			QTableWidgetItem *item = matrixTable->item(r, c);
			if(item && !item->text().isEmpty()) {
				return false;
			}
		}
	}
	return true;
}

