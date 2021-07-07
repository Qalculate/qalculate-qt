/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PLOT_DIALOG_H
#define PLOT_DIALOG_H

#include <QDialog>

#include <libqalculate/qalculate.h>

class QLineEdit;
class QPushButton;
class QRadioButton;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
class QTabWidget;
class QAbstractButton;

class PlotDialog : public QDialog {

	Q_OBJECT

	protected:

		QLineEdit *titleEdit, *expressionEdit, *variableEdit, *stepEdit, *titleEdit2, *xaxisEdit, *yaxisEdit, *minxEdit, *maxxEdit;
		QRadioButton *functionButton, *vectorButton, *pairedButton, *primaryButton, *secondaryButton, *rateButton, *stepButton;
		QCheckBox *rowsBox, *gridBox, *borderBox, *minyBox, *maxyBox, *logxBox, *logyBox;
		QComboBox *styleCombo, *smoothingCombo, *legendCombo;
		QPushButton *addButton, *applyButton, *removeButton, *applyButton2, *applyButton3;
		QTreeWidget *graphsTable;
		QSpinBox *minySpin, *maxySpin, *logxSpin, *logySpin, *rateSpin, *lineSpin;
		QTabWidget *tabs;
		Thread *plotThread;

		void generatePlotSeries(MathStructure **x_vector, MathStructure **y_vector, int type, QString str, QString str_x);
		bool generatePlot();
		void updatePlot();
		void updateItem(QTreeWidgetItem*);

	public:

		PlotDialog(QWidget *parent = NULL);
		virtual ~PlotDialog();

		void resetParameters();

	protected slots:

		void enableDisableButtons();
		void onExpressionActivated();
		void onAddClicked();
		void onApplyClicked();
		void onRemoveClicked();
		void onApply2Clicked();
		void onApply3Clicked();
		void onTypeToggled(QAbstractButton*, bool);
		void onRateStepToggled(QAbstractButton*, bool);
		void onGraphsSelectionChanged();
		void abort();

	public slots:

		void reject() override;
		void setExpression(const QString&);

};

#endif //PLOT_DIALOG_H
