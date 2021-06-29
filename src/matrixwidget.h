/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef MATRIX_WIDGET_H
#define MATRIX_WIDGET_H

#include <QWidget>

#include <libqalculate/qalculate.h>

class QTableWidget;
class QSpinBox;

class MatrixWidget : public QWidget {

	Q_OBJECT

	public:

		MatrixWidget(QWidget *parent = 0, int r = 8, int c = 8);
		~MatrixWidget();

		QSize sizeHint() const override;
		QString getMatrixString() const;
		void setMatrix(const MathStructure&);
		void setMatrixStrings(const MathStructure&);
		void setMatrixString(const QString&);
		void setEditable(bool);
		bool isEmpty() const;

	protected:

		QSpinBox *rowsSpin, *columnsSpin;
		QTableWidget *matrixTable;

	protected slots:

		void matrixRowsChanged(int);
		void matrixColumnsChanged(int);

	signals:

		void matrixChanged();
		void dimensionChanged(int, int);

};

#endif //MATRIX_WIDGET_H
