/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FP_CONVERSION_DIALOG_H
#define FP_CONVERSION_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QComboBox;
class QTextEdit;
class QLineEdit;

class FPConversionDialog : public QDialog {

	Q_OBJECT

	protected:

		QComboBox *formatCombo;
		QLineEdit *valueEdit, *hexEdit, *exp2Edit, *decEdit, *errorEdit;
		QTextEdit *binEdit;
		int getBits();

	protected slots:

		void formatChanged();
		void updateFields(int base);
		void hexChanged();
		void binChanged();
		void valueChanged();

	public slots:

		void setValue(const QString &str);
		void setBin(const QString &str);
		void setHex(const QString &str);

	public:

		FPConversionDialog(QWidget *parent = NULL);
		virtual ~FPConversionDialog();

};

#endif //FP_CONVERSION_DIALOG_H

