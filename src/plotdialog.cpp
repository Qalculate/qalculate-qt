/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTreeWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QDebug>

#include "qalculateqtsettings.h"
#include "plotdialog.h"

#define TYPE_ROLE Qt::UserRole
#define YAXIS2_ROLE Qt::UserRole + 1
#define STYLE_ROLE Qt::UserRole + 2
#define SMOOTHING_ROLE Qt::UserRole + 3
#define ROWS_ROLE Qt::UserRole + 4
#define YVECTOR_ROLE Qt::UserRole + 5
#define XVECTOR_ROLE Qt::UserRole + 6
#define X_ROLE Qt::UserRole + 7

PlotDialog::PlotDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle(tr("Plot"));
	QVBoxLayout *topbox = new QVBoxLayout(this);
	tabs = new QTabWidget(this);
	topbox->addWidget(tabs);
	QWidget *tab; QGridLayout *grid; QHBoxLayout *hbox; QButtonGroup *group; int r = 0;

	plotThread = NULL;

	tab = new QWidget(this);
	tabs->addTab(tab, tr("Data"));
	grid = new QGridLayout(tab);
	grid->addWidget(new QLabel(tr("Title:")), r, 0);
	titleEdit = new QLineEdit(this); grid->addWidget(titleEdit, r, 1); r++;
	grid->addWidget(new QLabel(tr("Expression:")), r, 0);
	expressionEdit = new MathLineEdit(this); grid->addWidget(expressionEdit, r, 1); r++;
	connect(expressionEdit, SIGNAL(textChanged(const QString&)), this, SLOT(enableDisableButtons()));
	connect(expressionEdit, SIGNAL(returnPressed()), this, SLOT(onExpressionActivated()));
	hbox = new QHBoxLayout(); grid->addLayout(hbox, r, 0, 1, 2); r++; hbox->addStretch(1); group = new QButtonGroup(this);
	functionButton = new QRadioButton(tr("Function"), this); group->addButton(functionButton, 0); hbox->addWidget(functionButton);
	vectorButton = new QRadioButton(tr("Vector/matrix"), this); group->addButton(vectorButton, 1); hbox->addWidget(vectorButton);
	pairedButton = new QRadioButton(tr("Paired matrix"), this); group->addButton(pairedButton, 2); hbox->addWidget(pairedButton);
	group->button(settings->default_plot_type)->setChecked(true);
	connect(group, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(onTypeToggled(QAbstractButton*, bool)));
	rowsBox = new QCheckBox(tr("Rows"), this); hbox->addWidget(rowsBox); rowsBox->setChecked(settings->default_plot_rows); rowsBox->setEnabled(settings->default_plot_type > 0);
	grid->addWidget(new QLabel(tr("X variable:")), r, 0);
	variableEdit = new QLineEdit(this); grid->addWidget(variableEdit, r, 1); r++;
	variableEdit->setText(QString::fromStdString(settings->default_plot_variable));
	variableEdit->setEnabled(settings->default_plot_type == 0);
	connect(variableEdit, SIGNAL(textChanged(const QString&)), this, SLOT(enableDisableButtons()));
	grid->addWidget(new QLabel(tr("Style:")), r, 0);
	styleCombo = new QComboBox(this); grid->addWidget(styleCombo, r, 1); r++;
	styleCombo->addItem(tr("Line"), PLOT_STYLE_LINES);
	styleCombo->addItem(tr("Points"), PLOT_STYLE_POINTS);
	styleCombo->addItem(tr("Line with points"), PLOT_STYLE_POINTS_LINES);
	styleCombo->addItem(tr("Boxes/bars"), PLOT_STYLE_BOXES);
	styleCombo->addItem(tr("Histogram"), PLOT_STYLE_HISTOGRAM);
	styleCombo->addItem(tr("Steps"), PLOT_STYLE_STEPS);
	styleCombo->addItem(tr("Candlesticks"), PLOT_STYLE_CANDLESTICKS);
	styleCombo->addItem(tr("Dots"), PLOT_STYLE_DOTS);
	styleCombo->setCurrentIndex(styleCombo->findData(settings->default_plot_style));
	grid->addWidget(new QLabel(tr("Smoothing:")), r, 0);
	smoothingCombo = new QComboBox(this); grid->addWidget(smoothingCombo, r, 1); r++;
	smoothingCombo->addItem(tr("None"), PLOT_SMOOTHING_NONE);
	smoothingCombo->addItem(tr("Monotonic"), PLOT_SMOOTHING_UNIQUE);
	smoothingCombo->addItem(tr("Natural cubic splines"), PLOT_SMOOTHING_CSPLINES);
	smoothingCombo->addItem(tr("Bezier"), PLOT_SMOOTHING_BEZIER);
	smoothingCombo->addItem(tr("Bezier (monotonic)"), PLOT_SMOOTHING_SBEZIER);
	smoothingCombo->setCurrentIndex(smoothingCombo->findData(settings->default_plot_smoothing));
	grid->addWidget(new QLabel(tr("Y-axis:")), r, 0);
	hbox = new QHBoxLayout(); grid->addLayout(hbox, r, 1); r++; group = new QButtonGroup(this);
	primaryButton = new QRadioButton(tr("Primary"), this); group->addButton(primaryButton); hbox->addWidget(primaryButton);
	secondaryButton = new QRadioButton(tr("Secondary"), this); group->addButton(secondaryButton); hbox->addWidget(secondaryButton);
	primaryButton->setChecked(true);
	hbox->addStretch(1);
	hbox = new QHBoxLayout(); grid->addLayout(hbox, r, 0, 1, 2); r++;
	hbox->addStretch(1);
	addButton = new QPushButton(tr("Add"), this); hbox->addWidget(addButton);
	addButton->setEnabled(false);
	connect(addButton, SIGNAL(clicked()), this, SLOT(onAddClicked()));
	applyButton = new QPushButton(tr("Apply"), this); hbox->addWidget(applyButton);
	applyButton->setEnabled(false);
	connect(applyButton, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
	removeButton = new QPushButton(tr("Remove"), this); hbox->addWidget(removeButton);
	removeButton->setEnabled(false);
	connect(removeButton, SIGNAL(clicked()), this, SLOT(onRemoveClicked()));
	graphsTable = new QTreeWidget(this); grid->addWidget(graphsTable, r, 0, 1, 2);
	grid->setRowStretch(r, 1); r++;
	graphsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	graphsTable->setColumnCount(2);
	QStringList list; list << tr("Title"); list << tr("Expression");
	graphsTable->setHeaderLabels(list);
	graphsTable->setRootIsDecorated(false);
	connect(graphsTable, SIGNAL(itemSelectionChanged()), this, SLOT(onGraphsSelectionChanged()));

	tab = new QWidget(this);
	tabs->addTab(tab, tr("Function Range"));
	grid = new QGridLayout(tab); r = 0;
	grid->addWidget(new QLabel(tr("Minimum x value:")), r, 0);
	minxEdit = new MathLineEdit(this); grid->addWidget(minxEdit, r, 1); r++;
	minxEdit->setText(QString::fromStdString(settings->default_plot_min));
	grid->addWidget(new QLabel(tr("Maximum x value:")), r, 0);
	maxxEdit = new MathLineEdit(this); grid->addWidget(maxxEdit, r, 1); r++;
	maxxEdit->setText(QString::fromStdString(settings->default_plot_max));
	group = new QButtonGroup(this);
	rateButton = new QRadioButton(tr("Sampling rate:"), this); group->addButton(rateButton, 0); grid->addWidget(rateButton, r, 0);
	rateSpin = new QSpinBox(this); grid->addWidget(rateSpin, r, 1); r++;
	rateSpin->setRange(1, INT_MAX); rateSpin->setValue(settings->default_plot_sampling_rate);
	rateButton->setChecked(settings->default_plot_use_sampling_rate);
	rateSpin->setEnabled(settings->default_plot_use_sampling_rate);
	stepButton = new QRadioButton(tr("Step size:"), this); group->addButton(stepButton, 1); grid->addWidget(stepButton, r, 0);
	stepEdit = new MathLineEdit(this); grid->addWidget(stepEdit, r, 1); r++;
	stepEdit->setText(QString::fromStdString(settings->default_plot_step));
	stepButton->setChecked(!settings->default_plot_use_sampling_rate);
	stepEdit->setEnabled(!settings->default_plot_use_sampling_rate);
	connect(group, SIGNAL(buttonToggled(QAbstractButton*, bool)), this, SLOT(onRateStepToggled(QAbstractButton*, bool)));
	applyButton2 = new QPushButton(tr("Apply"), this); grid->addWidget(applyButton2, r, 0, 1, 2, Qt::AlignRight | Qt::AlignTop);
	grid->setRowStretch(r, 1);
	connect(applyButton2, SIGNAL(clicked()), this, SLOT(onApply2Clicked()));

	tab = new QWidget(this);
	tabs->addTab(tab, tr("Appearance"));
	grid = new QGridLayout(tab); r = 0;
	grid->addWidget(new QLabel(tr("Title:")), r, 0);
	titleEdit2 = new QLineEdit(this); grid->addWidget(titleEdit2, r, 1); r++;
	gridBox = new QCheckBox(tr("Display grid"), this); grid->addWidget(gridBox, r, 0);
	gridBox->setChecked(settings->default_plot_display_grid);
	borderBox = new QCheckBox(tr("Display full border"), this); grid->addWidget(borderBox, r, 1); r++;
	borderBox->setChecked(settings->default_plot_full_border);
	minyBox = new QCheckBox(tr("Minimum y value:"), this); grid->addWidget(minyBox, r, 0);
	minySpin = new QSpinBox(this); grid->addWidget(minySpin, r, 1); r++;
	minySpin->setEnabled(false);
	minySpin->setRange(INT_MIN, INT_MAX); minySpin->setValue(-1);
	connect(minyBox, SIGNAL(toggled(bool)), minySpin, SLOT(setEnabled(bool)));
	maxyBox = new QCheckBox(tr("Maximum y value:"), this); grid->addWidget(maxyBox, r, 0);
	maxySpin = new QSpinBox(this); grid->addWidget(maxySpin, r, 1); r++;
	maxySpin->setEnabled(false);
	maxySpin->setRange(INT_MIN, INT_MAX); maxySpin->setValue(10);
	connect(maxyBox, SIGNAL(toggled(bool)), maxySpin, SLOT(setEnabled(bool)));
	logxBox = new QCheckBox(tr("Logarithmic x scale:"), this); grid->addWidget(logxBox, r, 0);
	logxSpin = new QSpinBox(this); grid->addWidget(logxSpin, r, 1); r++;
	logxSpin->setRange(2, 100); logxSpin->setValue(10);
	logxSpin->setEnabled(false);
	connect(logxBox, SIGNAL(toggled(bool)), logxSpin, SLOT(setEnabled(bool)));
	logyBox = new QCheckBox(tr("Logarithmic y scale:"), this); grid->addWidget(logyBox, r, 0);
	logySpin = new QSpinBox(this); grid->addWidget(logySpin, r, 1); r++;
	logySpin->setRange(2, 100); logySpin->setValue(10);
	logySpin->setEnabled(false);
	connect(logyBox, SIGNAL(toggled(bool)), logySpin, SLOT(setEnabled(bool)));
	grid->addWidget(new QLabel(tr("X-axis label:")), r, 0);
	xaxisEdit = new QLineEdit(this); grid->addWidget(xaxisEdit, r, 1); r++;
	grid->addWidget(new QLabel(tr("Y-axis label:")), r, 0);
	yaxisEdit = new QLineEdit(this); grid->addWidget(yaxisEdit, r, 1); r++;
	grid->addWidget(new QLabel(tr("Line width:")), r, 0);
	lineSpin = new QSpinBox(this); grid->addWidget(lineSpin, r, 1); r++;
	lineSpin->setRange(1, 10); lineSpin->setValue(settings->default_plot_linewidth);
	grid->addWidget(new QLabel(tr("Legend placement:")), r, 0);
	legendCombo = new QComboBox(this); grid->addWidget(legendCombo, r, 1); r++;
	legendCombo->addItem(tr("None"), PLOT_LEGEND_NONE);
	legendCombo->addItem(tr("Top-left"), PLOT_LEGEND_TOP_LEFT);
	legendCombo->addItem(tr("Top-right"), PLOT_LEGEND_TOP_RIGHT);
	legendCombo->addItem(tr("Bottom-left"), PLOT_LEGEND_BOTTOM_LEFT);
	legendCombo->addItem(tr("Bottom-right"), PLOT_LEGEND_BOTTOM_RIGHT);
	legendCombo->addItem(tr("Below"), PLOT_LEGEND_BELOW);
	legendCombo->addItem(tr("Outside"), PLOT_LEGEND_OUTSIDE);
	legendCombo->setCurrentIndex(legendCombo->findData(settings->default_plot_legend_placement));
	applyButton3 = new QPushButton(tr("Apply"), this); grid->addWidget(applyButton3, r, 0, 1, 2, Qt::AlignRight | Qt::AlignTop);
	grid->setRowStretch(r, 1);
	connect(applyButton3, SIGNAL(clicked()), this, SLOT(onApply3Clicked()));

	expressionEdit->setFocus();

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
	topbox->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(reject()));

}
PlotDialog::~PlotDialog() {}

void PlotDialog::reject() {
	resetParameters();
	settings->default_plot_legend_placement = (PlotLegendPlacement) legendCombo->currentData().toInt();
	settings->default_plot_smoothing = (PlotSmoothing) smoothingCombo->currentData().toInt();
	settings->default_plot_style = (PlotStyle) styleCombo->currentData().toInt();
	settings->default_plot_display_grid = gridBox->isChecked();
	settings->default_plot_full_border = borderBox->isChecked();
	settings->default_plot_min = minxEdit->text().toStdString();
	settings->default_plot_max = maxxEdit->text().toStdString();
	settings->default_plot_step = stepEdit->text().toStdString();
	settings->default_plot_sampling_rate = rateSpin->value();
	settings->default_plot_linewidth = lineSpin->value();
	settings->default_plot_rows = rowsBox->isChecked();
	settings->default_plot_type = (vectorButton->isChecked() ? 1 : (pairedButton->isChecked() ? 2 : 0));
	settings->default_plot_variable = variableEdit->text().toStdString();
	settings->default_plot_use_sampling_rate = rateButton->isChecked();;
	CALCULATOR->closeGnuplot();
	if(plotThread && plotThread->running) {
		plotThread->write(0);
	}
	QDialog::reject();
}
void PlotDialog::resetParameters() {
	expressionEdit->clear();
	titleEdit->clear();
	titleEdit2->clear();
	xaxisEdit->clear();
	yaxisEdit->clear();
	graphsTable->clear();
	addButton->setEnabled(false);
	removeButton->setEnabled(false);
	applyButton->setEnabled(false);
	primaryButton->setChecked(true);
}
void PlotDialog::enableDisableButtons() {
	bool b = (!functionButton->isChecked() || !variableEdit->text().trimmed().isEmpty()) && !expressionEdit->text().trimmed().isEmpty();
	addButton->setEnabled(b);
	QList<QTreeWidgetItem *> list = graphsTable->selectedItems();
	removeButton->setEnabled(list.count() >= 1);
	applyButton->setEnabled(b && list.count() == 1);
}
void PlotDialog::setExpression(const QString &str) {
	expressionEdit->setText(str);
	expressionEdit->setFocus();
	expressionEdit->selectAll();
}
void PlotDialog::onExpressionActivated() {
	if(applyButton->isEnabled()) onApplyClicked();
	else if(addButton->isEnabled()) onAddClicked();
}
void PlotDialog::onAddClicked() {
	QTreeWidgetItem *item = new QTreeWidgetItem(graphsTable);
	updateItem(item);
	graphsTable->selectionModel()->clear();
	item->setSelected(true);
	expressionEdit->setFocus();
	expressionEdit->selectAll();
	updatePlot();
}

std::string plot_min, plot_max, plot_x, plot_str, plot_step;
int plot_rate;
bool plot_busy;
std::vector<MathStructure> y_vectors;
std::vector<MathStructure> x_vectors;
std::vector<PlotDataParameters*> pdps;
PlotParameters pp;

void PlotDialog::abort() {
	CALCULATOR->abort();
}

class PlotThread : public Thread {
	protected:
		void run() {
			while(true) {
				EvaluationOptions eo;
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.parse_options = settings->evalops.parse_options;
				eo.parse_options.base = 10;
				eo.parse_options.read_precision = DONT_READ_PRECISION;
				int type = -1;
				if(!read(&type) || type <= 0) return;
				if(type == 2) {
					CALCULATOR->startControl();
					CALCULATOR->plotVectors(&pp, y_vectors, x_vectors, pdps, false, 0);
					CALCULATOR->stopControl();
				} else {
					void *x = NULL;
					if(!read(&x) || !x) return;
					MathStructure *y_vector = (MathStructure*) x;
					if(!read(&x)) return;
					CALCULATOR->beginTemporaryStopIntervalArithmetic();
					CALCULATOR->startControl();
					if(!x) {
						y_vector->set(CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(plot_str, eo.parse_options), eo));
					} else {
						MathStructure *x_vector = (MathStructure*) x;
						MathStructure min = CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(plot_min, eo.parse_options), eo);
						MathStructure max = CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(plot_max, eo.parse_options), eo);
						if(plot_rate < 0) {
							MathStructure m_step = CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(plot_step, eo.parse_options), eo);
							if(!CALCULATOR->aborted()) {
								y_vector->set(CALCULATOR->expressionToPlotVector(plot_str, min, max, m_step, x_vector, plot_x, eo.parse_options, 0));
							}
						} else if(!CALCULATOR->aborted()) {
							y_vector->set(CALCULATOR->expressionToPlotVector(plot_str, min, max, plot_rate, x_vector, plot_x, eo.parse_options, 0));
						}
					}
					CALCULATOR->stopControl();
					CALCULATOR->endTemporaryStopIntervalArithmetic();
				}
				plot_busy = false;
			}
		}
};

void PlotDialog::generatePlotSeries(MathStructure **x_vector, MathStructure **y_vector, int type, QString str, QString str_x) {
	*y_vector = new MathStructure();
	plot_str = str.toStdString();
	plot_x = str_x.toStdString();
	if(type == 1 || type == 2) {
		*x_vector = NULL;
	} else {
		*x_vector = new MathStructure();
		(*x_vector)->clearVector();
		plot_min = minxEdit->text().toStdString();
		plot_max = maxxEdit->text().toStdString();
		if(stepButton->isChecked()) {
			plot_rate = -1;
			plot_step = stepEdit->text().toStdString();
		} else {
			plot_rate = rateSpin->value();
		}
	}
	plot_busy = true;
	if(!plotThread) plotThread = new PlotThread;
	if(!plotThread->running) plotThread->start();
	if(plotThread->write(1) && plotThread->write((void*) *y_vector) && plotThread->write((void*) *x_vector)) {
		QProgressDialog *dialog = NULL;
		int i = 0;
		while(plot_busy && plotThread->running && i < 100) {
			sleep_ms(10);
			i++;
		}
		i = 0;
		if(plot_busy && plotThread->running) {
			dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
			dialog->setWindowTitle(tr("Calculating…"));
			connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
			dialog->setWindowModality(Qt::WindowModal);
			dialog->show();
			QApplication::setOverrideCursor(Qt::WaitCursor);
		}
		while(plot_busy && plotThread->running) {
			qApp->processEvents();
			sleep_ms(100);
		}

		if(dialog) {
			QApplication::restoreOverrideCursor();
			dialog->hide();
			dialog->deleteLater();
		}
	}
	settings->displayMessages(this);
}
bool PlotDialog::generatePlot() {
	for(int i = 0; ; i++) {
		QTreeWidgetItem *item = graphsTable->topLevelItem(i);
		if(!item) {
			if(i == 0) return false;
			break;
		}
		MathStructure *x_vector = (MathStructure*) item->data(0, XVECTOR_ROLE).value<void*>();
		MathStructure *y_vector = (MathStructure*) item->data(0, YVECTOR_ROLE).value<void*>();
		std::string title = item->text(0).toStdString();
		std::string expr = item->text(1).toStdString();
		int type = item->data(0, TYPE_ROLE).toInt();
		int style = item->data(0, STYLE_ROLE).toInt();
		int smoothing = item->data(0, SMOOTHING_ROLE).toInt();
		bool yaxis2 = item->data(0, YAXIS2_ROLE).toBool();
		bool rows = item->data(0, ROWS_ROLE).toBool();
		int count = 1;
		if(type == 1) {
			if(y_vector->isMatrix()) {
				count = 0;
				if(rows) {
					for(size_t i = 1; i <= y_vector->rows(); i++) {
						y_vectors.push_back(m_undefined);
						y_vector->rowToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						count++;
					}
				} else {
					for(size_t i = 1; i <= y_vector->columns(); i++) {
						y_vectors.push_back(m_undefined);
						y_vector->columnToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						count++;
					}
				}
			} else if(y_vector->isVector()) {
				y_vectors.push_back(*y_vector);
				x_vectors.push_back(m_undefined);
			} else {
				y_vectors.push_back(*y_vector);
				y_vectors[y_vectors.size() - 1].transform(STRUCT_VECTOR);
				x_vectors.push_back(m_undefined);
			}
		} else if(type == 2) {
			if(y_vector->isMatrix()) {
				count = 0;
				if(rows) {
					for(size_t i = 1; i <= y_vector->rows(); i += 2) {
						y_vectors.push_back(m_undefined);
						y_vector->rowToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						y_vector->rowToVector(i + 1, x_vectors[x_vectors.size() - 1]);
						count++;
					}
				} else {
					for(size_t i = 1; i <= y_vector->columns(); i += 2) {
						y_vectors.push_back(m_undefined);
						y_vector->columnToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						y_vector->columnToVector(i + 1, x_vectors[x_vectors.size() - 1]);
						count++;
					}
				}
			} else if(y_vector->isVector()) {
				y_vectors.push_back(*y_vector);
				x_vectors.push_back(m_undefined);
			} else {
				y_vectors.push_back(*y_vector);
				y_vectors[y_vectors.size() - 1].transform(STRUCT_VECTOR);
				x_vectors.push_back(m_undefined);
			}
		} else {
			y_vectors.push_back(*y_vector);
			x_vectors.push_back(*x_vector);
		}
		for(int i = 0; i < count; i++) {
			PlotDataParameters *pdp = new PlotDataParameters();
			pdp->title = title;
			if(pdp->title.empty()) {
				pdp->title = expr;
			}
			if(count > 1) {
				pdp->title += " :";
				pdp->title += i2s(i + 1);
			}
			pdp->test_continuous = (type != 1 && type != 2);
			pdp->smoothing = (PlotSmoothing) smoothing;
			pdp->style = (PlotStyle) style;
			pdp->yaxis2 = yaxis2;
			pdps.push_back(pdp);
		}
	}
	pp.legend_placement = (PlotLegendPlacement) legendCombo->currentData().toInt();
	pp.title = titleEdit2->text().toStdString();
	pp.x_label = xaxisEdit->text().toStdString();
	pp.y_label = yaxisEdit->text().toStdString();
	pp.grid = gridBox->isChecked();
	pp.x_log = logxBox->isChecked();
	pp.y_log = logyBox->isChecked();
	pp.x_log_base = logxSpin->value();
	pp.y_log_base = logySpin->value();
	pp.auto_y_min = !minyBox->isChecked();
	pp.auto_y_max = !maxyBox->isChecked();
	pp.y_min = minySpin->value();
	pp.y_max = maxySpin->value();
	pp.color = settings->default_plot_color;
	pp.show_all_borders = borderBox->isChecked();
	pp.linewidth = lineSpin->value();
	return true;
}
void PlotDialog::updatePlot() {
	if(!generatePlot()) {
		CALCULATOR->closeGnuplot();
		return;
	}
	plot_busy = true;
	if(!plotThread) plotThread = new PlotThread;
	if(!plotThread->running) plotThread->start();
	if(plotThread->write(2)) {
		QProgressDialog *dialog = NULL;
		int i = 0;
		while(plot_busy && plotThread->running && i < 100) {
			sleep_ms(10);
			i++;
		}
		i = 0;
		if(plot_busy && plotThread->running) {
			dialog = new QProgressDialog(tr("Processing…"), tr("Cancel"), 0, 0, this);
			connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
			dialog->setWindowModality(Qt::WindowModal);
			dialog->show();
			QApplication::setOverrideCursor(Qt::WaitCursor);
		}
		while(plot_busy && plotThread->running) {
			qApp->processEvents();
			sleep_ms(100);
		}

		if(dialog) {
			QApplication::restoreOverrideCursor();
			dialog->hide();
			dialog->deleteLater();
		}
	}
	settings->displayMessages(this);
	for(size_t i = 0; i < pdps.size(); i++) {
		if(pdps[i]) delete pdps[i];
	}
	pdps.clear();
	x_vectors.clear();
	y_vectors.clear();
}
void PlotDialog::updateItem(QTreeWidgetItem *item) {
	int type = (vectorButton->isChecked() ? 1 : (pairedButton->isChecked() ? 2 : 0));
	QString expression = expressionEdit->text().trimmed();
	QString title = titleEdit->text().trimmed();
	QString str_x = variableEdit->text().trimmed();
	if((type == 1 || type == 2) && title.isEmpty()) {
		Variable *v = CALCULATOR->getActiveVariable(expression.toStdString());
		if(v) {
			title = QString::fromStdString(v->title(false));
		}
	}
	MathStructure *x_vector, *y_vector;
	generatePlotSeries(&x_vector, &y_vector, type, expression, str_x);
	item->setData(0, TYPE_ROLE, type);
	item->setData(0, YAXIS2_ROLE, secondaryButton->isChecked());
	item->setData(0, ROWS_ROLE, rowsBox->isChecked());
	item->setData(0, SMOOTHING_ROLE, smoothingCombo->currentData());
	item->setData(0, STYLE_ROLE, styleCombo->currentData());
	item->setData(0, YVECTOR_ROLE, QVariant::fromValue((void*) y_vector));
	item->setData(0, XVECTOR_ROLE, QVariant::fromValue((void*) x_vector));
	item->setData(0, X_ROLE, str_x);
	item->setText(0, title);
	item->setText(1, expression);
	item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
}
void PlotDialog::onApplyClicked() {
	QList<QTreeWidgetItem *> list = graphsTable->selectedItems();
	if(list.count() != 1) return;
	MathStructure *x_vector = (MathStructure*) list[0]->data(0, XVECTOR_ROLE).value<void*>();
	if(x_vector) delete x_vector;
	MathStructure *y_vector = (MathStructure*) list[0]->data(0, YVECTOR_ROLE).value<void*>();
	if(y_vector) delete y_vector;
	updateItem(list[0]);
	expressionEdit->setFocus();
	expressionEdit->selectAll();
	updatePlot();
}
void PlotDialog::onRemoveClicked() {
	QList<QTreeWidgetItem *> list = graphsTable->selectedItems();
	for(int i = 0; i < list.count(); i++) {
		MathStructure *x_vector = (MathStructure*) list[i]->data(0, XVECTOR_ROLE).value<void*>();
		if(x_vector) delete x_vector;
		MathStructure *y_vector = (MathStructure*) list[i]->data(0, YVECTOR_ROLE).value<void*>();
		if(y_vector) delete y_vector;
		delete list[i];
	}
	updatePlot();
}
void PlotDialog::onApply2Clicked() {
	for(int i = 0; ; i++) {
		QTreeWidgetItem *item = graphsTable->topLevelItem(i);
		if(!item) break;
		MathStructure *x_vector = (MathStructure*) item->data(0, XVECTOR_ROLE).value<void*>();
		if(x_vector) delete x_vector;
		MathStructure *y_vector = (MathStructure*) item->data(0, YVECTOR_ROLE).value<void*>();
		if(y_vector) delete y_vector;
		generatePlotSeries(&x_vector, &y_vector, item->data(0, TYPE_ROLE).toInt(), item->text(1), item->data(0, X_ROLE).toString());
		item->setData(0, YVECTOR_ROLE, QVariant::fromValue((void*) y_vector));
		item->setData(0, XVECTOR_ROLE, QVariant::fromValue((void*) x_vector));
	}
	updatePlot();
}
void PlotDialog::onApply3Clicked() {
	updatePlot();
}
void PlotDialog::onTypeToggled(QAbstractButton *w, bool b) {
	if(b) {
		rowsBox->setEnabled(w != functionButton);
		variableEdit->setEnabled(w == functionButton);
		enableDisableButtons();
	}
}
void PlotDialog::onRateStepToggled(QAbstractButton *w, bool b) {
	if(b) {
		rateSpin->setEnabled(w == rateButton);
		stepEdit->setEnabled(w == stepButton);
	}
}
void PlotDialog::onGraphsSelectionChanged() {
	enableDisableButtons();
}

