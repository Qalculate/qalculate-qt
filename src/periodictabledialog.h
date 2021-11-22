/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PERIODIC_TABLE_DIALOG_H
#define PERIODIC_TABLE_DIALOG_H

#include <QDialog>
#include <QMap>

#include <libqalculate/qalculate.h>

class PeriodicTableDialog : public QDialog {

	Q_OBJECT

	protected:

		QMap<DataObject*, QDialog*> elementDialogs;

	protected slots:

		void elementClicked();
		void propertyClicked();

	public slots:

		void reject() override;
		void onAlwaysOnTopChanged();

	public:

		PeriodicTableDialog(QWidget *parent = NULL);
		virtual ~PeriodicTableDialog();

	signals:

		void insertPropertyRequest(DataObject*, DataProperty*);

};

#endif //PERIODIC_TABLE_DIALOG_H

