/*
    Qalculate (QT UI)

    Copyright (C) 2021-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <QLocalSocket>
#include <QLocalServer>
#include <QCommandLineParser>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QProgressDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDockWidget>
#include <QToolTip>
#include <QToolBar>
#include <QPushButton>
#include <QAction>
#include <QApplication>
#include <QLineEdit>
#include <QTextEdit>
#include <QMenu>
#include <QActionGroup>
#include <QToolButton>
#include <QSpinBox>
#include <QWidgetAction>
#include <QStyle>
#include <QTimer>
#include <QComboBox>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QScrollArea>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QKeySequenceEdit>
#include <QHeaderView>
#include <QDesktopServices>
#include <QClipboard>
#include <QMimeData>
#include <QScrollBar>
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
#	include <QStyleHints>
#endif
#include <QDebug>

#include "qalculatewindow.h"
#include "qalculateqtsettings.h"
#include "expressionedit.h"
#include "historyview.h"
#include "keypadwidget.h"
#include "unknowneditdialog.h"
#include "variableeditdialog.h"
#include "uniteditdialog.h"
#include "functioneditdialog.h"
#include "preferencesdialog.h"
#include "datasetsdialog.h"
#include "functionsdialog.h"
#include "variablesdialog.h"
#include "unitsdialog.h"
#include "fpconversiondialog.h"
#include "percentagecalculationdialog.h"
#include "plotdialog.h"
#include "periodictabledialog.h"
#include "calendarconversiondialog.h"
#include "matrixwidget.h"
#include "csvdialog.h"

class ViewThread : public Thread {
protected:
	virtual void run();
};
class CommandThread : public Thread {
protected:
	virtual void run();
};

enum {
	COMMAND_FACTORIZE,
	COMMAND_EXPAND_PARTIAL_FRACTIONS,
	COMMAND_EXPAND,
	COMMAND_CONVERT_UNIT,
	COMMAND_CONVERT_STRING,
	COMMAND_CONVERT_BASE,
	COMMAND_CONVERT_OPTIMAL,
	COMMAND_CALCULATE,
	COMMAND_EVAL
};

#ifdef _WIN32
#	define DEFAULT_HEIGHT 600
#	define DEFAULT_WIDTH 550
#else
#	define DEFAULT_HEIGHT 650
#	define DEFAULT_WIDTH 600
#endif

std::vector<std::string> alt_results;
int b_busy = 0, block_result_update = 0;
bool exact_comparison, command_aborted;
std::string original_expression, result_text, parsed_text, exact_text, previous_expression;
bool had_to_expression = false;
MathStructure *mstruct, *parsed_mstruct, *parsed_tostruct, matrix_mstruct, mstruct_exact, prepend_mstruct, lastx;
QString lastx_text, current_status;
std::string command_convert_units_string;
Unit *command_convert_unit;
bool block_expression_history = false;
int to_fraction = 0;
long int to_fixed_fraction = 0;
char to_prefix = 0;
int to_base = 0;
bool to_duo_syms = false;
int to_caf = -1;
unsigned int to_bits = 0;
Number to_nbase;
std::string result_bin, result_oct, result_dec, result_hex;
std::string auto_expression, auto_result, current_status_expression;
bool auto_calculation_updated = false, auto_format_updated = false, auto_error = false;
bool bases_is_result = false;
Number max_bases, min_bases;
bool title_modified = false;
MathStructure mauto;

bool contains_unknown_variable(const MathStructure &m) {
	if(m.isVariable()) return !m.variable()->isKnown();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_unknown_variable(m[i])) return true;
	}
	return false;
}

QAction *find_child_data(QObject *parent, const QString &name, int v) {
	QActionGroup *group = parent->findChild<QActionGroup*>(name);
	if(!group) return NULL;
	QList<QAction*> actions = group->actions();
	for(int i = 0; i < actions.count(); i++) {
		if(actions.at(i)->data().toInt() == v) return actions.at(i);
	}
	return NULL;
}

bool contains_rand_function(const MathStructure &m) {
	if(m.isFunction() && m.function()->category() == CALCULATOR->getFunctionById(FUNCTION_ID_RAND)->category()) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_rand_function(m[i])) return true;
	}
	return false;
}

bool contains_fraction_qt(const MathStructure &m) {
	if(m.isNumber()) return !m.number().isInteger();
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_fraction_qt(m[i])) return true;
	}
	return false;
}

std::string print_with_evalops(const Number &nr) {
	PrintOptions po;
	po.is_approximate = NULL;
	po.base = settings->evalops.parse_options.base;
	po.base_display = BASE_DISPLAY_NONE;
	po.twos_complement = settings->evalops.parse_options.twos_complement;
	po.hexadecimal_twos_complement = settings->evalops.parse_options.hexadecimal_twos_complement;
	if(((po.base == 2 && po.twos_complement) || (po.base == 16 && po.hexadecimal_twos_complement)) && nr.isNegative()) po.binary_bits = settings->evalops.parse_options.binary_bits;
	po.min_exp = EXP_NONE;
	Number nr_base;
	if(po.base == BASE_CUSTOM) {
		nr_base = CALCULATOR->customOutputBase();
		CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
	}
	if(po.base == BASE_CUSTOM && CALCULATOR->customInputBase().isInteger() && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
		nr_base = CALCULATOR->customOutputBase();
		CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
	} else if((po.base < BASE_CUSTOM && po.base != BASE_UNICODE && po.base != BASE_BIJECTIVE_26 && po.base != BASE_BINARY_DECIMAL) || (po.base == BASE_CUSTOM && CALCULATOR->customInputBase() <= 12 && CALCULATOR->customInputBase() >= -12)) {
		po.base = 10;
		std::string str = "dec(";
		str += nr.print(po);
		str += ")";
		return str;
	} else if(po.base == BASE_CUSTOM) {
		po.base = 10;
	}
	std::string str = nr.print(po);
	if(po.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nr_base);
	return str;
}

std::string unhtmlize(std::string str, bool b_ascii) {
	size_t i = 0, i2;
	while(true) {
		i = str.find("<", i);
		if(i == std::string::npos || i == str.length() - 1) break;
		i2 = str.find(">", i + 1);
		if(i2 == std::string::npos) break;
		if((i2 - i == 3 && str.substr(i + 1, 2) == "br") || (i2 - i == 4 && str.substr(i + 1, 3) == "/tr")) {
			str.replace(i, i2 - i + 1, "\n");
			continue;
		} else if(i2 - i == 5 && str.substr(i + 1, 4) == "head") {
			size_t i3 = str.find("</head>", i2 + 1);
			if(i3 != std::string::npos) {
				i2 = i3 + 6;
			}
		} else if(i2 - i == 4) {
			if(str.substr(i + 1, 3) == "sup") {
				size_t i3 = str.find("</sup>", i2 + 1);
				if(i3 != std::string::npos) {
					std::string str2 = unhtmlize(str.substr(i + 5, i3 - i - 5), b_ascii);
					if(!b_ascii && str2.length() == 1 && str2[0] == '2') str.replace(i, i3 - i + 6, SIGN_POWER_2);
					else if(!b_ascii && str2.length() == 1 && str2[0] == '3') str.replace(i, i3 - i + 6, SIGN_POWER_3);
					else if((str.length() == i3 + 6 || (str.length() == i3 + 13 && str.find("</span>", i3) != std::string::npos)) && (unicode_length(str2) == 1 || str2.find_first_not_of(NUMBERS) == std::string::npos)) str.replace(i, i3 - i + 6, std::string("^") + str2);
					else str.replace(i, i3 - i + 6, std::string("^(") + str2 + ")");
					continue;
				}
			} else if(str.substr(i + 1, 3) == "sub") {
				size_t i3 = str.find("</sub>", i + 4);
				if(i3 != std::string::npos) {
					if(i3 - i2 > 16 && str.substr(i2 + 1, 7) == "<small>" && str.substr(i3 - 8, 8) == "</small>") str.erase(i, i3 - i + 6);
					else str.replace(i, i3 - i + 6, std::string("_") + unhtmlize(str.substr(i + 5, i3 - i - 5), b_ascii));
					continue;
				}
			}
		} else if(i2 - i == 17 && str.substr(i + 1, 16) == "i class=\"symbol\"") {
			size_t i3 = str.find("</i>", i2 + 1);
			if(i3 != std::string::npos) {
				std::string name = unhtmlize(str.substr(i2 + 1, i3 - i2 - 1), b_ascii);
				if(name.length() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) {
					name.insert(0, 1, '\\');
				} else {
					name.insert(0, 1, '\"');
					name += '\"';
				}
				str.replace(i, i3 - i + 4, name);
				continue;
			}
		}
		str.erase(i, i2 - i + 1);
	}
	gsub(" " SIGN_DIVISION_SLASH " ", "/", str);
	gsub("&amp;", "&", str);
	gsub("&gt;", ">", str);
	gsub("&lt;", "<", str);
	gsub("&quot;", "\"", str);
	gsub("&hairsp;", "", str);
	gsub("&nbsp;", " ", str);
	gsub("&thinsp;", THIN_SPACE, str);
	gsub("&#8239;", NNBSP, str);
	return str;
}

void base_from_string(std::string str, int &base, Number &nbase, bool input_base) {
	if(equalsIgnoreCase(str, "golden") || equalsIgnoreCase(str, "golden ratio") || str == "φ") base = BASE_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "roman") || equalsIgnoreCase(str, "roman")) base = BASE_ROMAN_NUMERALS;
	else if(!input_base && (equalsIgnoreCase(str, "time") || equalsIgnoreCase(str, "time"))) base = BASE_TIME;
	else if(str == "b26" || str == "B26") base = BASE_BIJECTIVE_26;
	else if(equalsIgnoreCase(str, "bcd")) base = BASE_BINARY_DECIMAL;
	else if(equalsIgnoreCase(str, "unicode")) base = BASE_UNICODE;
	else if(equalsIgnoreCase(str, "supergolden") || equalsIgnoreCase(str, "supergolden ratio") || str == "ψ") base = BASE_SUPER_GOLDEN_RATIO;
	else if(equalsIgnoreCase(str, "pi") || str == "π") base = BASE_PI;
	else if(str == "e") base = BASE_E;
	else if(str == "sqrt(2)" || str == "sqrt 2" || str == "sqrt2" || str == "√2") base = BASE_SQRT2;
	else {
		EvaluationOptions eo = settings->evalops;
		eo.parse_options.base = 10;
		MathStructure m;
		eo.approximation = APPROXIMATION_TRY_EXACT;
		CALCULATOR->beginTemporaryStopMessages();
		CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 350, eo);
		if(CALCULATOR->endTemporaryStopMessages()) {
			base = BASE_CUSTOM;
			nbase.clear();
		} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
			base = m.number().intValue();
		} else {
			base = BASE_CUSTOM;
			nbase = m.number();
		}
	}
}

QalculateToolButton::QalculateToolButton(QWidget *parent) : QToolButton(parent), longPressTimer(NULL), b_longpress(false) {}
QalculateToolButton::~QalculateToolButton() {}
void QalculateToolButton::mousePressEvent(QMouseEvent *e) {
	b_longpress = false;
	if(e->button() == Qt::LeftButton) {
		if(!longPressTimer) {
			longPressTimer = new QTimer(this);
			longPressTimer->setSingleShot(true);
			connect(longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
		}
		longPressTimer->start(500);
	}
	QToolButton::mousePressEvent(e);
}
void QalculateToolButton::longPressTimeout() {
	b_longpress = true;
	showMenu();
}
void QalculateToolButton::mouseReleaseEvent(QMouseEvent *e) {
	if(e->button() == Qt::RightButton) {
		showMenu();
	} else {
		if(longPressTimer && longPressTimer->isActive() && e->button() == Qt::LeftButton) {
			longPressTimer->stop();
		} else if(b_longpress && e->button() == Qt::LeftButton) {
			b_longpress = false;
			blockSignals(true);
			QToolButton::mouseReleaseEvent(e);
			blockSignals(false);
			return;
		}
		QToolButton::mouseReleaseEvent(e);
	}
}

QalculateDockWidget::QalculateDockWidget(const QString &name, QWidget *parent, ExpressionEdit *editwidget) : QDockWidget(name, parent), expressionEdit(editwidget) {}
QalculateDockWidget::QalculateDockWidget(QWidget *parent, ExpressionEdit *editwidget) : QDockWidget(parent), expressionEdit(editwidget) {}
QalculateDockWidget::~QalculateDockWidget() {}

void QalculateDockWidget::keyPressEvent(QKeyEvent *e) {
	QDockWidget::keyPressEvent(e);
	if(!e->isAccepted() && isFloating()) {
		expressionEdit->setFocus();
		expressionEdit->keyPressEvent(e);
	}
}
void QalculateDockWidget::closeEvent(QCloseEvent *e) {
	emit dockClosed();
	QDockWidget::closeEvent(e);
}

class QalculateTableWidget : public QTableWidget {

	public:

		QalculateTableWidget(QWidget *parent) : QTableWidget(parent) {}
		virtual ~QalculateTableWidget() {}

	protected:

		void keyPressEvent(QKeyEvent *e) override {
			if(state() != EditingState) {
				e->ignore();
				return;
			}
			QTableWidget::keyPressEvent(e);
		}

};

#define ADD_SECTION(str) \
	if(!menu->style()->styleHint(QStyle::SH_Menu_SupportsSections)) { \
		aw = new QWidgetAction(this); \
		QLabel *label = new QLabel(str, this); \
		label->setAlignment(Qt::AlignCenter); \
		aw->setDefaultWidget(label); \
		menu->addSeparator(); \
		menu->addAction(aw); \
		menu->addSeparator(); \
	} else { \
		menu->addSection(str); \
	}

#define SET_BINARY_BITS \
	int binary_bits = settings->printops.binary_bits;\
	if(binary_bits <= 0) binary_bits = 64;\
	else if(binary_bits > 32 && binary_bits % 32 != 0) binary_bits += (32 - binary_bits % 32);

QalculateWindow::QalculateWindow() : QMainWindow() {

	init_in_progress = true;

	QWidget *w_top = new QWidget(this);
	setCentralWidget(w_top);

	send_event = true;

	workspace_changed = false;

	ecTimer = NULL;
	emhTimer = NULL;
	resizeTimer = NULL;
	rfTimer = NULL;
	decimalsTimer = NULL;
	autoCalculateTimer = NULL;
	shortcutsDialog = NULL;
	preferencesDialog = NULL;
	functionsDialog = NULL;
	datasetsDialog = NULL;
	variablesDialog = NULL;
	unitsDialog = NULL;
	fpConversionDialog = NULL;
	percentageDialog = NULL;
	plotDialog = NULL;
	periodicTableDialog = NULL;
	calendarConversionDialog = NULL;

	mauto.setAborted();

	QVBoxLayout *topLayout = new QVBoxLayout(w_top);
	topLayout->setContentsMargins(0, 0, 0, 0);

	tb = addToolBar("Toolbar");
	tb->setContextMenuPolicy(Qt::CustomContextMenu);
	tb->setObjectName("Toolbar");
	tb->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	tb->setFloatable(false);
	tb->setMovable(false);
	tb->toggleViewAction()->setVisible(false);

	tbAction = new QAction(tr("Show toolbar"), this);
	tbAction->setCheckable(true);
	connect(tbAction, SIGNAL(triggered(bool)), tb, SLOT(setVisible(bool)));

	tmenu = NULL;
	connect(tb, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showToolbarContextMenu(const QPoint&)));

	ehSplitter = new QSplitter(Qt::Vertical, this);
	topLayout->addWidget(ehSplitter, 1);

	QAction *action; QActionGroup *group; QMenu *menu, *menu2;
	int w, w2; QWidgetAction *aw; QWidget *aww; QHBoxLayout *awl;

	QFont appfont;
	if(settings->use_custom_app_font) appfont.fromString(QString::fromStdString(settings->custom_app_font));

	menuAction_t = new QToolButton(this); menuAction_t->setIcon(LOAD_ICON("menu")); menuAction_t->setText(tr("Menu"));
	menuAction_t->setPopupMode(QToolButton::InstantPopup);
	menu = new QMenu(tr("Menu"), this);
	menuAction_t->setMenu(menu);
	menu2 = menu;
	menu = menu2->addMenu(tr("New"));
	newFunctionAction = menu->addAction(tr("Function…"), this, SLOT(newFunction()));
	newVariableAction = menu->addAction(tr("Variable/Constant…"), this, SLOT(newVariable()));
	menu->addAction(tr("Unknown Variable…"), this, SLOT(newUnknown()));
	menu->addAction(tr("Matrix…"), this, SLOT(newMatrix()));
	menu->addAction(tr("Unit…"), this, SLOT(newUnit()));
	menu = menu2->addMenu(tr("Workspaces"));
	openWSAction = menu->addAction(tr("Open…"), this, SLOT(openWorkspace()));
	defaultWSAction = menu->addAction(tr("Open default"), this, SLOT(openDefaultWorkspace()));
	menu->addSeparator();
	saveWSAction = menu->addAction(tr("Save"), this, SLOT(saveWorkspace()));
	saveWSAsAction = menu->addAction(tr("Save As…"), this, SLOT(saveWorkspaceAs()));
	recentWSMenu = menu;
	recentWSSeparator = NULL;
	updateWSActions();
	menu = menu2;
	menu->addSeparator();
	menu->addAction(tr("Import CSV File…"), this, SLOT(importCSV()));
	menu->addAction(tr("Export CSV File…"), this, SLOT(exportCSV()));
	menu->addSeparator();
	functionsAction = menu->addAction(tr("Functions"), this, SLOT(openFunctions()));
	variablesAction = menu->addAction(tr("Variables and Constants"), this, SLOT(openVariables()));
	unitsAction = menu->addAction(tr("Units"), this, SLOT(openUnits()));
	datasetsAction = menu->addAction(tr("Data Sets"), this, SLOT(openDatasets()));
	menu->addSeparator();
	plotAction = menu->addAction(tr("Plot Functions/Data"), this, SLOT(openPlot()));
	fpAction = menu->addAction(tr("Floating Point Conversion (IEEE 754)"), this, SLOT(openFPConversion()));
	calendarsAction = menu->addAction(tr("Calendar Conversion"), this, SLOT(openCalendarConversion()));
	percentageAction = menu->addAction(tr("Percentage Calculation Tool"), this, SLOT(openPercentageCalculation()));
	periodicTableAction = menu->addAction(tr("Periodic Table"), this, SLOT(openPeriodicTable()));
	menu->addSeparator();
	exratesAction = menu->addAction(tr("Update Exchange Rates"), this, SLOT(fetchExchangeRates()));
	menu->addSeparator();
	group = new QActionGroup(this);
	action = menu->addAction(tr("Normal Mode"), this, SLOT(normalModeActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_normalmode"); if(!settings->rpn_mode && !settings->chain_mode) action->setChecked(true);
	action = menu->addAction(tr("RPN Mode"), this, SLOT(rpnModeActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_rpnmode"); if(settings->rpn_mode) action->setChecked(true); rpnAction = action;
	action = menu->addAction(tr("Chain Mode"), this, SLOT(chainModeActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_chainmode"); if(settings->chain_mode) action->setChecked(true); chainAction = action;
	menu->addSeparator();
	menu->addAction(tr("Keyboard Shortcuts"), this, SLOT(editKeyboardShortcuts()));
	menu->addAction(tr("Preferences"), this, SLOT(editPreferences()));
	menu->addSeparator();
	helpAction = menu->addAction(tr("Help"), this, SLOT(help()));
	menu->addAction(tr("Report a Bug"), this, SLOT(reportBug()));
	menu->addAction(tr("Check for Updates"), this, SLOT(checkVersion()));
	menu->addAction(tr("About %1").arg("Qt"), qApp, SLOT(aboutQt()));
	menu->addAction(tr("About %1").arg("Qalculate!"), this, SLOT(showAbout()));
	menu->addSeparator();
	quitAction = menu->addAction(tr("Quit"), qApp, SLOT(closeAllWindows()));

	modeAction_t = new QToolButton(this); modeAction_t->setIcon(LOAD_ICON("configure")); modeAction_t->setText(tr("Mode"));
	modeAction_t->setPopupMode(QToolButton::InstantPopup);
	menu = new QMenu(tr("Mode"), this);
	modeAction_t->setMenu(menu);

	ADD_SECTION(tr("General Display Mode"));
	QFontMetrics fm1(settings->use_custom_app_font ? appfont : menu->font());
	menu->setToolTipsVisible(true);
	w = fm1.boundingRect(tr("General Display Mode")).width() * 1.5;
	group = new QActionGroup(this); group->setObjectName("group_general");
	action = menu->addAction(tr("Normal"), this, SLOT(normalActivated())); action->setCheckable(true); group->addAction(action);
	action->setToolTip("500 000<br>5 × 10<sup>14</sup><br>50 km/s<br>y − x<br>erf(10) ≈ 1.000 000 000"); normalAction = action;
	action->setData(EXP_PRECISION); if(settings->printops.min_exp == EXP_PRECISION) action->setChecked(true);
	action = menu->addAction(tr("Scientific"), this, SLOT(scientificActivated())); action->setCheckable(true); group->addAction(action);
	action->setToolTip("5 × 10<sup>5</sup><br>5 × 10<sup>4</sup> m·s<sup>−1</sup><br>−y + x<br>erf(10) ≈ 1.000 000 000"); sciAction = action;
	action->setData(EXP_SCIENTIFIC); if(settings->printops.min_exp == EXP_SCIENTIFIC) action->setChecked(true);
	action = menu->addAction(tr("Engineering"), this, SLOT(engineeringActivated())); action->setCheckable(true); group->addAction(action);
	action->setToolTip("500 × 10<sup>3</sup><br>50 × 10<sup>3</sup> m/s<br>−y + x<br>erf(10) ≈ 1.000 000 000"); engAction = action;
	action->setData(EXP_BASE_3); if(settings->printops.min_exp == EXP_BASE_3) action->setChecked(true);
	action = menu->addAction(tr("Simple"), this, SLOT(simpleActivated())); action->setCheckable(true); group->addAction(action);
	action->setToolTip("500 000 000 000 000<br>50 km/s<br>y − x<br>erf(10) ≈ 1"); simpleAction = action;
	action->setData(EXP_NONE); if(settings->printops.min_exp == EXP_NONE) action->setChecked(true);

	ADD_SECTION(tr("Angle Unit"));
	w2 = fm1.boundingRect(tr("Angle Unit")).width() * 1.5; if(w2 > w) w = w2;
	group = new QActionGroup(this); group->setObjectName("group_angleunit");
	action = menu->addAction(tr("Radians"), this, SLOT(angleUnitActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_radians"); radAction = action; action->setData(ANGLE_UNIT_RADIANS);
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_RADIANS) action->setChecked(true);
	action = menu->addAction(tr("Degrees"), this, SLOT(angleUnitActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_degrees"); degAction = action; action->setData(ANGLE_UNIT_DEGREES);
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_DEGREES) action->setChecked(true);
	action = menu->addAction(tr("Gradians"), this, SLOT(angleUnitActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_gradians"); graAction = action; action->setData(ANGLE_UNIT_GRADIANS);
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_GRADIANS) action->setChecked(true);
	menu2 = menu;
	angleMenu = menu2->addMenu(tr("Other"));
	menu = menu2;

	ADD_SECTION(tr("Approximation"));
	w2 = fm1.boundingRect(tr("Approximation")).width() * 1.5; if(w2 > w) w = w2;
	group = new QActionGroup(this);
	action = menu->addAction(tr("Automatic", "Automatic approximation"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_autoappr");
	action->setData(-1); assumptionTypeActions[0] = action; if(settings->dual_approximation < 0) action->setChecked(true);
	action = menu->addAction(tr("Dual", "Dual approximation"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_dualappr");
	action->setData(-2); assumptionTypeActions[0] = action; if(settings->dual_approximation > 0) action->setChecked(true);
	action = menu->addAction(tr("Exact", "Exact approximation"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(APPROXIMATION_EXACT); assumptionTypeActions[0] = action; if(settings->dual_approximation == 0 && settings->evalops.approximation == APPROXIMATION_EXACT) action->setChecked(true); action->setObjectName("action_exact");
	action = menu->addAction(tr("Approximate"), this, SLOT(approximationActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(APPROXIMATION_TRY_EXACT); assumptionTypeActions[0] = action; if(settings->dual_approximation == 0 && (settings->evalops.approximation == APPROXIMATION_APPROXIMATE || settings->evalops.approximation == APPROXIMATION_TRY_EXACT)) action->setChecked(true); action->setObjectName("action_approximate");

	menu->addSeparator();
	menu2 = menu;
	menu = menu2->addMenu(tr("Assumptions"));
	ADD_SECTION(tr("Type", "Assumptions type"));
	group = new QActionGroup(this); group->setObjectName("group_type");
	action = menu->addAction(tr("Number"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_NUMBER); assumptionTypeActions[0] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_NUMBER) action->setChecked(true);
	action = menu->addAction(tr("Real"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_REAL); assumptionTypeActions[1] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_REAL) action->setChecked(true);
	action = menu->addAction(tr("Rational"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_RATIONAL); assumptionTypeActions[2] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_RATIONAL) action->setChecked(true);
	action = menu->addAction(tr("Integer"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_INTEGER); assumptionTypeActions[3] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_INTEGER) action->setChecked(true);
	action = menu->addAction(tr("Boolean"), this, SLOT(assumptionsTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_TYPE_BOOLEAN); assumptionTypeActions[4] = action; if(CALCULATOR->defaultAssumptions()->type() == ASSUMPTION_TYPE_BOOLEAN) action->setChecked(true);
	ADD_SECTION(tr("Sign", "Assumptions sign"));
	group = new QActionGroup(this); group->setObjectName("group_sign");
	action = menu->addAction(tr("Unknown", "Unknown assumptions sign"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_UNKNOWN); assumptionSignActions[0] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_UNKNOWN) action->setChecked(true);
	action = menu->addAction(tr("Non-zero"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NONZERO); assumptionSignActions[1] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NONZERO) action->setChecked(true);
	action = menu->addAction(tr("Positive"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_POSITIVE); assumptionSignActions[2] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_POSITIVE) action->setChecked(true);
	action = menu->addAction(tr("Non-negative"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NONNEGATIVE); assumptionSignActions[3] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NONNEGATIVE) action->setChecked(true);
	action = menu->addAction(tr("Negative"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NEGATIVE); assumptionSignActions[4] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NEGATIVE) action->setChecked(true);
	action = menu->addAction(tr("Non-positive"), this, SLOT(assumptionsSignActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(ASSUMPTION_SIGN_NONPOSITIVE); assumptionSignActions[5] = action; if(CALCULATOR->defaultAssumptions()->sign() == ASSUMPTION_SIGN_NONPOSITIVE) action->setChecked(true);
	menu = menu2;

	ADD_SECTION(tr("Result Base"));
	w2 = fm1.boundingRect(tr("Result Base")).width() * 1.5; if(w2 > w) w = w2;
	bool base_checked = false;
	group = new QActionGroup(this); group->setObjectName("group_outbase");
	action = menu->addAction(tr("Binary"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BINARY); if(settings->printops.base == BASE_BINARY) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Octal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_OCTAL); if(settings->printops.base == BASE_OCTAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Decimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DECIMAL); if(settings->printops.base == BASE_DECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Hexadecimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_HEXADECIMAL); if(settings->printops.base == BASE_HEXADECIMAL) {base_checked = true; action->setChecked(true);}
	menu2 = menu;
	menu = menu2->addMenu(tr("Other"));
	action = menu->addAction(tr("Duodecimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DUODECIMAL); if(settings->printops.base == BASE_DUODECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Sexagesimal"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SEXAGESIMAL); if(settings->printops.base == BASE_SEXAGESIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Time format"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_TIME); if(settings->printops.base == BASE_TIME) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Roman numerals"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_ROMAN_NUMERALS); if(settings->printops.base == BASE_ROMAN_NUMERALS) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Unicode"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_UNICODE); if(settings->printops.base == BASE_UNICODE) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Bijective base-26"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BIJECTIVE_26); if(settings->printops.base == BASE_BIJECTIVE_26) action->setChecked(true);
	action = menu->addAction("Float", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_FP32); if(settings->printops.base == BASE_FP32) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("Double", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_FP64); if(settings->printops.base == BASE_FP64) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Binary-encoded decimal (BCD)"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BINARY_DECIMAL); if(settings->printops.base == BASE_BINARY_DECIMAL) action->setChecked(true);
	action = menu->addAction("φ", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_GOLDEN_RATIO); if(settings->printops.base == BASE_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("ψ", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SUPER_GOLDEN_RATIO); if(settings->printops.base == BASE_SUPER_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("π", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_PI); if(settings->printops.base == BASE_PI) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("e", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_E); if(settings->printops.base == BASE_E) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("√2", this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SQRT2); if(settings->printops.base == BASE_SQRT2) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Custom:", "Number base"), this, SLOT(outputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_CUSTOM); if(!base_checked) action->setChecked(true); customOutputBaseAction = action;
	aw = new QWidgetAction(this);
	aww = new QWidget(this);
	aw->setDefaultWidget(aww);
	awl = new QHBoxLayout(aww);
	customOutputBaseEdit = new QSpinBox(this);
	customOutputBaseEdit->setRange(INT_MIN, INT_MAX);
	customOutputBaseEdit->setValue(settings->printops.base == BASE_CUSTOM ? (CALCULATOR->customOutputBase().isZero() ? 10 : CALCULATOR->customOutputBase().intValue()) : ((settings->printops.base >= 2 && settings->printops.base <= 36) ? settings->printops.base : 10)); customOutputBaseEdit->setObjectName("spinbox_outbase");
	customOutputBaseEdit->setAlignment(Qt::AlignRight);
	connect(customOutputBaseEdit, SIGNAL(valueChanged(int)), this, SLOT(onCustomOutputBaseChanged(int)));
	awl->addWidget(customOutputBaseEdit, 0);
	menu->addAction(aw);
	menu = menu2;

	ADD_SECTION(tr("Expression Base"));
	w2 = fm1.boundingRect(tr("Expression Base")).width() * 1.5; if(w2 > w) w = w2;
	base_checked = false;
	group = new QActionGroup(this); group->setObjectName("group_inbase");
	action = menu->addAction(tr("Binary"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BINARY); if(settings->evalops.parse_options.base == BASE_BINARY) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Octal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_OCTAL); if(settings->evalops.parse_options.base == BASE_OCTAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Decimal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DECIMAL); if(settings->evalops.parse_options.base == BASE_DECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Hexadecimal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_HEXADECIMAL); if(settings->evalops.parse_options.base == BASE_HEXADECIMAL) {base_checked = true; action->setChecked(true);}
	menu2 = menu;
	menu = menu2->addMenu(tr("Other"));
	action = menu->addAction(tr("Duodecimal"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_DUODECIMAL); if(settings->evalops.parse_options.base == BASE_DUODECIMAL) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Roman numerals"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_ROMAN_NUMERALS); if(settings->evalops.parse_options.base == BASE_ROMAN_NUMERALS) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Unicode"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_UNICODE); if(settings->evalops.parse_options.base == BASE_UNICODE) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Bijective base-26"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BIJECTIVE_26); if(settings->evalops.parse_options.base == BASE_BIJECTIVE_26) action->setChecked(true);
	action = menu->addAction(tr("Binary-encoded decimal (BCD)"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_BINARY_DECIMAL); if(settings->evalops.parse_options.base == BASE_BINARY_DECIMAL) action->setChecked(true);
	action = menu->addAction("φ", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_GOLDEN_RATIO); if(settings->evalops.parse_options.base == BASE_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("ψ", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SUPER_GOLDEN_RATIO); if(settings->evalops.parse_options.base == BASE_SUPER_GOLDEN_RATIO) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("π", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_PI); if(settings->evalops.parse_options.base == BASE_PI) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("e", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_E); if(settings->evalops.parse_options.base == BASE_E) {base_checked = true; action->setChecked(true);}
	action = menu->addAction("√2", this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_SQRT2); if(settings->evalops.parse_options.base == BASE_SQRT2) {base_checked = true; action->setChecked(true);}
	action = menu->addAction(tr("Custom:", "Number base"), this, SLOT(inputBaseActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(BASE_CUSTOM); if(!base_checked) action->setChecked(true); customInputBaseAction = action;
	aw = new QWidgetAction(this);
	aww = new QWidget(this);
	aw->setDefaultWidget(aww);
	awl = new QHBoxLayout(aww);
	customInputBaseEdit = new QSpinBox(this);
	customInputBaseEdit->setRange(INT_MIN, INT_MAX);
	customInputBaseEdit->setValue(settings->evalops.parse_options.base == BASE_CUSTOM ? (CALCULATOR->customInputBase().isZero() ? 10 : CALCULATOR->customInputBase().intValue()) : ((settings->evalops.parse_options.base >= 2 && settings->evalops.parse_options.base <= 36) ? settings->evalops.parse_options.base : 10)); customInputBaseEdit->setObjectName("spinbox_inbase");
	customInputBaseEdit->setAlignment(Qt::AlignRight);
	connect(customInputBaseEdit, SIGNAL(valueChanged(int)), this, SLOT(onCustomInputBaseChanged(int)));
	awl->addWidget(customInputBaseEdit, 0);
	menu->addAction(aw);
	menu = menu2;
	menu->addSeparator();

	aw = new QWidgetAction(this);
	aww = new QWidget(this);
	aw->setDefaultWidget(aww);
	QGridLayout *awg = new QGridLayout(aww);
	awg->addWidget(new QLabel(tr("Precision:"), this), 0, 0);
	QSpinBox *precisionEdit = new QSpinBox(this); precisionEdit->setObjectName("spinbox_precision");
	precisionEdit->setRange(2, 10000);
	precisionEdit->setValue(CALCULATOR->getPrecision());
	precisionEdit->setAlignment(Qt::AlignRight);
	connect(precisionEdit, SIGNAL(valueChanged(int)), this, SLOT(onPrecisionChanged(int)));
	awg->addWidget(precisionEdit, 0, 1);
	awg->addWidget(new QLabel(tr("Min decimals:"), this), 1, 0);
	minDecimalsEdit = new QSpinBox(this); minDecimalsEdit->setObjectName("spinbox_mindecimals");
	minDecimalsEdit->setRange(0, 10000);
	minDecimalsEdit->setValue(settings->printops.use_min_decimals ? settings->printops.min_decimals : 0);
	minDecimalsEdit->setAlignment(Qt::AlignRight);
	connect(minDecimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(onMinDecimalsChanged(int)));
	awg->addWidget(minDecimalsEdit, 1, 1);
	awg->addWidget(new QLabel(tr("Max decimals:"), this), 2, 0);
	maxDecimalsEdit = new QSpinBox(this); maxDecimalsEdit->setObjectName("spinbox_maxdecimals");
	maxDecimalsEdit->setRange(-1, 10000);
	maxDecimalsEdit->setSpecialValueText(tr("off", "Max decimals"));
	maxDecimalsEdit->setValue(settings->printops.use_max_decimals ? settings->printops.max_decimals : -1);
	maxDecimalsEdit->setAlignment(Qt::AlignRight);
	connect(maxDecimalsEdit, SIGNAL(valueChanged(int)), this, SLOT(onMaxDecimalsChanged(int)));
	awg->addWidget(maxDecimalsEdit, 2, 1);
	menu->addAction(aw);

	menu->setMinimumWidth(w);
	tb->addWidget(modeAction_t);
	modeAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);

	toAction_t = new QalculateToolButton(this); toAction_t->setIcon(LOAD_ICON("convert")); toAction_t->setText(tr("Convert"));
	toAction_t->setEnabled(false);
	connect(toAction_t, SIGNAL(clicked()), this, SLOT(onToActivated()));
	toMenu = new QMenu(this);
	tb->addWidget(toAction_t);
	toAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	storeAction_t = new QalculateToolButton(this); storeAction_t->setIcon(LOAD_ICON("document-save")); storeAction_t->setText(tr("Store"));
	storeAction_t->setPopupMode(QToolButton::MenuButtonPopup);
	connect(storeAction_t, SIGNAL(clicked()), this, SLOT(onStoreActivated()));
	variablesMenu = new QMenu(this);
	storeAction_t->setMenu(variablesMenu);
	tb->addWidget(storeAction_t);
	storeAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	functionsAction_t = new QalculateToolButton(this); functionsAction_t->setIcon(LOAD_ICON("function")); functionsAction_t->setText(tr("Functions"));
	functionsAction_t->setPopupMode(QToolButton::MenuButtonPopup);
	connect(functionsAction_t, SIGNAL(clicked()), this, SLOT(openFunctions()));
	functionsMenu = new QMenu(this);
	functionsAction_t->setMenu(functionsMenu);
	tb->addWidget(functionsAction_t);
	functionsAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	unitsAction_t = new QalculateToolButton(this); unitsAction_t->setIcon(LOAD_ICON("units")); unitsAction_t->setText(tr("Units"));
	unitsAction_t->setPopupMode(QToolButton::MenuButtonPopup);
	connect(unitsAction_t, SIGNAL(clicked()), this, SLOT(openUnits()));
	unitsMenu = new QMenu(this);
	unitsAction_t->setMenu(unitsMenu);
	tb->addWidget(unitsAction_t);
	unitsAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	if(CALCULATOR->canPlot()) {
		plotAction_t = new QAction(LOAD_ICON("plot"), tr("Plot"), this);
		connect(plotAction_t, SIGNAL(triggered(bool)), this, SLOT(openPlot()));
		tb->addAction(plotAction_t);
	} else {
		plotAction_t = NULL;
	}
	basesAction = new QAction(LOAD_ICON("number-bases"), tr("Number bases"), this);
	connect(basesAction, SIGNAL(triggered(bool)), this, SLOT(onBasesActivated(bool)));
	basesAction->setCheckable(true);
	tb->addAction(basesAction);
	keypadAction_t = new QToolButton(this); keypadAction_t->setIcon(LOAD_ICON("keypad")); keypadAction_t->setText(tr("Keypad"));
	keypadAction_t->setToolTip(tr("Keypad")); keypadAction_t->setPopupMode(QToolButton::InstantPopup);
	menu = new QMenu(this);
	keypadAction_t->setMenu(menu);
	group = new QActionGroup(this); group->setObjectName("group_keypad");
	action = menu->addAction(tr("General"), this, SLOT(keypadTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(KEYPAD_GENERAL); gKeypadAction = action;
	action = menu->addAction(tr("Programming"), this, SLOT(keypadTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(KEYPAD_PROGRAMMING); pKeypadAction = action;
	action = menu->addAction(tr("Algebra"), this, SLOT(keypadTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(KEYPAD_ALGEBRA); xKeypadAction = action;
	action = menu->addAction(tr("Custom"), this, SLOT(keypadTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(KEYPAD_CUSTOM); cKeypadAction = action;
	action = menu->addAction(tr("Number Pad"), this, SLOT(keypadTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(KEYPAD_NUMBERPAD); action->setEnabled(settings->hide_numpad); nKeypadAction = action;
	action = menu->addAction(tr("None"), this, SLOT(keypadTypeActivated())); action->setCheckable(true); group->addAction(action);
	action->setData(-1); action->setChecked(true); keypadAction = action;
	menu->addSeparator();
	action = menu->addAction(tr("Always Show Number Pad"), this, SLOT(showNumpad(bool))); action->setCheckable(true); action->setChecked(!settings->hide_numpad); showNumpadAction = action;
	action = menu->addAction(tr("Separate Menu Buttons"), this, SLOT(showSeparateKeypadMenuButtons(bool))); action->setCheckable(true); action->setChecked(settings->separate_keypad_menu_buttons);
	action = menu->addAction(tr("Reset Keypad Position"), this, SLOT(resetKeypadPosition())); action->setEnabled(false); resetKeypadPositionAction = action;
	tb->addWidget(keypadAction_t);
	keypadAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	QWidget *spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	tb->addWidget(spacer);
	tb->addWidget(menuAction_t);
	menuAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);

	expressionEdit = new ExpressionEdit(this, tb);
	QFont font = expressionEdit->font();
	if(font.pixelSize() >= 0) {
		font.setPixelSize(font.pixelSize() * 1.35);
	} else {
		font.setPointSize(font.pointSize() * 1.35);
	}
	expressionEdit->setFont(font);
	expressionEdit->setFocus();
	ehSplitter->addWidget(expressionEdit);
	historyView = new HistoryView(this);
	historyView->expressionEdit = expressionEdit;

	ehSplitter->addWidget(historyView);
	ehSplitter->setStretchFactor(0, 0);
	ehSplitter->setStretchFactor(1, 1);
	ehSplitter->setCollapsible(0, false);
	ehSplitter->setCollapsible(1, false);

	basesDock = new QalculateDockWidget(tr("Number bases"), this, expressionEdit);
	basesDock->setObjectName("number-bases-dock");
	QWidget *basesWidget = new QWidget(this);
	QGridLayout *basesGrid = new QGridLayout(basesWidget);
	binLabel = new QLabel(tr("Binary:"));
	basesGrid->addWidget(binLabel, 0, 0, Qt::AlignTop);
	octLabel = new QLabel(tr("Octal:"));
	basesGrid->addWidget(octLabel, 1, 0);
	decLabel = new QLabel(tr("Decimal:"));
	basesGrid->addWidget(decLabel, 2, 0);
	hexLabel = new QLabel(tr("Hexadecimal:"));
	basesGrid->addWidget(hexLabel, 3, 0);

	binEdit = new QLabel();
	binEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse | Qt::TextSelectableByKeyboard);
	binEdit->setFocusPolicy(Qt::NoFocus);
	updateBinEditSize(settings->use_custom_app_font ? &appfont : NULL);
	binEdit->setAlignment(Qt::AlignRight | Qt::AlignTop);
	basesGrid->addWidget(binEdit, 0, 1);
	octEdit = new QLabel("0");
	QFontMetrics fm2(settings->use_custom_app_font ? appfont : octEdit->font());
	octEdit->setAlignment(Qt::AlignRight);
	octEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	octEdit->setFocusPolicy(Qt::NoFocus);
	octEdit->setMinimumHeight(fm2.lineSpacing());
	basesGrid->addWidget(octEdit, 1, 1);
	decEdit = new QLabel("0");
	decEdit->setAlignment(Qt::AlignRight);
	decEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	decEdit->setFocusPolicy(Qt::NoFocus);
	decEdit->setMinimumHeight(fm2.lineSpacing());
	basesGrid->addWidget(decEdit, 2, 1);
	hexEdit = new QLabel("0");
	hexEdit->setAlignment(Qt::AlignRight);
	hexEdit->setTextInteractionFlags(Qt::TextSelectableByMouse);
	hexEdit->setFocusPolicy(Qt::NoFocus);
	hexEdit->setMinimumHeight(fm2.lineSpacing());
	SET_BINARY_BITS
	result_bin = "";
	result_hex = "";
	for(int i = binary_bits; i > 0; i -= 4) {
		if(i < binary_bits) result_bin += " ";
		result_bin += "0000";
	}
	for(int i = binary_bits > 128 ? 128 : binary_bits; i > 0; i -= 8) {
		if(i < binary_bits) result_hex += " ";
		result_hex += "00";
	}
	result_oct = "0";
	result_dec = "0";
	updateResultBases();
	basesGrid->addWidget(hexEdit, 3, 1);
	basesGrid->setRowStretch(4, 1);
	basesDock->setWidget(basesWidget);
	addDockWidget(Qt::TopDockWidgetArea, basesDock);
	basesDock->hide();
	basesMenu = NULL;
	binEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(binEdit, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showBasesContextMenu(const QPoint&)));
	octEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(octEdit, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showBasesContextMenu(const QPoint&)));
	decEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(decEdit, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showBasesContextMenu(const QPoint&)));
	hexEdit->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(hexEdit, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showBasesContextMenu(const QPoint&)));
	connect(binEdit, SIGNAL(linkActivated(const QString&)), this, SLOT(resultBasesLinkActivated(const QString&)));

	keypad = new KeypadWidget(this);
	keypadDock = new QalculateDockWidget(this, expressionEdit);
	keypadDock->setObjectName("keypad-dock");
	keypadDock->setWidget(keypad);
	if(!settings->hide_numpad && get_screen_geometry(this).width() < keypad->sizeHint().width() + 50) {
		showNumpadAction->trigger();
		if(settings->show_keypad) nKeypadAction->trigger();
	}
	updateKeypadTitle();
	keypadDock->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(keypadDock, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showKeypadContextMenu(const QPoint&)));
	addDockWidget(Qt::BottomDockWidgetArea, keypadDock);

	QWidget *rpnWidget = new QWidget(this);
	rpnDock = new QalculateDockWidget(tr("RPN Stack"), this, expressionEdit);
	rpnDock->setObjectName("rpn-dock");
	QHBoxLayout *rpnBox = new QHBoxLayout(rpnWidget);
	rpnView = new QalculateTableWidget(this);
	rpnView->setColumnCount(1);
	rpnView->setRowCount(0);
	rpnView->setSelectionBehavior(QAbstractItemView::SelectRows);
	rpnView->horizontalHeader()->setStretchLastSection(true);
	rpnView->horizontalHeader()->hide();
	rpnView->verticalHeader()->setSectionsMovable(true);
	rpnView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
	rpnView->setSelectionMode(QAbstractItemView::SingleSelection);
	rpnView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
	connect(rpnView, SIGNAL(cellChanged(int, int)), this, SLOT(registerChanged(int)));
	rpnBox->addWidget(rpnView);
	QToolBar *rpnTB = new QToolBar(this);
	rpnTB->setToolButtonStyle(Qt::ToolButtonIconOnly);
	rpnTB->setOrientation(Qt::Vertical);
	rpnBox->addWidget(rpnTB);
	rpnUpAction = new QAction(LOAD_ICON("go-up"), "RPN Up", this); rpnUpAction->setEnabled(false);
	connect(rpnUpAction, SIGNAL(triggered(bool)), this, SLOT(registerUp()));
	rpnTB->addAction(rpnUpAction);
	rpnDownAction = new QAction(LOAD_ICON("go-down"), "RPN Down", this); rpnDownAction->setEnabled(false);
	connect(rpnDownAction, SIGNAL(triggered(bool)), this, SLOT(registerDown()));
	rpnTB->addAction(rpnDownAction);
	rpnSwapAction = new QAction(LOAD_ICON("rpn-swap"), "RPN Swap", this); rpnSwapAction->setEnabled(false);
	connect(rpnSwapAction, SIGNAL(triggered(bool)), this, SLOT(registerSwap()));
	rpnTB->addAction(rpnSwapAction);
	rpnCopyAction = new QAction(LOAD_ICON("edit-copy"), "RPN Copy", this); rpnCopyAction->setEnabled(false);
	connect(rpnCopyAction, SIGNAL(triggered(bool)), this, SLOT(copyRegister()));
	rpnTB->addAction(rpnCopyAction);
	rpnLastxAction = new QAction(LOAD_ICON("edit-undo"), "RPN LastX", this); rpnLastxAction->setEnabled(false);
	connect(rpnLastxAction, SIGNAL(triggered(bool)), this, SLOT(rpnLastX()));
	rpnTB->addAction(rpnLastxAction);
	rpnDeleteAction = new QAction(LOAD_ICON("edit-delete"), "RPN Delete", this); rpnDeleteAction->setEnabled(false);
	connect(rpnDeleteAction, SIGNAL(triggered(bool)), this, SLOT(deleteRegister()));
	rpnTB->addAction(rpnDeleteAction);
	rpnClearAction = new QAction(LOAD_ICON("edit-clear"), "RPN Clear", this); rpnClearAction->setEnabled(false);
	connect(rpnClearAction, SIGNAL(triggered(bool)), this, SLOT(clearStack()));
	rpnTB->addAction(rpnClearAction);
	rpnDock->setWidget(rpnWidget);
	addDockWidget(Qt::TopDockWidgetArea, rpnDock);
	if(!settings->rpn_mode) rpnDock->hide();

	QLocalServer::removeServer("qalculate-qt");
	server = new QLocalServer(this);
	server->listen("qalculate-qt");
	connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));

	mstruct = new MathStructure();
	settings->current_result = NULL;
	mstruct_exact.setUndefined();
	prepend_mstruct.setUndefined();
	parsed_mstruct = new MathStructure();
	parsed_tostruct = new MathStructure();
	viewThread = new ViewThread;
	commandThread = new CommandThread;

	settings->printops.can_display_unicode_string_arg = (void*) historyView;

	b_busy = 0;

	expressionEdit->setFocus();

	if(settings->custom_result_font.empty()) settings->custom_result_font = historyView->font().toString().toStdString();
	if(settings->custom_expression_font.empty()) settings->custom_expression_font = expressionEdit->font().toString().toStdString();
	if(settings->custom_keypad_font.empty()) settings->custom_keypad_font = keypad->font().toString().toStdString();
	if(settings->custom_app_font.empty()) settings->custom_app_font = QApplication::font().toString().toStdString();
	if(settings->use_custom_keypad_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_keypad_font)); keypad->setFont(font);}
	if(settings->use_custom_expression_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_expression_font)); expressionEdit->setFont(font);}
	if(settings->use_custom_result_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_result_font)); historyView->setFont(font); rpnView->setFont(font);}

	loadShortcuts();

	connect(functionsMenu, SIGNAL(aboutToShow()), this, SLOT(initializeFunctionsMenu()));
	connect(variablesMenu, SIGNAL(aboutToShow()), this, SLOT(initializeVariablesMenu()));
	connect(unitsMenu, SIGNAL(aboutToShow()), this, SLOT(initializeUnitsMenu()));
	connect(historyView, SIGNAL(insertTextRequested(std::string)), this, SLOT(onInsertTextRequested(std::string)));
	connect(historyView, SIGNAL(insertValueRequested(int)), this, SLOT(onInsertValueRequested(int)));
	connect(historyView, SIGNAL(historyReloaded()), this, SLOT(onHistoryReloaded()));
	connect(expressionEdit, SIGNAL(returnPressed()), this, SLOT(calculate()));
	connect(expressionEdit, SIGNAL(textChanged()), this, SLOT(onExpressionChanged()));
	connect(expressionEdit, SIGNAL(statusChanged(QString, bool, bool, bool, bool)), this, SLOT(onStatusChanged(QString, bool, bool, bool, bool)));
	connect(expressionEdit, SIGNAL(toConversionRequested(std::string)), this, SLOT(onToConversionRequested(std::string)));
	connect(expressionEdit, SIGNAL(calculateRPNRequest(int)), this, SLOT(calculateRPN(int)));
	connect(expressionEdit, SIGNAL(expressionStatusModeChanged(bool)), this, SLOT(onExpressionStatusModeChanged(bool)));
	connect(tb, SIGNAL(visibilityChanged(bool)), this, SLOT(onToolbarVisibilityChanged(bool)));
	connect(keypadDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onKeypadVisibilityChanged(bool)));
	connect(basesDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onBasesVisibilityChanged(bool)));
	connect(rpnDock, SIGNAL(visibilityChanged(bool)), this, SLOT(onRPNVisibilityChanged(bool)));
	connect(rpnDock, SIGNAL(dockClosed()), this, SLOT(onRPNClosed()));
	connect(keypad, SIGNAL(expressionCalculationUpdated(int)), this, SLOT(expressionCalculationUpdated(int)));
	connect(keypad, SIGNAL(expressionFormatUpdated(bool)), this, SLOT(expressionFormatUpdated(bool)));
	connect(keypad, SIGNAL(resultFormatUpdated(int)), this, SLOT(resultFormatUpdated(int)));
	connect(keypad, SIGNAL(expressionCalculationUpdated(int)), this, SLOT(keypadPreferencesChanged()));
	connect(keypad, SIGNAL(expressionFormatUpdated(bool)), this, SLOT(keypadPreferencesChanged()));
	connect(keypad, SIGNAL(resultFormatUpdated(int)), this, SLOT(keypadPreferencesChanged()));
	connect(keypad, SIGNAL(shortcutClicked(int, const QString&)), this, SLOT(shortcutClicked(int, const QString&)));
	connect(keypad, SIGNAL(symbolClicked(const QString&)), this, SLOT(onSymbolClicked(const QString&)));
	connect(keypad, SIGNAL(operatorClicked(const QString&)), this, SLOT(onOperatorClicked(const QString&)));
	connect(keypad, SIGNAL(functionClicked(MathFunction*)), this, SLOT(onFunctionClicked(MathFunction*)));
	connect(keypad, SIGNAL(variableClicked(Variable*)), this, SLOT(onVariableClicked(Variable*)));
	connect(keypad, SIGNAL(unitClicked(Unit*)), this, SLOT(onUnitClicked(Unit*)));
	connect(keypad, SIGNAL(prefixClicked(Prefix*)), this, SLOT(onPrefixClicked(Prefix*)));
	connect(keypad, SIGNAL(delClicked()), this, SLOT(onDelClicked()));
	connect(keypad, SIGNAL(clearClicked()), this, SLOT(onClearClicked()));
	connect(keypad, SIGNAL(equalsClicked()), this, SLOT(onEqualsClicked()));
	connect(keypad, SIGNAL(parenthesesClicked()), this, SLOT(onParenthesesClicked()));
	connect(keypad, SIGNAL(bracketsClicked()), this, SLOT(onBracketsClicked()));
	connect(keypad, SIGNAL(endClicked()), this, SLOT(onEndClicked()));
	connect(keypad, SIGNAL(startClicked()), this, SLOT(onStartClicked()));
	connect(keypad, SIGNAL(leftClicked()), this, SLOT(onLeftClicked()));
	connect(keypad, SIGNAL(rightClicked()), this, SLOT(onRightClicked()));
	connect(keypad, SIGNAL(backspaceClicked()), this, SLOT(onBackspaceClicked()));
	connect(keypad, SIGNAL(MSClicked()), this, SLOT(onMSClicked()));
	connect(keypad, SIGNAL(MRClicked()), this, SLOT(onMRClicked()));
	connect(keypad, SIGNAL(MCClicked()), this, SLOT(onMCClicked()));
	connect(keypad, SIGNAL(MPlusClicked()), this, SLOT(onMPlusClicked()));
	connect(keypad, SIGNAL(MMinusClicked()), this, SLOT(onMMinusClicked()));
	connect(keypad, SIGNAL(storeClicked()), this, SLOT(onStoreActivated()));
	connect(keypad, SIGNAL(newFunctionClicked()), this, SLOT(newFunction()));
	connect(keypad, SIGNAL(answerClicked()), this, SLOT(onAnswerClicked()));
	connect(keypad, SIGNAL(baseClicked(int, bool, bool)), this, SLOT(onBaseClicked(int, bool, bool)));
	connect(keypad, SIGNAL(factorizeClicked()), this, SLOT(onFactorizeClicked()));
	connect(keypad, SIGNAL(expandClicked()), this, SLOT(onExpandClicked()));
	connect(keypad, SIGNAL(expandPartialFractionsClicked()), this, SLOT(onExpandPartialFractionsClicked()));
	connect(keypad, SIGNAL(openVariablesRequest()), this, SLOT(openVariables()));
	connect(keypad, SIGNAL(openUnitsRequest()), this, SLOT(openUnits()));
	connect(keypad, SIGNAL(openPercentageCalculationRequest()), this, SLOT(openPercentageCalculation()));
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	connect(QGuiApplication::styleHints(), SIGNAL(colorSchemeChanged(Qt::ColorScheme)), this, SLOT(onColorSchemeChanged()));
#endif

	if(settings->enable_tooltips != 1) onEnableTooltipsChanged();

	if(!settings->window_geometry.isEmpty()) restoreGeometry(settings->window_geometry);
	if(settings->window_geometry.isEmpty() || (settings->preferences_version[0] == 3 && settings->preferences_version[1] < 22 && height() == 650 && width() == 600)) try_resize(this, DEFAULT_WIDTH, DEFAULT_HEIGHT);
	if(!settings->window_state.isEmpty()) restoreState(settings->window_state);
	if(!settings->splitter_state.isEmpty()) ehSplitter->restoreState(settings->splitter_state);

	if(settings->show_bases >= 0) basesDock->setVisible(settings->show_bases > 0);
	if(settings->show_keypad >= 0) keypadDock->setVisible(settings->show_keypad > 0);
	rpnDock->setVisible(settings->rpn_mode);

	if(settings->always_on_top) setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

	if(settings->use_custom_app_font) {
		QTimer *timer = new QTimer();
		timer->setSingleShot(true);
		connect(timer, SIGNAL(timeout()), this, SLOT(onAppFontTimer()));
		timer->start(1);
	}

}
QalculateWindow::~QalculateWindow() {}

void QalculateWindow::initializeFunctionsMenu() {
	if(functionsMenu->isEmpty()) updateFunctionsMenu();
}
void QalculateWindow::initializeVariablesMenu() {
	if(variablesMenu->isEmpty()) updateVariablesMenu();
}
void QalculateWindow::initializeUnitsMenu() {
	if(unitsMenu->isEmpty()) updateUnitsMenu();
}

void QalculateWindow::keypadPreferencesChanged() {
	if(preferencesDialog) {
		preferencesDialog->updateVariableUnits();
		preferencesDialog->updateComplexForm();
		preferencesDialog->updateIntervalDisplay();
		preferencesDialog->updateIntervalCalculation();
		preferencesDialog->updateConciseUncertaintyInput();
	}
}
void QalculateWindow::updateKeypadTitle() {
	QString str = tr("Keypad") + " (";
	if(settings->keypad_type == KEYPAD_GENERAL) str += tr("General");
	else if(settings->keypad_type == KEYPAD_PROGRAMMING) str += tr("Programming");
	else if(settings->keypad_type == KEYPAD_ALGEBRA) str += tr("Algebra");
	else if(settings->keypad_type == KEYPAD_CUSTOM) str += tr("Custom");
	else if(settings->keypad_type == KEYPAD_NUMBERPAD) str += tr("Number Pad");
	str += ")";
	keypadDock->setWindowTitle(str);
}
void QalculateWindow::showKeypadContextMenu(const QPoint &pos) {
	QPoint p = keypadDock->mapToGlobal(pos);
	if(p.y() < keypad->mapToGlobal(QPoint(0, 0)).y()) keypadAction_t->menu()->popup(p);
}
void QalculateWindow::showToolbarContextMenu(const QPoint &pos) {
	QWidget *w = tb->childAt(pos);
	if(w == storeAction_t || w == functionsAction_t || w == unitsAction_t) return;
	if(!tmenu) {
		tmenu = new QMenu(this);
		QActionGroup *group = new QActionGroup(this);
		QAction *action = tmenu->addAction(tr("Icons only"), this, SLOT(setToolbarStyle()));
		action->setCheckable(true);
		action->setChecked(tb->toolButtonStyle() == Qt::ToolButtonIconOnly);
		action->setData(Qt::ToolButtonIconOnly);
		group->addAction(action);
		action = tmenu->addAction(tr("Text only"), this, SLOT(setToolbarStyle()));
		action->setCheckable(true);
		action->setChecked(tb->toolButtonStyle() == Qt::ToolButtonTextOnly);
		action->setData(Qt::ToolButtonTextOnly);
		group->addAction(action);
		action = tmenu->addAction(tr("Text beside icons"), this, SLOT(setToolbarStyle()));
		action->setCheckable(true);
		action->setChecked(tb->toolButtonStyle() == Qt::ToolButtonTextBesideIcon);
		action->setData(Qt::ToolButtonTextBesideIcon);
		group->addAction(action);
		action = tmenu->addAction(tr("Text under icons"), this, SLOT(setToolbarStyle()));
		action->setCheckable(true);
		action->setChecked(tb->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);
		action->setData(Qt::ToolButtonTextUnderIcon);
		group->addAction(action);
		tmenu->addSeparator();
		tmenu->addAction(tbAction);
	}
	tmenu->popup(tb->mapToGlobal(pos));
}
void QalculateWindow::showBasesContextMenu(const QPoint &pos) {
	if(((QLabel*) sender())->text().isEmpty()) return;
	if(!basesMenu) {
		basesMenu = new QMenu(this);
		copyBasesAction = basesMenu->addAction(tr("Copy"), this, SLOT(copyBases()));
	}
	copyBasesAction->setData(QVariant::fromValue((void*) sender()));
	basesMenu->popup(((QWidget*) sender())->mapToGlobal(pos));
}
void QalculateWindow::copyBases() {
	QLabel *label = (QLabel*) copyBasesAction->data().value<void*>();
	QApplication::clipboard()->setText(label->hasSelectedText() ? label->selectedText() : (label == binEdit ? QString::fromStdString(result_bin) : label->text()));
}
void QalculateWindow::setToolbarStyle() {
	QAction *action = qobject_cast<QAction*>(sender());
	settings->toolbar_style = action->data().toInt();
	tb->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	modeAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	toAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	storeAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	functionsAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	unitsAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	menuAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
	keypadAction_t->setToolButtonStyle((Qt::ToolButtonStyle) settings->toolbar_style);
}

void QalculateWindow::loadShortcuts() {
	if(plotAction_t) plotAction_t->setToolTip(tr("Plot Functions/Data"));
	storeAction_t->setToolTip(tr("Store") + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
	unitsAction_t->setToolTip(tr("Units") + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
	functionsAction_t->setToolTip(tr("Functions") + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
	basesAction->setToolTip(tr("Number Bases"));
	toAction_t->setToolTip(tr("Convert"));
	menuAction_t->setToolTip(tr("Menu"));
	modeAction_t->setToolTip(tr("Mode"));
	rpnUpAction->setToolTip(tr("Rotate the stack or move the selected register up"));
	rpnDownAction->setToolTip(tr("Rotate the stack or move the selected register down"));
	rpnSwapAction->setToolTip(tr("Swap the top two values or move the selected value to the top of the stack"));
	rpnDeleteAction->setToolTip(tr("Delete the top or selected value"));
	rpnLastxAction->setToolTip(tr("Enter the top value from before the last numeric operation"));
	rpnCopyAction->setToolTip(tr("Copy the selected or top value to the top of the stack"));
	rpnClearAction->setToolTip(tr("Clear the RPN stack"));
	for(size_t i = 0; i < settings->keyboard_shortcuts.size(); i++) {
		keyboardShortcutAdded(settings->keyboard_shortcuts[i]);
	}
}
void QalculateWindow::keyboardShortcutRemoved(keyboard_shortcut *ks) {
	if(ks->type[0] == SHORTCUT_TYPE_COMPLETE && ks->key == "Tab") {
		expressionEdit->enableTabCompletion(false);
		return;
	}
	if(!ks->action) return;
	if(ks->new_action) {
		removeAction(ks->action);
		ks->action->deleteLater();
		return;
	}
	QList<QKeySequence> shortcuts = ks->action->shortcuts();
	shortcuts.removeAll(QKeySequence::fromString(ks->key));
	ks->action->setShortcuts(shortcuts);
	if(ks->type[0] == SHORTCUT_TYPE_PLOT && plotAction_t) {
		plotAction_t->setToolTip(tr("Plot Functions/Data") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_STORE) {
		storeAction_t->setToolTip(tr("Store") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))) + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
	} else if(ks->type[0] == SHORTCUT_TYPE_MANAGE_UNITS) {
		unitsAction_t->setToolTip(tr("Units") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))) + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
	} else if(ks->type[0] == SHORTCUT_TYPE_MANAGE_FUNCTIONS) {
		functionsAction_t->setToolTip(tr("Functions") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))) + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
	} else if(ks->type[0] == SHORTCUT_TYPE_NUMBER_BASES) {
		basesAction->setToolTip(tr("Number Bases") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_CONVERT) {
		toAction_t->setToolTip(tr("Convert") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_MODE) {
		modeAction_t->setToolTip(tr("Mode") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_MENU) {
		menuAction_t->setToolTip(tr("Menu") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_UP) {
		rpnUpAction->setToolTip(tr("Rotate the stack or move the selected register up") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_DOWN) {
		rpnDownAction->setToolTip(tr("Rotate the stack or move the selected register down") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_SWAP) {
		rpnSwapAction->setToolTip(tr("Swap the top two values or move the selected value to the top of the stack") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_DELETE) {
		rpnDeleteAction->setToolTip(tr("Delete the top or selected value") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_LASTX) {
		rpnLastxAction->setToolTip(tr("Enter the top value from before the last numeric operation") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_COPY) {
		rpnCopyAction->setToolTip(tr("Copy the selected or top value to the top of the stack") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	} else if(ks->type[0] == SHORTCUT_TYPE_RPN_CLEAR) {
		rpnClearAction->setToolTip(tr("Clear the RPN stack") + (shortcuts.isEmpty() ? QString() : QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText))));
	}
}
void QalculateWindow::keyboardShortcutAdded(keyboard_shortcut *ks) {
	if(ks->type.size() == 1 && ks->type[0] == SHORTCUT_TYPE_COMPLETE && ks->key == "Tab") {
		expressionEdit->enableTabCompletion(true);
		ks->new_action = false;
		return;
	}
	QAction *action = NULL;
	if(ks->type.size() == 1) {
		switch(ks->type[0]) {
			case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {action = functionsAction; break;}
			case SHORTCUT_TYPE_MANAGE_VARIABLES: {action = variablesAction; break;}
			case SHORTCUT_TYPE_MANAGE_UNITS: {action = unitsAction; break;}
			case SHORTCUT_TYPE_MANAGE_DATA_SETS: {action = datasetsAction; break;}
			case SHORTCUT_TYPE_FLOATING_POINT: {action = fpAction; break;}
			case SHORTCUT_TYPE_CALENDARS: {action = calendarsAction; break;}
			case SHORTCUT_TYPE_PERIODIC_TABLE: {action = periodicTableAction; break;}
			case SHORTCUT_TYPE_PERCENTAGE_TOOL: {action = percentageAction; break;}
			case SHORTCUT_TYPE_NUMBER_BASES: {action = basesAction; break;}
			case SHORTCUT_TYPE_RPN_MODE: {action = rpnAction; break;}
			case SHORTCUT_TYPE_DEGREES: {action = degAction; break;}
			case SHORTCUT_TYPE_RADIANS: {action = radAction; break;}
			case SHORTCUT_TYPE_GRADIANS: {action = graAction; break;}
			case SHORTCUT_TYPE_NORMAL_NOTATION: {action = normalAction; break;}
			case SHORTCUT_TYPE_SCIENTIFIC_NOTATION: {action = sciAction; break;}
			case SHORTCUT_TYPE_ENGINEERING_NOTATION: {action = engAction; break;}
			case SHORTCUT_TYPE_SIMPLE_NOTATION: {action = simpleAction; break;}
			case SHORTCUT_TYPE_CHAIN_MODE: {action = chainAction; break;}
			case SHORTCUT_TYPE_KEYPAD: {action = keypadAction; break;}
			case SHORTCUT_TYPE_GENERAL_KEYPAD: {action = gKeypadAction; break;}
			case SHORTCUT_TYPE_PROGRAMMING_KEYPAD: {action = pKeypadAction; break;}
			case SHORTCUT_TYPE_ALGEBRA_KEYPAD: {action = xKeypadAction; break;}
			case SHORTCUT_TYPE_CUSTOM_KEYPAD: {action = cKeypadAction; break;}
			case SHORTCUT_TYPE_NEW_VARIABLE: {action = newVariableAction; break;}
			case SHORTCUT_TYPE_NEW_FUNCTION: {action = newFunctionAction; break;}
			case SHORTCUT_TYPE_PLOT: {action = plotAction; break;}
			case SHORTCUT_TYPE_UPDATE_EXRATES: {action = exratesAction; break;}
			case SHORTCUT_TYPE_HISTORY_SEARCH: {action = historyView->findAction; break;}
			case SHORTCUT_TYPE_HELP: {action = helpAction; break;}
			case SHORTCUT_TYPE_QUIT: {action = quitAction; break;}
			case SHORTCUT_TYPE_RPN_UP: {action = rpnUpAction; break;}
			case SHORTCUT_TYPE_RPN_DOWN: {action = rpnDownAction; break;}
			case SHORTCUT_TYPE_RPN_SWAP: {action = rpnSwapAction; break;}
			case SHORTCUT_TYPE_RPN_LASTX: {action = rpnLastxAction; break;}
			case SHORTCUT_TYPE_RPN_COPY: {action = rpnCopyAction; break;}
			case SHORTCUT_TYPE_RPN_DELETE: {action = rpnDeleteAction; break;}
			case SHORTCUT_TYPE_RPN_CLEAR: {action = rpnClearAction; break;}
			default: {}
		}
	}
	if(action) {
		ks->new_action = false;
		QList<QKeySequence> shortcuts = action->shortcuts();
		shortcuts << QKeySequence::fromString(ks->key);
		addAction(action);
		action->setShortcuts(shortcuts);
		if(ks->type[0] == SHORTCUT_TYPE_PLOT) {
			if(plotAction_t) plotAction_t->setToolTip(tr("Plot Functions/Data") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_MANAGE_UNITS) {
			unitsAction_t->setToolTip(tr("Units") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)) + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
		} else if(ks->type[0] == SHORTCUT_TYPE_MANAGE_FUNCTIONS) {
			functionsAction_t->setToolTip(tr("Functions") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)) + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
		} else if(ks->type[0] == SHORTCUT_TYPE_NUMBER_BASES) {
			basesAction->setToolTip(tr("Number Bases") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_UP) {
			rpnUpAction->setToolTip(tr("Rotate the stack or move the selected register up") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_DOWN) {
			rpnDownAction->setToolTip(tr("Rotate the stack or move the selected register down") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_SWAP) {
			rpnSwapAction->setToolTip(tr("Swap the top two values or move the selected value to the top of the stack") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_DELETE) {
			rpnDeleteAction->setToolTip(tr("Delete the top or selected value") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_LASTX) {
			rpnLastxAction->setToolTip(tr("Enter the top value from before the last numeric operation") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_COPY) {
			rpnCopyAction->setToolTip(tr("Copy the selected or top value to the top of the stack") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_RPN_CLEAR) {
			rpnClearAction->setToolTip(tr("Clear the RPN stack") + QString(" (%1)").arg(shortcuts[0].toString(QKeySequence::NativeText)));
		}
	} else {
		if(ks->type[0] == SHORTCUT_TYPE_MODE) {
			modeAction_t->setToolTip(tr("Mode") + QString(" (%1)").arg(QKeySequence::fromString(ks->key).toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_MENU) {
			menuAction_t->setToolTip(tr("Menu") + QString(" (%1)").arg(QKeySequence::fromString(ks->key).toString(QKeySequence::NativeText)));
		} else if(ks->type[0] == SHORTCUT_TYPE_STORE) {
			storeAction_t->setToolTip(tr("Store") + QString("(%1)").arg(QKeySequence::fromString(ks->key).toString(QKeySequence::NativeText)) + "<br><br>" + tr("<i>Right-click/long press</i>: %1").arg(tr("Open menu")));
		}
		if(ks->type.size() == 1 && ks->type[0] == SHORTCUT_TYPE_CONVERT) {
			toAction_t->setToolTip(tr("Convert") + QString(" (%1)").arg(QKeySequence::fromString(ks->key).toString(QKeySequence::NativeText)));
		}
		ks->new_action = true;
		action = new QAction(this);
		action->setShortcut(ks->key);
		action->setData(QVariant::fromValue((void*) ks));
		addAction(action);
		connect(action, SIGNAL(triggered()), this, SLOT(shortcutActivated()));
	}
	ks->action = action;
}
void QalculateWindow::shortcutActivated() {
	keyboard_shortcut *ks = (keyboard_shortcut*) qobject_cast<QAction*>(sender())->data().value<void*>();
	for(size_t i = 0; i < ks->type.size(); i++) {
		triggerShortcut(ks->type[i], ks->value[i]);
	}
}
void QalculateWindow::shortcutClicked(int type, const QString &value) {
	triggerShortcut((shortcut_type) type, value.toStdString());
}
bool contains_prefix(const MathStructure &m) {
	if(m.isUnit() && (m.prefix() || m.unit()->subtype() == SUBTYPE_COMPOSITE_UNIT)) return true;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_prefix(m[i])) return true;
	}
	return false;
}
void QalculateWindow::triggerShortcut(int type, const std::string &value) {
	switch(type) {
		case SHORTCUT_TYPE_NEGATE: {
			negate();
			break;
		}
		case SHORTCUT_TYPE_INVERT: {
			onFunctionClicked(CALCULATOR->getActiveFunction("inv"));
			break;
		}
		case SHORTCUT_TYPE_APPROXIMATE: {
			approximateResult();
			break;
		}
		case SHORTCUT_TYPE_MODE: {
			modeAction_t->showMenu();
			break;
		}
		case SHORTCUT_TYPE_FUNCTIONS_MENU: {
			functionsAction_t->showMenu();
			break;
		}
		case SHORTCUT_TYPE_UNITS_MENU: {
			unitsAction_t->showMenu();
			break;
		}
		case SHORTCUT_TYPE_VARIABLES_MENU: {
			storeAction_t->showMenu();
			break;
		}
		case SHORTCUT_TYPE_MENU: {
			menuAction_t->showMenu();
			break;
		}
		case SHORTCUT_TYPE_FUNCTION: {
			onFunctionClicked(CALCULATOR->getActiveFunction(value));
			break;
		}
		case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
			insertFunction(CALCULATOR->getActiveFunction(value), this);
			break;
		}
		case SHORTCUT_TYPE_VARIABLE: {
			onVariableClicked(CALCULATOR->getActiveVariable(value));
			break;
		}
		case SHORTCUT_TYPE_UNIT: {
			Unit *u = CALCULATOR->getActiveUnit(value);
			if(!u) {
				CALCULATOR->clearMessages();
				CompositeUnit cu("", "", "", value);
				if(CALCULATOR->message()) {
					settings->displayMessages(this);
				} else {
					onUnitClicked(&cu);
				}
			} else {
				onUnitClicked(u);
			}
			break;
		}
		case SHORTCUT_TYPE_OPERATOR: {
			onOperatorClicked(QString::fromStdString(value));
			break;
		}
		case SHORTCUT_TYPE_TEXT: {
			onSymbolClicked(QString::fromStdString(value));
			break;
		}
		case SHORTCUT_TYPE_DATE: {
			expressionEdit->insertDate();
			break;
		}
		case SHORTCUT_TYPE_MATRIX: {
			expressionEdit->insertMatrix();
			break;
		}
		case SHORTCUT_TYPE_SMART_PARENTHESES: {
			expressionEdit->smartParentheses();
			break;
		}
		case SHORTCUT_TYPE_CONVERT_TO: {
			ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
			executeCommand(COMMAND_CONVERT_STRING, true, CALCULATOR->unlocalizeExpression(value, pa));
			break;
		}
		case SHORTCUT_TYPE_CONVERT: {
			onToActivated(false);
			break;
		}
		case SHORTCUT_TYPE_OPTIMAL_UNIT: {
			executeCommand(COMMAND_CONVERT_OPTIMAL);
			break;
		}
		case SHORTCUT_TYPE_BASE_UNITS: {
			executeCommand(COMMAND_CONVERT_BASE);
			break;
		}
		case SHORTCUT_TYPE_OPTIMAL_PREFIX: {
			to_prefix = 0;
			bool b_use_unit_prefixes = settings->printops.use_unit_prefixes;
			bool b_use_prefixes_for_all_units = settings->printops.use_prefixes_for_all_units;
			if((!expressionEdit->expressionHasChanged() || (settings->rpn_mode && CALCULATOR->RPNStackSize() != 0)) && contains_prefix(*mstruct)) {
				mstruct->unformat(settings->evalops);
				executeCommand(COMMAND_CALCULATE, false);
			}
			settings->printops.use_unit_prefixes = true;
			settings->printops.use_prefixes_for_all_units = true;
			setResult(NULL, true, false, true);
			settings->printops.use_unit_prefixes = b_use_unit_prefixes;
			settings->printops.use_prefixes_for_all_units = b_use_prefixes_for_all_units;
			break;
		}
		case SHORTCUT_TYPE_TO_NUMBER_BASE: {
			int save_base = settings->printops.base;
			Number save_nbase = CALCULATOR->customOutputBase();
			to_base = 0;
			to_bits = 0;
			Number nbase;
			base_from_string(value, settings->printops.base, nbase);
			CALCULATOR->setCustomOutputBase(nbase);
			resultFormatUpdated();
			settings->printops.base = save_base;
			CALCULATOR->setCustomOutputBase(save_nbase);
			break;
		}
		case SHORTCUT_TYPE_FACTORIZE: {
			executeCommand(COMMAND_FACTORIZE);
			break;
		}
		case SHORTCUT_TYPE_EXPAND: {
			executeCommand(COMMAND_EXPAND);
			break;
		}
		case SHORTCUT_TYPE_PARTIAL_FRACTIONS: {
			executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
			break;
		}
		case SHORTCUT_TYPE_INPUT_BASE: {
			Number nbase;
			base_from_string(value, settings->evalops.parse_options.base, nbase, true);
			QAction *action = find_child_data(this, "group_inbase", settings->evalops.parse_options.base);
			if(!action) action = customInputBaseAction;
			if(action) action->setChecked(true);
			if(settings->evalops.parse_options.base == BASE_CUSTOM) CALCULATOR->setCustomInputBase(nbase);
			if(action == customInputBaseAction) customInputBaseEdit->setValue(settings->evalops.parse_options.base == BASE_CUSTOM ? CALCULATOR->customInputBase().intValue() : settings->evalops.parse_options.base);
			expressionFormatUpdated(false);
			keypad->updateBase();
			break;
		}
		case SHORTCUT_TYPE_OUTPUT_BASE: {
			Number nbase;
			base_from_string(value, settings->printops.base, nbase, false);
			to_base = 0;
			to_bits = 0;
			QAction *action = find_child_data(this, "group_outbase", settings->printops.base);
			if(!action) action = customOutputBaseAction;
			if(action) action->setChecked(true);
			if(settings->printops.base == BASE_CUSTOM) CALCULATOR->setCustomOutputBase(nbase);
			if(action == customOutputBaseAction) customOutputBaseEdit->setValue(settings->printops.base == BASE_CUSTOM ? CALCULATOR->customOutputBase().intValue() : settings->printops.base);
			resultFormatUpdated();
			keypad->updateBase();
			break;
		}
		case SHORTCUT_TYPE_HISTORY_SEARCH: {
			historyView->findAction->trigger();
			break;
		}
		case SHORTCUT_TYPE_HISTORY_CLEAR: {
			historyView->editClear();
			expressionEdit->clearHistory();
			break;
		}
		case SHORTCUT_TYPE_MEMORY_CLEAR: {
			onMCClicked();
			break;
		}
		case SHORTCUT_TYPE_MEMORY_RECALL: {
			onMRClicked();
			break;
		}
		case SHORTCUT_TYPE_MEMORY_STORE: {
			onMSClicked();
			break;
		}
		case SHORTCUT_TYPE_MEMORY_ADD: {
			onMPlusClicked();
			break;
		}
		case SHORTCUT_TYPE_MEMORY_SUBTRACT: {
			onMMinusClicked();
			break;
		}
		case SHORTCUT_TYPE_COPY_RESULT: {
			if(settings->v_result.empty()) break;
			int v = s2i(value);
			if(v < 0 || v > 8) v = 0;
			bool b_ascii = (v != 1 && v != 4 && v != 6 && (v != 0 || settings->copy_ascii));
			bool b_nounits = (v == 3 || v == 8 || (v == 0 && settings->copy_ascii && settings->copy_ascii_without_units));
			QString str_ascii, str_html;
			if(v > 3) {
				if(settings->history_expression_type == 1 && !settings->v_expression[settings->v_expression.size() - 1].empty()) {
					if(b_ascii) {
						str_ascii += QString::fromStdString(unformat(settings->v_expression[settings->v_expression.size() - 1]));
					} else {
						str_ascii += QString::fromStdString(settings->v_expression[settings->v_expression.size() - 1]);
						if(v < 6) b_ascii = true;
						else str_html += QString::fromStdString(settings->v_expression[settings->v_expression.size() - 1]);
					}
				} else {
					if(b_ascii) {
						str_ascii += QString::fromStdString(unformat(unhtmlize(settings->v_parse[settings->v_parse.size() - 1], true)));
					} else {
						str_html += QString::fromStdString(uncolorize(settings->v_parse[settings->v_parse.size() - 1]));
						str_ascii += QString::fromStdString(unhtmlize(settings->v_parse[settings->v_parse.size() - 1]));
					}
				}
			}
			if(v <= 3 || v >= 6) {
				if(b_ascii) {
					if(v > 3) str_ascii += " = ";
					std::string str = settings->v_result[settings->v_result.size() - 1][0];
					if(b_nounits) {
						size_t i = str.find("<span style=\"color:#008000\">");
						if(i == std::string::npos) i = str.find("<span style=\"color:#BBFFBB\">");
						if(i != std::string::npos && str.find("</span>", i) == str.length() - 7) {
							str = str.substr(0, i);
						}
					}
					str_ascii += QString::fromStdString(unformat(unhtmlize(str, true))).trimmed();
				} else {
					if(v > 3) {
						if(settings->v_exact[settings->v_exact.size() - 1][0]) {
							str_html += " = ";
							str_ascii += " = ";
						} else {
							str_html += " " SIGN_ALMOST_EQUAL " ";
							str_ascii += " " SIGN_ALMOST_EQUAL " ";
						}
					}
					str_html += QString::fromStdString(uncolorize(settings->v_result[settings->v_result.size() - 1][0]));
					str_ascii += QString::fromStdString(unhtmlize(settings->v_result[settings->v_result.size() - 1][0]));
				}
			}
			if(b_ascii) {
				QApplication::clipboard()->setText(str_ascii);
			} else {
				QMimeData *qm = new QMimeData();
				qm->setHtml(str_html);
				qm->setText(str_ascii);
				if(v <= 3 || v >= 6) qm->setObjectName("history_result");
				else qm->setObjectName("history_expression");
				QApplication::clipboard()->setMimeData(qm);
			}
			break;
		}
		case SHORTCUT_TYPE_INSERT_RESULT: {
			if(!settings->v_result.empty()) {
				expressionEdit->blockCompletion();
				expressionEdit->insertPlainText(QString::fromStdString(unhtmlize(settings->v_result[settings->v_result.size() - 1][0])));
				if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
				expressionEdit->blockCompletion(false);
			}
			break;
		}
		case SHORTCUT_TYPE_ALWAYS_ON_TOP: {
			settings->always_on_top = !settings->always_on_top;
			onAlwaysOnTopChanged();
			break;
		}
		case SHORTCUT_TYPE_COMPLETE: {
			expressionEdit->completeOrActivateFirst();
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_CLEAR: {
			expressionEdit->clear();
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_DELETE: {
			expressionEdit->textCursor().deleteChar();
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_BACKSPACE: {
			expressionEdit->textCursor().deletePreviousChar();
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_START: {
			expressionEdit->moveCursor(QTextCursor::Start);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_END: {
			expressionEdit->moveCursor(QTextCursor::End);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_RIGHT: {
			expressionEdit->moveCursor(QTextCursor::NextCharacter);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_LEFT: {
			expressionEdit->moveCursor(QTextCursor::PreviousCharacter);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_UP: {
			expressionEdit->moveCursor(QTextCursor::PreviousRow);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_DOWN: {
			expressionEdit->moveCursor(QTextCursor::NextRow);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_UNDO: {
			expressionEdit->editUndo();
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_REDO: {
			expressionEdit->editRedo();
			break;
		}
		case SHORTCUT_TYPE_CALCULATE_EXPRESSION: {
			calculate();
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_HISTORY_NEXT: {
			QKeyEvent e(QEvent::KeyPress, Qt::Key_PageDown, Qt::NoModifier);
			expressionEdit->keyPressEvent(&e);
			break;
		}
		case SHORTCUT_TYPE_EXPRESSION_HISTORY_PREVIOUS: {
			QKeyEvent e(QEvent::KeyPress, Qt::Key_PageUp, Qt::NoModifier);
			expressionEdit->keyPressEvent(&e);
			break;
		}
		case SHORTCUT_TYPE_PRECISION: {
			int v = s2i(value);
			if(settings->previous_precision > 0 && CALCULATOR->getPrecision() == v) {
				v = settings->previous_precision;
				settings->previous_precision = 0;
			} else {
				settings->previous_precision = CALCULATOR->getPrecision();
			}
			QSpinBox *w = findChild<QSpinBox*>("spinbox_precision");
			if(w) {
				w->blockSignals(true);
				w->setValue(v);
				w->blockSignals(false);
			}
			CALCULATOR->setPrecision(v);
			expressionCalculationUpdated();
			break;
		}
		case SHORTCUT_TYPE_MAX_DECIMALS: {
			int v = s2i(value);
			QSpinBox *w = findChild<QSpinBox*>("spinbox_maxdecimals");
			if(w) w->blockSignals(true);
			if(v < 0 || (settings->printops.use_max_decimals && settings->printops.max_decimals == v)) {
				if(w) w->setValue(-1);
				settings->printops.use_max_decimals = false;
				settings->printops.max_decimals = -1;
			} else {
				if(w) w->setValue(v);
				settings->printops.use_max_decimals = true;
				settings->printops.max_decimals = v;
			}
			if(w) w->blockSignals(false);
			resultFormatUpdated(true);
			break;
		}
		case SHORTCUT_TYPE_MIN_DECIMALS: {
			int v = s2i(value);
			QSpinBox *w = findChild<QSpinBox*>("spinbox_mindecimals");
			if(w) w->blockSignals(true);
			if(v < 0 || (settings->printops.use_min_decimals && settings->printops.min_decimals == v)) {
				if(w) w->setValue(0);
				settings->printops.use_min_decimals = false;
				settings->printops.min_decimals = 0;
			} else {
				if(w) w->setValue(v);
				settings->printops.use_min_decimals = true;
				settings->printops.min_decimals = v;
			}
			if(w) w->blockSignals(false);
			resultFormatUpdated(true);
			break;
		}
		case SHORTCUT_TYPE_MINMAX_DECIMALS: {
			int v = s2i(value);
			QSpinBox *w1 = findChild<QSpinBox*>("spinbox_mindecimals");
			QSpinBox *w2 = findChild<QSpinBox*>("spinbox_maxdecimals");
			if(w1) w1->blockSignals(true);
			if(w2) w2->blockSignals(true);
			if(v < 0 || (settings->printops.use_min_decimals && settings->printops.min_decimals == v && settings->printops.use_max_decimals && settings->printops.max_decimals == v)) {
				if(w1) w1->setValue(0);
				if(w2) w2->setValue(-1);
				settings->printops.use_min_decimals = false;
				settings->printops.min_decimals = 0;
				settings->printops.use_max_decimals = false;
				settings->printops.max_decimals = -1;
			} else {
				if(w1) w1->setValue(v);
				if(w2) w2->setValue(v);
				settings->printops.use_min_decimals = true;
				settings->printops.min_decimals = v;
				settings->printops.use_max_decimals = true;
				settings->printops.max_decimals = v;
			}
			if(w1) w1->blockSignals(false);
			if(w2) w2->blockSignals(false);
			resultFormatUpdated(true);
			break;
		}
		case SHORTCUT_TYPE_MANAGE_FUNCTIONS: {functionsAction->trigger(); break;}
		case SHORTCUT_TYPE_MANAGE_VARIABLES: {variablesAction->trigger(); break;}
		case SHORTCUT_TYPE_MANAGE_UNITS: {unitsAction->trigger(); break;}
		case SHORTCUT_TYPE_MANAGE_DATA_SETS: {datasetsAction->trigger(); break;}
		case SHORTCUT_TYPE_FLOATING_POINT: {fpAction->trigger(); break;}
		case SHORTCUT_TYPE_CALENDARS: {calendarsAction->trigger(); break;}
		case SHORTCUT_TYPE_PERIODIC_TABLE: {periodicTableAction->trigger(); break;}
		case SHORTCUT_TYPE_PERCENTAGE_TOOL: {percentageAction->trigger(); break;}
		case SHORTCUT_TYPE_NUMBER_BASES: {basesAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_MODE: {rpnAction->trigger(); break;}
		case SHORTCUT_TYPE_DEGREES: {degAction->trigger(); break;}
		case SHORTCUT_TYPE_RADIANS: {radAction->trigger(); break;}
		case SHORTCUT_TYPE_GRADIANS: {graAction->trigger(); break;}
		case SHORTCUT_TYPE_NORMAL_NOTATION: {normalAction->trigger(); break;}
		case SHORTCUT_TYPE_SCIENTIFIC_NOTATION: {sciAction->trigger(); break;}
		case SHORTCUT_TYPE_ENGINEERING_NOTATION: {engAction->trigger(); break;}
		case SHORTCUT_TYPE_SIMPLE_NOTATION: {simpleAction->trigger(); break;}
		case SHORTCUT_TYPE_CHAIN_MODE: {chainAction->trigger(); break;}
		case SHORTCUT_TYPE_KEYPAD: {keypadAction->trigger(); break;}
		case SHORTCUT_TYPE_GENERAL_KEYPAD: {gKeypadAction->trigger(); break;}
		case SHORTCUT_TYPE_PROGRAMMING_KEYPAD: {pKeypadAction->trigger(); break;}
		case SHORTCUT_TYPE_ALGEBRA_KEYPAD: {xKeypadAction->trigger(); break;}
		case SHORTCUT_TYPE_CUSTOM_KEYPAD: {cKeypadAction->trigger(); break;}
		case SHORTCUT_TYPE_STORE: {onStoreActivated(); break;}
		case SHORTCUT_TYPE_NEW_VARIABLE: {newVariableAction->trigger(); break;}
		case SHORTCUT_TYPE_NEW_FUNCTION: {newFunctionAction->trigger(); break;}
		case SHORTCUT_TYPE_PLOT: {plotAction->trigger(); break;}
		case SHORTCUT_TYPE_UPDATE_EXRATES: {exratesAction->trigger(); break;}
		case SHORTCUT_TYPE_HELP: {helpAction->trigger(); break;}
		case SHORTCUT_TYPE_QUIT: {quitAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_UP: {rpnUpAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_DOWN: {rpnDownAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_SWAP: {rpnSwapAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_LASTX: {rpnLastxAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_COPY: {rpnCopyAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_DELETE: {rpnDeleteAction->trigger(); break;}
		case SHORTCUT_TYPE_RPN_CLEAR: {rpnClearAction->trigger(); break;}
	}
}
bool sort_compare_item(ExpressionItem *o1, ExpressionItem *o2) {
	return o1->title(true, settings->printops.use_unicode_signs) < o2->title(true, settings->printops.use_unicode_signs);
}
void QalculateWindow::updateAngleUnitsMenu() {
	QActionGroup *group = findChild<QActionGroup*>("group_angleunit");
	if(!group) return;
	QMenu *menu = angleMenu;
	QAction *action = NULL;
	menu->clear();
	Unit *u_rad = CALCULATOR->getRadUnit();
	for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
		if(CALCULATOR->units[i]->baseUnit() == u_rad) {
			Unit *u = CALCULATOR->units[i];
			if(u != u_rad && !u->isHidden() && u->isActive() && u->baseExponent() == 1 && !u->hasName("gra") && !u->hasName("deg")) {
				action = menu->addAction(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) menu)), this, SLOT(angleUnitActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_angle_unit_" + QString::fromStdString(u->referenceName())); action->setData(ANGLE_UNIT_CUSTOM); if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && CALCULATOR->customAngleUnit() == u) action->setChecked(true);
			}
		}
	}
	action = menu->addAction(tr("None"), this, SLOT(angleUnitActivated())); action->setCheckable(true); group->addAction(action); action->setObjectName("action_angle_unit_none"); action->setData(ANGLE_UNIT_NONE); if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_NONE) action->setChecked(true);
	action = menu->addAction(tr("Other")); action->setCheckable(true); group->addAction(action); action->setObjectName("action_angle_unit_other"); action->setVisible(false); action->setData(-1); if(!group->checkedAction()) action->setChecked(true);
}

struct tree_struct_m {
	std::string item;
	std::list<tree_struct_m> items;
	std::list<tree_struct_m>::iterator it;
	std::list<tree_struct_m>::reverse_iterator rit;
	std::vector<void*> objects;
	tree_struct_m *parent;
	void sort() {
		items.sort();
		for(std::list<tree_struct_m>::iterator it = items.begin(); it != items.end(); ++it) {
			it->sort();
		}
	}
	bool operator < (const tree_struct_m &s1) const {
		return string_is_less(item, s1.item);
	}
};

void QalculateWindow::updateFunctionsMenu() {
	functionsMenu->clear();
	functionsMenu->addAction(tr("New Function…"), this, SLOT(newFunction()));
	functionsMenu->addSeparator();
	for(size_t i = 0; i < settings->favourite_functions.size();) {
		if(!CALCULATOR->stillHasFunction(settings->favourite_functions[i]) || !settings->favourite_functions[i]->isActive()) {
			settings->favourite_functions.erase(settings->favourite_functions.begin() + i);
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < settings->recent_functions.size();) {
		if(!CALCULATOR->stillHasFunction(settings->recent_functions[i]) || !settings->recent_functions[i]->isActive()) {
			settings->recent_functions.erase(settings->recent_functions.begin() + i);
		} else {
			i++;
		}
	}
	favouriteFunctionActions.clear();
	recentFunctionActions.clear();
	if(settings->show_all_functions) {

		tree_struct_m function_cats;
		std::vector<MathFunction*> user_functions;

		{size_t cat_i, cat_i_prev;
		bool b;
		std::string str, cat, cat_sub;
		MathFunction *f = NULL;
		function_cats.items.clear();
		function_cats.objects.clear();
		function_cats.parent = NULL;
		user_functions.clear();
		std::list<tree_struct_m>::iterator it;
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			if(CALCULATOR->functions[i]->isActive() && !CALCULATOR->functions[i]->isHidden()) {
				if(CALCULATOR->functions[i]->isLocal() && !CALCULATOR->functions[i]->isBuiltin() && !CALCULATOR->functions[i]->isHidden()) {
					b = false;
					for(size_t i3 = 0; i3 < user_functions.size(); i3++) {
						f = user_functions[i3];
						if(string_is_less(CALCULATOR->functions[i]->title(true, settings->printops.use_unicode_signs), f->title(true, settings->printops.use_unicode_signs))) {
							b = true;
							user_functions.insert(user_functions.begin() + i3, CALCULATOR->functions[i]);
							break;
						}
					}
					if(!b) user_functions.push_back(CALCULATOR->functions[i]);
				}
				tree_struct_m *item = &function_cats;
				if(!CALCULATOR->functions[i]->category().empty()) {
					cat = CALCULATOR->functions[i]->category();
					cat_i = cat.find("/"); cat_i_prev = 0;
					b = false;
					while(true) {
						if(cat_i == std::string::npos) {
							cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
						} else {
							cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
						}
						b = false;
						for(it = item->items.begin(); it != item->items.end(); ++it) {
							if(cat_sub == it->item) {
								item = &*it;
								b = true;
								break;
							}
						}
						if(!b) {
							tree_struct_m cat;
							item->items.push_back(cat);
							it = item->items.end();
							--it;
							it->parent = item;
							item = &*it;
							item->item = cat_sub;
						}
						if(cat_i == std::string::npos) {
							break;
						}
						cat_i_prev = cat_i + 1;
						cat_i = cat.find("/", cat_i_prev);
					}
				}
				b = false;
				for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
					f = (MathFunction*) item->objects[i3];
					if(string_is_less(CALCULATOR->functions[i]->title(true, settings->printops.use_unicode_signs), f->title(true, settings->printops.use_unicode_signs))) {
						b = true;
						item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->functions[i]);
						break;
					}
				}
				if(!b) item->objects.push_back((void*) CALCULATOR->functions[i]);
			}
		}

		function_cats.sort();}

		favouriteFunctionsMenu = functionsMenu->addMenu(tr("Favorites"));
		recentFunctionsMenu = functionsMenu->addMenu(tr("Recent"));
		int first_index = 5;
		QMenu *sub = functionsMenu, *sub2, *sub3;
		QAction *action = NULL;
		sub2 = sub;
		MathFunction *f;
		tree_struct_m *titem, *titem2;
		function_cats.rit = function_cats.items.rbegin();
		if(function_cats.rit != function_cats.items.rend()) {
			titem = &*function_cats.rit;
			++function_cats.rit;
			titem->rit = titem->items.rbegin();
		} else {
			titem = NULL;
		}
		std::stack<QMenu*> menus;
		menus.push(sub);
		sub3 = sub;
		if(!user_functions.empty()) {
			sub = functionsMenu->addMenu(tr("User functions"));
			for(size_t i = 0; i < user_functions.size(); i++) {
				action = sub->addAction(QString::fromStdString(user_functions[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(functionActivated()));
				action->setData(QVariant::fromValue((void*) user_functions[i]));
			}
			first_index++;
		}
		functionsMenu->addSeparator();
		while(titem) {
			if(!titem->items.empty() || !titem->objects.empty()) {
				gsub("&", "&&", titem->item);
				sub = new QMenu(QString::fromStdString(titem->item));
				sub3->insertMenu(sub3->actions().value(sub3 == functionsMenu ? first_index : 0), sub);
				menus.push(sub);
				sub3 = sub;
				for(size_t i = 0; i < titem->objects.size(); i++) {
					f = (MathFunction*) titem->objects[i];
					action = sub->addAction(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(functionActivated()));
					action->setData(QVariant::fromValue((void*) f));
				}
			} else {
				titem = titem->parent;
			}
			while(titem && titem->rit == titem->items.rend()) {
				titem = titem->parent;
				menus.pop();
				if(menus.size() > 0) sub3 = menus.top();
			}
			if(titem) {
				titem2 = &*titem->rit;
				++titem->rit;
				titem = titem2;
				titem->rit = titem->items.rbegin();
			}
		}
		sub = sub2;
		for(size_t i = 0; i < function_cats.objects.size(); i++) {
			f = (MathFunction*) function_cats.objects[i];
			if(!f->isLocal()) {
				action = sub->addAction(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(functionActivated()));
				action->setData(QVariant::fromValue((void*) f));
			}
		}
		functionsMenu->addSeparator();
	} else {
		favouriteFunctionsMenu = functionsMenu;
		recentFunctionsMenu = functionsMenu;
	}
	QAction *action = functionsMenu->addAction(tr("Open dialog"));
	action->setCheckable(true);
	action->setChecked(settings->use_function_dialog);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(useFunctionDialog(bool)));
	firstFunctionsMenuOptionAction = action;
	action = functionsMenu->addAction(tr("Show all functions"));
	action->setCheckable(true);
	action->setChecked(settings->show_all_functions);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(showAllFunctions(bool)));
	updateRecentFunctions();
	updateFavouriteFunctions();
}
void QalculateWindow::useFunctionDialog(bool b) {
	settings->use_function_dialog = b;
}
void QalculateWindow::showAllFunctions(bool b) {
	settings->show_all_functions = b;
	updateFunctionsMenu();
}
void QalculateWindow::updateFavouriteFunctions() {
	if(functionsMenu->isEmpty()) return;
	for(int i = 0; i < favouriteFunctionActions.size(); i++) {
		favouriteFunctionsMenu->removeAction(favouriteFunctionActions[i]);
		favouriteFunctionActions[i]->deleteLater();
	}
	bool b_empty = settings->show_all_functions && favouriteFunctionActions.isEmpty();
	favouriteFunctionActions.clear();
	bool update_recent = false;
	if(!settings->favourite_functions.empty()) {
		std::sort(settings->favourite_functions.begin(), settings->favourite_functions.end(), sort_compare_item);
		for(size_t i = 0; i < settings->favourite_functions.size(); i++) {
			MathFunction *f = settings->favourite_functions[i];
			if(!settings->show_all_functions) {
				for(size_t i2 = 0; i2 < settings->recent_functions.size(); i2++) {
					if(settings->recent_functions[i2] == f) {
						settings->recent_functions.erase(settings->recent_functions.begin() + i2);
						update_recent = true;
						break;
					}
				}
			}
			QAction *action = new QAction(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), favouriteFunctionsMenu);
			connect(action, SIGNAL(triggered()), this, SLOT(functionActivated()));
			action->setData(QVariant::fromValue((void*) f));
			favouriteFunctionActions << action;
		}
		if(!settings->show_all_functions) {
			QAction *action = new QAction(favouriteFunctionsMenu);
			action->setSeparator(true);
			favouriteFunctionActions << action;
		}
		if(update_recent) updateRecentFunctions();
		favouriteFunctionsMenu->insertActions(settings->show_all_functions ? NULL : (recentFunctionActions.isEmpty() ?firstFunctionsMenuOptionAction : recentFunctionActions.at(0)), favouriteFunctionActions);
		if(b_empty) {
			favouriteFunctionsMenu->hide();
			favouriteFunctionsMenu->show();
			favouriteFunctionsMenu->hide();
		}
	}
}
void QalculateWindow::updateRecentFunctions() {
	if(functionsMenu->isEmpty()) return;
	for(int i = 0; i < recentFunctionActions.size(); i++) {
		recentFunctionsMenu->removeAction(recentFunctionActions[i]);
		recentFunctionActions[i]->deleteLater();
	}
	bool b_empty = settings->show_all_functions && recentFunctionActions.isEmpty();
	recentFunctionActions.clear();
	if(!settings->recent_functions.empty()) {
		for(size_t i = 0; i < settings->recent_functions.size() && (i < 5 || settings->show_all_functions); i++) {
			QAction *action = new QAction(QString::fromStdString(settings->recent_functions[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), recentFunctionsMenu);
			connect(action, SIGNAL(triggered()), this, SLOT(functionActivated()));
			action->setData(QVariant::fromValue((void*) settings->recent_functions[i]));
			recentFunctionActions << action;
		}
		if(!settings->show_all_functions) {
			QAction *action = new QAction(recentFunctionsMenu);
			action->setSeparator(true);
			recentFunctionActions << action;
		}
		recentFunctionsMenu->insertActions(settings->show_all_functions ? NULL : firstFunctionsMenuOptionAction, recentFunctionActions);
		if(b_empty) {
			recentFunctionsMenu->hide();
			recentFunctionsMenu->show();
			recentFunctionsMenu->hide();
		}
	}
}
void QalculateWindow::addToRecentFunctions(MathFunction *f) {
	if(!settings->show_all_functions) {
		for(size_t i = 0; i < settings->favourite_functions.size(); i++) {
			if(settings->favourite_functions[i] == f) return;
		}
	}
	for(size_t i = 0; i < settings->recent_functions.size(); i++) {
		if(settings->recent_functions[i] == f) {
			if(i == 0) return;
			settings->recent_functions.erase(settings->recent_functions.begin() + i);
			break;
		}
	}
	if(settings->recent_functions.size() > 10) settings->recent_functions.pop_back();
	settings->recent_functions.insert(settings->recent_functions.begin(), f);
	updateRecentFunctions();
}

void QalculateWindow::updateUnitsMenu() {
	unitsMenu->clear();
	for(size_t i = 0; i < settings->favourite_units.size();) {
		if(!CALCULATOR->stillHasUnit(settings->favourite_units[i]) || !settings->favourite_units[i]->isActive()) {
			settings->favourite_units.erase(settings->favourite_units.begin() + i);
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < settings->recent_units.size();) {
		if(!CALCULATOR->stillHasUnit(settings->recent_units[i]) || !settings->recent_units[i]->isActive()) {
			settings->recent_units.erase(settings->recent_units.begin() + i);
		} else {
			i++;
		}
	}
	favouriteUnitActions.clear();
	recentUnitActions.clear();
	if(settings->show_all_units) {

		tree_struct_m unit_cats;
		std::vector<Unit*> user_units;

		{size_t cat_i, cat_i_prev;
		bool b;
		std::string str, cat, cat_sub;
		Unit *u = NULL;
		unit_cats.items.clear();
		unit_cats.objects.clear();
		unit_cats.parent = NULL;
		user_units.clear();
		std::list<tree_struct_m>::iterator it;
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i]->isActive() && (!CALCULATOR->units[i]->isHidden() || CALCULATOR->units[i]->isCurrency())) {
				if(CALCULATOR->units[i]->isLocal() && !CALCULATOR->units[i]->isBuiltin()) {
					b = false;
					for(size_t i3 = 0; i3 < user_units.size(); i3++) {
						u = user_units[i3];
						if(string_is_less(CALCULATOR->units[i]->title(true, settings->printops.use_unicode_signs), u->title(true, settings->printops.use_unicode_signs))) {
							b = true;
							user_units.insert(user_units.begin() + i3, CALCULATOR->units[i]);
							break;
						}
					}
					if(!b) user_units.push_back(CALCULATOR->units[i]);
				}
				tree_struct_m *item = &unit_cats;
				if(!CALCULATOR->units[i]->category().empty()) {
					cat = CALCULATOR->units[i]->category();
					cat_i = cat.find("/"); cat_i_prev = 0;
					b = false;
					while(true) {
						if(cat_i == std::string::npos) {
							cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
						} else {
							cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
						}
						b = false;
						for(it = item->items.begin(); it != item->items.end(); ++it) {
							if(cat_sub == it->item) {
								item = &*it;
								b = true;
								break;
							}
						}
						if(!b) {
							tree_struct_m cat;
							item->items.push_back(cat);
							it = item->items.end();
							--it;
							it->parent = item;
							item = &*it;
							item->item = cat_sub;
						}
						if(cat_i == std::string::npos) {
							break;
						}
						cat_i_prev = cat_i + 1;
						cat_i = cat.find("/", cat_i_prev);
					}
				}
				b = false;
				for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
					u = (Unit*) item->objects[i3];
					if(string_is_less(CALCULATOR->units[i]->title(true, settings->printops.use_unicode_signs), u->title(true, settings->printops.use_unicode_signs))) {
						b = true;
						item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->units[i]);
						break;
					}
				}
				if(!b) item->objects.push_back((void*) CALCULATOR->units[i]);
			}
		}

		unit_cats.sort();}

		favouriteUnitsMenu = unitsMenu->addMenu(tr("Favorites"));
		recentUnitsMenu = unitsMenu->addMenu(tr("Recent"));
		int first_index = 3;
		QMenu *sub = unitsMenu, *sub2, *sub3;
		QAction *action = NULL;
		sub2 = sub;
		Unit *u;
		tree_struct_m *titem, *titem2;
		unit_cats.rit = unit_cats.items.rbegin();
		if(unit_cats.rit != unit_cats.items.rend()) {
			titem = &*unit_cats.rit;
			++unit_cats.rit;
			titem->rit = titem->items.rbegin();
		} else {
			titem = NULL;
		}
		std::stack<QMenu*> menus;
		menus.push(sub);
		sub3 = sub;
		if(!user_units.empty()) {
			sub = unitsMenu->addMenu(tr("User units"));
			for(size_t i = 0; i < user_units.size(); i++) {
				action = sub->addAction(QString::fromStdString(user_units[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(unitActivated()));
				action->setData(QVariant::fromValue((void*) user_units[i]));
			}
			first_index++;
		}
		unitsMenu->addSeparator();
		while(titem) {
			if(!titem->items.empty() || !titem->objects.empty()) {
				gsub("&", "&&", titem->item);
				sub = new QMenu(QString::fromStdString(titem->item));
				sub3->insertMenu(sub3->actions().value(sub3 == unitsMenu ? first_index : 0), sub);
				menus.push(sub);
				sub3 = sub;
				bool is_currencies = false;
				for(size_t i = 0; i < titem->objects.size(); i++) {
					u = (Unit*) titem->objects[i];
					if(!is_currencies && u->isCurrency()) is_currencies = true;
					if(!u->isHidden()) {
						action = sub->addAction(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(unitActivated()));
						action->setData(QVariant::fromValue((void*) u));
					}
				}
				if(is_currencies) {
					sub = sub3->addMenu(tr("more"));
					connect(sub3, SIGNAL(aboutToShow()), this, SLOT(addCurrencyFlagsToMenu()));
					connect(sub, SIGNAL(aboutToShow()), this, SLOT(addCurrencyFlagsToMenu()));
					for(size_t i = 0; i < titem->objects.size(); i++) {
						u = (Unit*) titem->objects[i];
						if(u->isHidden()) {
							action = sub->addAction(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(unitActivated()));
							action->setData(QVariant::fromValue((void*) u));
						}
					}
				}
			} else {
				titem = titem->parent;
			}
			while(titem && titem->rit == titem->items.rend()) {
				titem = titem->parent;
				menus.pop();
				if(menus.size() > 0) sub3 = menus.top();
			}
			if(titem) {
				titem2 = &*titem->rit;
				++titem->rit;
				titem = titem2;
				titem->rit = titem->items.rbegin();
			}
		}
		sub = sub2;
		for(size_t i = 0; i < unit_cats.objects.size(); i++) {
			u = (Unit*) unit_cats.objects[i];
			if(!u->isLocal()) {
				action = sub->addAction(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(unitActivated()));
				action->setData(QVariant::fromValue((void*) u));
			}
		}
		unitsMenu->addSeparator();
		sub = unitsMenu->addMenu(tr("Prefixes"));
		int index = 0;
		Prefix *p = CALCULATOR->getPrefix(index);
		while(p) {
			QString str = QString::fromStdString(p->preferredDisplayName(false, true, false, false, &can_display_unicode_string_function, (void*) this).name);
			if(p->type() == PREFIX_DECIMAL) {
				str += " (10^";
				str += QString::number(((DecimalPrefix*) p)->exponent());
				str += ")";
			} else if(p->type() == PREFIX_BINARY) {
				str += " (2^";
				str += QString::number(((DecimalPrefix*) p)->exponent());
				str += ")";
			}
			action = sub->addAction(str, this, SLOT(prefixActivated()));
			action->setData(QVariant::fromValue((void*) p));
			index++;
			p = CALCULATOR->getPrefix(index);
		}
		unitsMenu->addSeparator();
	} else {
		favouriteUnitsMenu = unitsMenu;
		recentUnitsMenu = unitsMenu;
	}
	QAction *action = unitsMenu->addAction(tr("Show all units"));
	action->setCheckable(true);
	action->setChecked(settings->show_all_units);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(showAllUnits(bool)));
	firstUnitsMenuOptionAction = action;
	updateRecentUnits();
	updateFavouriteUnits();
	updateAngleUnitsMenu();
}
void QalculateWindow::addCurrencyFlagsToMenu() {
	QMenu *menu = qobject_cast<QMenu*>(sender());
	QList<QAction*> actions = menu->actions();
	for(int i = 0; i < actions.count(); i++) {
		QAction *action = actions.at(i);
		Unit *u = (Unit*) action->data().value<void*>();
		if(u && u->isCurrency() && QFile::exists(":/data/flags/" + QString::fromStdString(u->referenceName() + ".png"))) {
			action->setIcon(QIcon(":/data/flags/" + QString::fromStdString(u->referenceName() + ".png")));
		}
	}
	disconnect(menu, SIGNAL(aboutToShow()), this, SLOT(addCurrencyFlagsToMenu()));
}
void QalculateWindow::showAllUnits(bool b) {
	settings->show_all_units = b;
	updateUnitsMenu();
}
void QalculateWindow::updateFavouriteUnits() {
	if(unitsMenu->isEmpty()) return;
	for(int i = 0; i < favouriteUnitActions.size(); i++) {
		favouriteUnitsMenu->removeAction(favouriteUnitActions[i]);
		favouriteUnitActions[i]->deleteLater();
	}
	bool b_empty = settings->show_all_units && favouriteUnitActions.isEmpty();
	favouriteUnitActions.clear();
	bool update_recent = false;
	if(!settings->favourite_units.empty()) {
		std::sort(settings->favourite_units.begin(), settings->favourite_units.end(), sort_compare_item);
		for(size_t i = 0; i < settings->favourite_units.size(); i++) {
			Unit *u = settings->favourite_units[i];
			if(!settings->show_all_units) {
				for(size_t i2 = 0; i2 < settings->recent_units.size(); i2++) {
					if(settings->recent_units[i2] == u) {
						settings->recent_units.erase(settings->recent_units.begin() + i2);
						update_recent = true;
						break;
					}
				}
			}
			QAction *action = new QAction(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), favouriteUnitsMenu);
			connect(action, SIGNAL(triggered()), this, SLOT(unitActivated()));
			if(u->isCurrency() && QFile::exists(":/data/flags/" + QString::fromStdString(u->referenceName() + ".png"))) {
				action->setIcon(QIcon(":/data/flags/" + QString::fromStdString(u->referenceName() + ".png")));
			}
			action->setData(QVariant::fromValue((void*) u));
			favouriteUnitActions << action;
		}
		if(!settings->show_all_units) {
			QAction *action = new QAction(favouriteUnitsMenu);
			action->setSeparator(true);
			favouriteUnitActions << action;
		}
		if(update_recent) updateRecentUnits();
		favouriteUnitsMenu->insertActions(settings->show_all_units ? NULL : (recentUnitActions.isEmpty() ? firstUnitsMenuOptionAction : recentUnitActions.at(0)), favouriteUnitActions);
		if(b_empty) {
			favouriteUnitsMenu->hide();
			favouriteUnitsMenu->show();
			favouriteUnitsMenu->hide();
		}
	}
}
void QalculateWindow::updateRecentUnits() {
	if(unitsMenu->isEmpty()) return;
	for(int i = 0; i < recentUnitActions.size(); i++) {
		recentUnitsMenu->removeAction(recentUnitActions[i]);
		recentUnitActions[i]->deleteLater();
	}
	bool b_empty = settings->show_all_units && recentUnitActions.isEmpty();
	recentUnitActions.clear();
	if(!settings->recent_units.empty()) {
		for(size_t i = 0; i < settings->recent_units.size() && (i < 5 || settings->show_all_units); i++) {
			Unit *u = settings->recent_units[i];
			QAction *action = new QAction(QString::fromStdString(u->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), recentUnitsMenu);
			connect(action, SIGNAL(triggered()), this, SLOT(unitActivated()));
			if(u->isCurrency() && QFile::exists(":/data/flags/" + QString::fromStdString(u->referenceName() + ".png"))) {
				action->setIcon(QIcon(":/data/flags/" + QString::fromStdString(u->referenceName() + ".png")));
			}
			action->setData(QVariant::fromValue((void*) u));
			recentUnitActions << action;
		}
		if(!settings->show_all_units) {
			QAction *action = new QAction(recentUnitsMenu);
			action->setSeparator(true);
			recentUnitActions << action;
		}
		recentUnitsMenu->insertActions(settings->show_all_units ? NULL : firstUnitsMenuOptionAction, recentUnitActions);
		if(b_empty) {
			recentUnitsMenu->hide();
			recentUnitsMenu->show();
			recentUnitsMenu->hide();
		}
	}
}
void QalculateWindow::addToRecentUnits(Unit *u) {
	if(!settings->show_all_units) {
		for(size_t i = 0; i < settings->favourite_units.size(); i++) {
			if(settings->favourite_units[i] == u) return;
		}
	}
	for(size_t i = 0; i < settings->recent_units.size(); i++) {
		if(settings->recent_units[i] == u) {
			if(i == 0) return;
			settings->recent_units.erase(settings->recent_units.begin() + i);
			break;
		}
	}
	if(settings->recent_units.size() > 10) settings->recent_units.pop_back();
	settings->recent_units.insert(settings->recent_units.begin(), u);
	updateRecentUnits();
}
void QalculateWindow::updateVariablesMenu() {
	variablesMenu->clear();
	variablesMenu->addAction(variablesAction);
	variablesMenu->addSeparator();
	for(size_t i = 0; i < settings->favourite_variables.size();) {
		if(!CALCULATOR->stillHasVariable(settings->favourite_variables[i]) || !settings->favourite_variables[i]->isActive()) {
			settings->favourite_variables.erase(settings->favourite_variables.begin() + i);
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < settings->recent_variables.size();) {
		if(!CALCULATOR->stillHasVariable(settings->recent_variables[i]) || !settings->recent_variables[i]->isActive()) {
			settings->recent_variables.erase(settings->recent_variables.begin() + i);
		} else {
			i++;
		}
	}
	favouriteVariableActions.clear();
	recentVariableActions.clear();
	if(settings->show_all_variables) {

		tree_struct_m variable_cats;
		std::vector<Variable*> user_variables;

		{size_t cat_i, cat_i_prev;
		bool b;
		std::string str, cat, cat_sub;
		Variable *v = NULL;
		variable_cats.items.clear();
		variable_cats.objects.clear();
		variable_cats.parent = NULL;
		user_variables.clear();
		std::list<tree_struct_m>::iterator it;
		for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
			if(CALCULATOR->variables[i]->isActive() && !CALCULATOR->variables[i]->isHidden()) {
				if(CALCULATOR->variables[i]->isLocal() && !CALCULATOR->variables[i]->isBuiltin()) {
					b = false;
					for(size_t i3 = 0; i3 < user_variables.size(); i3++) {
						v = user_variables[i3];
						if(string_is_less(CALCULATOR->variables[i]->title(true, settings->printops.use_unicode_signs), v->title(true, settings->printops.use_unicode_signs))) {
							b = true;
							user_variables.insert(user_variables.begin() + i3, CALCULATOR->variables[i]);
							break;
						}
					}
					if(!b) user_variables.push_back(CALCULATOR->variables[i]);
				}
				tree_struct_m *item = &variable_cats;
				if(!CALCULATOR->variables[i]->category().empty()) {
					cat = CALCULATOR->variables[i]->category();
					cat_i = cat.find("/"); cat_i_prev = 0;
					b = false;
					while(true) {
						if(cat_i == std::string::npos) {
							cat_sub = cat.substr(cat_i_prev, cat.length() - cat_i_prev);
						} else {
							cat_sub = cat.substr(cat_i_prev, cat_i - cat_i_prev);
						}
						b = false;
						for(it = item->items.begin(); it != item->items.end(); ++it) {
							if(cat_sub == it->item) {
								item = &*it;
								b = true;
								break;
							}
						}
						if(!b) {
							tree_struct_m cat;
							item->items.push_back(cat);
							it = item->items.end();
							--it;
							it->parent = item;
							item = &*it;
							item->item = cat_sub;
						}
						if(cat_i == std::string::npos) {
							break;
						}
						cat_i_prev = cat_i + 1;
						cat_i = cat.find("/", cat_i_prev);
					}
				}
				b = false;
				for(size_t i3 = 0; i3 < item->objects.size(); i3++) {
					v = (Variable*) item->objects[i3];
					if(string_is_less(CALCULATOR->variables[i]->title(true, settings->printops.use_unicode_signs), v->title(true, settings->printops.use_unicode_signs))) {
						b = true;
						item->objects.insert(item->objects.begin() + i3, (void*) CALCULATOR->variables[i]);
						break;
					}
				}
				if(!b) item->objects.push_back((void*) CALCULATOR->variables[i]);
			}
		}

		variable_cats.sort();}

		favouriteVariablesMenu = variablesMenu->addMenu(tr("Favorites"));
		recentVariablesMenu = variablesMenu->addMenu(tr("Recent"));
		int first_index = 5;
		QMenu *sub = variablesMenu, *sub2, *sub3;
		QAction *action = NULL;
		sub2 = sub;
		Variable *v;
		tree_struct_m *titem, *titem2;
		variable_cats.rit = variable_cats.items.rbegin();
		if(variable_cats.rit != variable_cats.items.rend()) {
			titem = &*variable_cats.rit;
			++variable_cats.rit;
			titem->rit = titem->items.rbegin();
		} else {
			titem = NULL;
		}
		std::stack<QMenu*> menus;
		menus.push(sub);
		sub3 = sub;
		if(!user_variables.empty()) {
			sub = variablesMenu->addMenu(tr("User variables"));
			for(size_t i = 0; i < user_variables.size(); i++) {
				action = sub->addAction(QString::fromStdString(user_variables[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(variableActivated()));
				action->setData(QVariant::fromValue((void*) user_variables[i]));
			}
			first_index++;
		}
		variablesMenu->addSeparator();
		while(titem) {
			if(!titem->items.empty() || !titem->objects.empty()) {
				gsub("&", "&&", titem->item);
				sub = new QMenu(QString::fromStdString(titem->item));
				sub3->insertMenu(sub3->actions().value(sub3 == variablesMenu ? first_index : 0), sub);
				menus.push(sub);
				sub3 = sub;
				for(size_t i = 0; i < titem->objects.size(); i++) {
					v = (Variable*) titem->objects[i];
					action = sub->addAction(QString::fromStdString(v->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(variableActivated()));
					action->setData(QVariant::fromValue((void*) v));
				}
			} else {
				titem = titem->parent;
			}
			while(titem && titem->rit == titem->items.rend()) {
				titem = titem->parent;
				menus.pop();
				if(menus.size() > 0) sub3 = menus.top();
			}
			if(titem) {
				titem2 = &*titem->rit;
				++titem->rit;
				titem = titem2;
				titem->rit = titem->items.rbegin();
			}
		}
		sub = sub2;
		for(size_t i = 0; i < variable_cats.objects.size(); i++) {
			v = (Variable*) variable_cats.objects[i];
			if(!v->isLocal()) {
				action = sub->addAction(QString::fromStdString(v->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), this, SLOT(variableActivated()));
				action->setData(QVariant::fromValue((void*) v));
			}
		}
		variablesMenu->addSeparator();
	} else {
		favouriteVariablesMenu = variablesMenu;
		recentVariablesMenu = variablesMenu;
	}
	QAction *action = variablesMenu->addAction(tr("Show all variables"));
	action->setCheckable(true);
	action->setChecked(settings->show_all_variables);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(showAllVariables(bool)));
	firstVariablesMenuOptionAction = action;
	updateRecentVariables();
	updateFavouriteVariables();
}
void QalculateWindow::showAllVariables(bool b) {
	settings->show_all_variables = b;
	updateVariablesMenu();
}
void QalculateWindow::updateFavouriteVariables() {
	if(variablesMenu->isEmpty()) return;
	for(int i = 0; i < favouriteVariableActions.size(); i++) {
		favouriteVariablesMenu->removeAction(favouriteVariableActions[i]);
		favouriteVariableActions[i]->deleteLater();
	}
	bool b_empty = settings->show_all_variables && favouriteVariableActions.isEmpty();
	favouriteVariableActions.clear();
	bool update_recent = false;
	if(!settings->favourite_variables.empty()) {
		std::sort(settings->favourite_variables.begin(), settings->favourite_variables.end(), sort_compare_item);
		for(size_t i = 0; i < settings->favourite_variables.size(); i++) {
			Variable *v = settings->favourite_variables[i];
			if(!settings->show_all_variables) {
				for(size_t i2 = 0; i2 < settings->recent_variables.size(); i2++) {
					if(settings->recent_variables[i2] == v) {
						settings->recent_variables.erase(settings->recent_variables.begin() + i2);
						update_recent = true;
						break;
					}
				}
			}
			QAction *action = new QAction(QString::fromStdString(v->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), favouriteVariablesMenu);
			connect(action, SIGNAL(triggered()), this, SLOT(variableActivated()));
			action->setData(QVariant::fromValue((void*) v));
			favouriteVariableActions << action;
		}
		if(!settings->show_all_variables) {
			QAction *action = new QAction(favouriteVariablesMenu);
			action->setSeparator(true);
			favouriteVariableActions << action;
		}
		if(update_recent) updateRecentVariables();
		favouriteVariablesMenu->insertActions(settings->show_all_variables ? NULL : (recentVariableActions.isEmpty() ? firstVariablesMenuOptionAction : recentVariableActions.at(0)), favouriteVariableActions);
		if(b_empty) {
			favouriteVariablesMenu->hide();
			favouriteVariablesMenu->show();
			favouriteVariablesMenu->hide();
		}
	}
}
void QalculateWindow::updateRecentVariables() {
	if(variablesMenu->isEmpty()) return;
	for(int i = 0; i < recentVariableActions.size(); i++) {
		recentVariablesMenu->removeAction(recentVariableActions[i]);
		recentVariableActions[i]->deleteLater();
	}
	bool b_empty = settings->show_all_variables && recentVariableActions.isEmpty();
	recentVariableActions.clear();
	if(!settings->recent_variables.empty()) {
		for(size_t i = 0; i < settings->recent_variables.size() && (i < 5 || settings->show_all_variables); i++) {
			QAction *action = new QAction(QString::fromStdString(settings->recent_variables[i]->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this)), recentVariablesMenu);
			connect(action, SIGNAL(triggered()), this, SLOT(variableActivated()));
			action->setData(QVariant::fromValue((void*) settings->recent_variables[i]));
			recentVariableActions << action;
		}
		if(!settings->show_all_variables) {
			QAction *action = new QAction(recentVariablesMenu);
			action->setSeparator(true);
			recentVariableActions << action;
		}
		recentVariablesMenu->insertActions(settings->show_all_variables ? NULL : firstVariablesMenuOptionAction, recentVariableActions);
		if(b_empty) {
			recentVariablesMenu->hide();
			recentVariablesMenu->show();
			recentVariablesMenu->hide();
		}
	}
}
void QalculateWindow::addToRecentVariables(Variable *v) {
	if(!settings->show_all_variables) {
		for(size_t i = 0; i < settings->favourite_variables.size(); i++) {
			if(settings->favourite_variables[i] == v) return;
		}
	}
	for(size_t i = 0; i < settings->recent_variables.size(); i++) {
		if(settings->recent_variables[i] == v) {
			if(i == 0) return;
			settings->recent_variables.erase(settings->recent_variables.begin() + i);
			break;
		}
	}
	if(settings->recent_variables.size() > 10) settings->recent_variables.pop_back();
	settings->recent_variables.insert(settings->recent_variables.begin(), v);
	updateRecentVariables();
}
void QalculateWindow::variableActivated() {
	Variable *v = (Variable*) ((QAction*) sender())->data().value<void*>();
	onVariableClicked(v);
	addToRecentVariables(v);
}
void QalculateWindow::unitActivated() {
	onUnitActivated((Unit*) ((QAction*) sender())->data().value<void*>());
}
void QalculateWindow::prefixActivated() {
	Prefix *p = (Prefix*) ((QAction*) sender())->data().value<void*>();
	if(!p) return;
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(QString::fromStdString(p->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(-1, true)));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::functionActivated() {
	MathFunction *f = (MathFunction*) ((QAction*) sender())->data().value<void*>();
	if(settings->use_function_dialog) insertFunction(f, this);
	else onFunctionClicked(f);
	addToRecentFunctions(f);
}
void QalculateWindow::registerUp() {
	if(CALCULATOR->RPNStackSize() <= 1) return;
	QList<QTableWidgetItem*> list = rpnView->selectedItems();
	QTableWidgetItem *item, *item2;
	if(list.isEmpty()) item = rpnView->item(0, 0);
	else item = list[0];
	if(!item) return;
	int index = item->row();
	if(index == 0) {
		CALCULATOR->moveRPNRegister(1, CALCULATOR->RPNStackSize());
		item2 = rpnView->item(CALCULATOR->RPNStackSize() - 1, 0);
	} else {
		CALCULATOR->moveRPNRegisterUp(index + 1);
		item2 = rpnView->item(index - 1, 0);
	}
	if(item2) {
		QString str = item->text();
		rpnView->blockSignals(true);
		item->setText(item2->text());
		item2->setText(str);
		rpnView->blockSignals(false);
	}
}
void QalculateWindow::registerDown() {
	if(CALCULATOR->RPNStackSize() <= 1) return;
	QList<QTableWidgetItem*> list = rpnView->selectedItems();
	QTableWidgetItem *item, *item2;
	if(list.isEmpty()) item = rpnView->item(0, 0);
	else item = list[0];
	if(!item) return;
	int index = item->row();
	if(index + 1 == (int) CALCULATOR->RPNStackSize()) {
		CALCULATOR->moveRPNRegister(CALCULATOR->RPNStackSize(), 1);
		item2 = rpnView->item(0, 0);
	} else {
		CALCULATOR->moveRPNRegisterDown(index + 1);
		item2 = rpnView->item(index + 1, 0);
	}
	if(item2) {
		QString str = item->text();
		rpnView->blockSignals(true);
		item->setText(item2->text());
		item2->setText(str);
		rpnView->blockSignals(false);
	}
}
void QalculateWindow::registerSwap() {
	if(CALCULATOR->RPNStackSize() <= 1) return;
	QList<QTableWidgetItem*> list = rpnView->selectedItems();
	QTableWidgetItem *item, *item2;
	if(list.isEmpty()) item = rpnView->item(0, 0);
	else item = list[0];
	if(!item) return;
	int index = item->row();
	if(index == 0) {
		CALCULATOR->moveRPNRegister(1, 2);
		item2 = rpnView->item(1, 0);
	} else {
		CALCULATOR->moveRPNRegister(index + 1, 1);
		item2 = rpnView->item(0, 0);
	}
	if(item2) {
		QString str = item->text();
		rpnView->blockSignals(true);
		item->setText(item2->text());
		item2->setText(str);
		rpnView->blockSignals(false);
	}
}
void QalculateWindow::rpnLastX() {
	if(expressionEdit->expressionHasChanged()) {
		if(!expressionEdit->toPlainText().trimmed().isEmpty()) {
			calculateExpression(true);
		}
	}
	CALCULATOR->RPNStackEnter(new MathStructure(lastx));
	RPNRegisterAdded(lastx_text.toStdString(), 0);
}
void QalculateWindow::copyRegister() {
	if(CALCULATOR->RPNStackSize() == 0) return;
	QList<QTableWidgetItem*> list = rpnView->selectedItems();
	QTableWidgetItem *item;
	if(list.isEmpty()) item = rpnView->item(0, 0);
	else item = list[0];
	if(!item) return;
	int index = item->row();
	CALCULATOR->RPNStackEnter(new MathStructure(*CALCULATOR->getRPNRegister(index + 1)));
	RPNRegisterAdded(item->text().toStdString(), 0);
}
void QalculateWindow::deleteRegister() {
	if(CALCULATOR->RPNStackSize() == 0) return;
	QList<QTableWidgetItem*> list = rpnView->selectedItems();
	QTableWidgetItem *item;
	if(list.isEmpty()) item = rpnView->item(0, 0);
	else item = list[0];
	if(!item) return;
	int index = item->row();
	CALCULATOR->deleteRPNRegister(index + 1);
	RPNRegisterRemoved(index);
}
void QalculateWindow::clearStack() {
	CALCULATOR->clearRPNStack();
	rpnView->clear();
	rpnView->setRowCount(0);
	rpnCopyAction->setEnabled(false);
	rpnDeleteAction->setEnabled(false);
	rpnClearAction->setEnabled(false);
	rpnUpAction->setEnabled(false);
	rpnDownAction->setEnabled(false);
	rpnSwapAction->setEnabled(false);
}
void QalculateWindow::registerChanged(int index) {
	calculateExpression(true, false, OPERATION_ADD, NULL, true, index);
}

void QalculateWindow::onInsertTextRequested(std::string str) {
	expressionEdit->blockCompletion();
	gsub("…", "", str);
	expressionEdit->insertPlainText(QString::fromStdString(unhtmlize(str)));
	expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::showAbout() {
	QMessageBox::about(this, tr("About %1").arg(qApp->applicationDisplayName()), QString("<font size=+2><b>%1 v%4</b></font><br><font size=+1>%2</font><br><font size=+1><i><a href=\"https://qalculate.github.io/\">https://qalculate.github.io/</a></i></font><br><br>Copyright © 2003-2007, 2008, 2016-2024 Hanna Knutsson<br>%3").arg(qApp->applicationDisplayName()).arg(tr("Powerful and easy to use calculator")).arg(tr("License: GNU General Public License version 2 or later")).arg(qApp->applicationVersion()));
}
void QalculateWindow::onInsertValueRequested(int i) {
	expressionEdit->blockCompletion();
	Number nr(i, 1, 0);
	expressionEdit->insertPlainText(QString("%1(%2)").arg(QString::fromStdString(settings->f_answer->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true))).arg(QString::fromStdString(print_with_evalops(nr))));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onSymbolClicked(const QString &str) {
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(str);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onOperatorClicked(const QString &str) {
	QString s, s_low;
	if(str.length() >= 3) {
		s_low = str.toLower();
		if(str == "NOT") s = "!";
		else if(s_low == "not") s = str + " ";
		else if(s_low == "nor" || s_low == "mod" || s_low == "rem" || s_low == "comb" || s_low == "perm" || s_low == "xor" || s_low == "bitand" || s_low == "bitor" || s_low == "nand" || s_low == "cross" || s_low == "dot" || s_low == "and" || s_low == "or" || s_low == "per" || s_low == "times" || s_low == "minus" || s_low == "plus" || s_low == "div") s = " " + str + " ";
		else s = str;
	} else {
		s = str;
	}
	if(settings->rpn_mode && str != "NOT" && str != "not") {
		if(s != str || str == "%") {
			if(s_low == "mod") {onFunctionClicked(CALCULATOR->getFunctionById(FUNCTION_ID_MOD)); return;}
			else if(s_low == "rem" || str == "%") {onFunctionClicked(CALCULATOR->getFunctionById(FUNCTION_ID_REM)); return;}
			MathFunction *f = CALCULATOR->getActiveFunction(s_low.toStdString());
			if(f) {
				onFunctionClicked(f);
				return;
			}
		}
		if(expressionEdit->expressionHasChanged()) {
			if(!expressionEdit->toPlainText().trimmed().isEmpty()) {
				calculateExpression(true);
			}
		}
		calculateExpression(true, false, OPERATION_ADD, NULL, false, 0, str.toStdString());
		return;
	}
	expressionEdit->blockCompletion();
	bool do_exec = false;
	if(str == "~") {
		expressionEdit->wrapSelection(str, true, false);
	} else if(str == "!") {
		QTextCursor cur = expressionEdit->textCursor();
		do_exec = (str == "!") && cur.hasSelection() && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
		if(do_exec) expressionEdit->blockParseStatus();
		expressionEdit->wrapSelection(str);
		if(do_exec) expressionEdit->blockParseStatus(false);
	} else if(str == "E" || str == "e") {
		if(expressionEdit->textCursor().hasSelection()) expressionEdit->wrapSelection(QString::fromUtf8(settings->multiplicationSign()) + "10^");
		else expressionEdit->insertPlainText(settings->printops.exp_display == EXP_UPPERCASE_E ? "E" : "e");
	} else {
		if((str == "//" || (s != str && (s_low == "mod" || s_low == "rem"))) && (settings->rpn_mode || expressionEdit->document()->isEmpty() || settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN || (expressionEdit->textCursor().atStart() && !expressionEdit->textCursor().hasSelection()))) {
			MathFunction *f;
			if(s_low == "mod") f = CALCULATOR->getFunctionById(FUNCTION_ID_MOD);
			else if(s_low == "rem") f = CALCULATOR->getFunctionById(FUNCTION_ID_REM);
			else f = CALCULATOR->getActiveFunction("div");
			if(f) {
				expressionEdit->blockCompletion(false);
				onFunctionClicked(f);
				return;
			}
		}
		if(!expressionEdit->doChainMode(s)) expressionEdit->wrapSelection(s);
	}
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	if(do_exec) calculate();
	expressionEdit->blockCompletion(false);
}

bool last_is_number(std::string str) {
	str = CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options);
	CALCULATOR->parseSigns(str);
	if(str.empty()) return false;
	return is_not_in(OPERATORS SPACES SEXADOT DOT LEFT_VECTOR_WRAP LEFT_PARENTHESIS COMMAS, str[str.length() - 1]);
}
void QalculateWindow::onFunctionClicked(MathFunction *f) {
	if(!f) return;
	if(settings->rpn_mode && (f->minargs() <= 1 || (int) CALCULATOR->RPNStackSize() >= f->minargs())) {
		calculateRPN(f);
		return;
	}
	QString sargs;
	bool b_text = USE_QUOTES(f->getArgumentDefinition(1), f);
	if(f->id() == FUNCTION_ID_CIRCULAR_SHIFT || f->id() == FUNCTION_ID_BIT_CMP) {
		Argument *arg_bits = f->getArgumentDefinition(f->id() == FUNCTION_ID_CIRCULAR_SHIFT ? 3 : 2);
		Argument *arg_steps = (f->id() == FUNCTION_ID_CIRCULAR_SHIFT ? f->getArgumentDefinition(2) : NULL);
		Argument *arg_signed = f->getArgumentDefinition(f->id() == FUNCTION_ID_CIRCULAR_SHIFT ? 4 : 3);
		MathSpinBox *spin = NULL;
		if(arg_bits && arg_signed && (f->id() != FUNCTION_ID_CIRCULAR_SHIFT || arg_steps)) {
			QDialog *dialog = new QDialog(this);
			if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
			dialog->setWindowTitle(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)));
			QVBoxLayout *box = new QVBoxLayout(dialog);
			QGridLayout *grid = new QGridLayout();
			box->addLayout(grid);
			if(arg_steps) {
				grid->addWidget(new QLabel(tr("%1:").arg(QString::fromStdString(arg_steps->name()))), 0, 0);
				int min = INT_MIN, max = INT_MAX;
				if(arg_steps->type() == ARGUMENT_TYPE_INTEGER) {
					IntegerArgument *iarg = (IntegerArgument*) arg_steps;
					if(iarg->min()) {
						min = iarg->min()->intValue();
					}
					if(iarg->max()) {
						max = iarg->max()->intValue();
					}
				}
				spin = new MathSpinBox();
				spin->setRange(min, max);
				if(!f->getDefaultValue(2).empty()) {
					spin->setValue(s2i(f->getDefaultValue(2)));
				} else if(!arg_steps->zeroForbidden() && min <= 0 && max >= 0) {
					spin->setValue(0);
				} else {
					if(max < 0) {
						spin->setValue(max);
					} else if(min <= 1) {
						spin->setValue(1);
					} else {
						spin->setValue(min);
					}
				}
				grid->addWidget(spin, 0, 1);
			}
			grid->addWidget(new QLabel(tr("%1:").arg(QString::fromStdString(arg_bits->name()))), arg_steps ? 1 : 0, 0);
			QComboBox *combo = new QComboBox(dialog);
			combo->addItem("8", 8);
			combo->addItem("16", 16);
			combo->addItem("32", 32);
			combo->addItem("64", 64);
			combo->addItem("128", 128);
			combo->addItem("256", 256);
			combo->addItem("512", 512);
			combo->addItem("1024", 1024);
			int i = combo->findData(settings->default_bits >= 0 ? settings->default_bits : (settings->evalops.parse_options.binary_bits > 0 ? settings->evalops.parse_options.binary_bits : 32));
			if(i < 0) i = 2;
			combo->setCurrentIndex(i);
			grid->addWidget(combo, arg_steps ? 1 : 0, 1);
			QCheckBox *button = new QCheckBox(QString::fromStdString(arg_signed->name()));
			button->setChecked(settings->default_signed > 0 || (settings->default_signed < 0 && (settings->evalops.parse_options.twos_complement || (f->id() == FUNCTION_ID_CIRCULAR_SHIFT && settings->printops.twos_complement))));
			grid->addWidget(button, arg_steps ? 2 : 1, 0, 2, 1, Qt::AlignRight);
			QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
			buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
			buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
			box->addWidget(buttonBox);
			connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
			connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
			if(dialog->exec() == QDialog::Accepted) {
				Number bits = combo->currentData().toInt();
				if(bits.isZero()) bits = 32;
				sargs = QString::fromStdString(CALCULATOR->getComma());
				sargs += " ";
				if(spin) {
					if(settings->evalops.parse_options.base != BASE_DECIMAL) {
						Number nr(spin->value(), 1);
						sargs += QString::fromStdString(print_with_evalops(nr));
					} else {
						sargs += spin->cleanText();
					}
					sargs += QString::fromStdString(CALCULATOR->getComma());
					sargs += " ";
				}
				sargs += QString::fromStdString(print_with_evalops(bits));
				sargs += QString::fromStdString(CALCULATOR->getComma());
				sargs += " ";
				if(button->isChecked()) sargs += "1";
				else sargs += "0";
				settings->default_bits = bits.intValue();
				settings->default_signed = button->isChecked();
				dialog->deleteLater();
			} else {
				dialog->deleteLater();
				return;
			}
		}
	} else if(f->id() == FUNCTION_ID_ROOT || f->id() == FUNCTION_ID_LOGN || (f->minargs() == 2 && ((f->getArgumentDefinition(2) && f->getArgumentDefinition(2)->type() == ARGUMENT_TYPE_INTEGER) xor (f->getArgumentDefinition(1) && f->getArgumentDefinition(1)->type() == ARGUMENT_TYPE_INTEGER)))) {
		size_t index = 2;
		if(f->id() != FUNCTION_ID_ROOT && f->id() != FUNCTION_ID_LOGN && f->getArgumentDefinition(1) && f->getArgumentDefinition(1)->type() == ARGUMENT_TYPE_INTEGER) index = 1;
		Argument *arg_n = f->getArgumentDefinition(index);
		if(arg_n) {
			QLineEdit *entry = NULL;
			MathSpinBox *spin = NULL;
			QDialog *dialog = new QDialog(this);
			if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
			dialog->setWindowTitle(QString::fromStdString(f->title(true, settings->printops.use_unicode_signs)));
			QVBoxLayout *box = new QVBoxLayout(dialog);
			QGridLayout *grid = new QGridLayout();
			box->addLayout(grid);
			grid->addWidget(new QLabel(tr("%1:").arg(QString::fromStdString(arg_n->name()))), 0, 0);
			int min = INT_MIN, max = INT_MAX;
			if(arg_n->type() == ARGUMENT_TYPE_INTEGER) {
				IntegerArgument *iarg = (IntegerArgument*) arg_n;
				if(iarg->min()) {
					min = iarg->min()->intValue();
				}
				if(iarg->max()) {
					max = iarg->max()->intValue();
				}
			}
			if(settings->evalops.parse_options.base != BASE_DECIMAL || f->id() == FUNCTION_ID_ROOT || arg_n->type() == ARGUMENT_TYPE_INTEGER) {
				spin = new MathSpinBox(dialog);
				spin->setRange(min, max);
				if(f->id() == FUNCTION_ID_ROOT || f->id() == FUNCTION_ID_LOGN) {
					spin->setValue(2);
				} else if(!f->getDefaultValue(index).empty()) {
					spin->setValue(s2i(f->getDefaultValue(index)));
				} else if(!arg_n->zeroForbidden() && min <= 0 && max >= 0) {
					spin->setValue(0);
				} else {
					if(max < 0) {
						spin->setValue(max);
					} else if(min <= 1) {
						spin->setValue(1);
					} else {
						spin->setValue(min);
					}
				}
				grid->addWidget(spin, 0, 1);
				spin->setFocus();
				connect(spin, SIGNAL(returnPressed()), dialog, SLOT(accept()));
			} else {
				entry = new MathLineEdit(dialog);
				entry->setText(QString::fromStdString(f->getDefaultValue(index)));
				grid->addWidget(entry, 0, 1);
				entry->setFocus();
				connect(entry, SIGNAL(returnPressed()), dialog, SLOT(accept()));
			}
			QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
			buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
			buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
			box->addWidget(buttonBox);
			connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
			connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
			if(dialog->exec() == QDialog::Accepted) {
				sargs = QString::fromStdString(CALCULATOR->getComma());
				sargs += " ";
				if(spin && settings->evalops.parse_options.base != BASE_DECIMAL) {
					Number nr(spin->value(), 1);
					sargs += QString::fromStdString(print_with_evalops(nr));
				} else if(spin) {
					sargs += spin->cleanText();
				} else {
					sargs += entry->text();
				}
				dialog->deleteLater();
			} else {
				dialog->deleteLater();
				return;
			}
		}
	}
	expressionEdit->blockCompletion();
	QTextCursor cur = expressionEdit->textCursor();
	bool do_exec = false;
	int minargs = f->minargs();
	if(minargs == 1 && f->id() == FUNCTION_ID_LOGN) minargs = 2;
	if(settings->chain_mode) {
		if(!expressionEdit->document()->isEmpty()) {
			expressionEdit->selectAll();
			do_exec = !sargs.isEmpty() || minargs <= 1;
		}
	} else if(cur.hasSelection()) {
		do_exec = (!sargs.isEmpty() || minargs <= 1) && cur.selectionStart() == 0 && cur.selectionEnd() == expressionEdit->toPlainText().length();
	} else if(cur.atEnd() && last_is_number(expressionEdit->toPlainText().toStdString())) {
		expressionEdit->selectAll();
		do_exec = !sargs.isEmpty() || minargs <= 1;
	}
	if(do_exec) expressionEdit->blockParseStatus();
	expressionEdit->wrapSelection(f->referenceName() == "neg" ? SIGN_MINUS : QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true)), true, true, minargs > 1 && sargs.isEmpty(), sargs, b_text);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	if(do_exec) expressionEdit->blockParseStatus(false);
	expressionEdit->blockCompletion(false);
	if(do_exec) calculate();
}
void QalculateWindow::negate() {
	onFunctionClicked(CALCULATOR->getActiveFunction("neg"));
}
void QalculateWindow::onVariableClicked(Variable *v) {
	if(!v) return;
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(QString::fromStdString(v->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_VARIABLE, true)));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onUnitClicked(Unit *u) {
	if(!u) return;
	expressionEdit->blockCompletion();
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		PrintOptions po = settings->printops;
		po.is_approximate = NULL;
		po.can_display_unicode_string_arg = (void*) expressionEdit;
		expressionEdit->insertPlainText(QString::fromStdString(((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true)));
	} else {
		expressionEdit->insertPlainText(QString::fromStdString(u->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_UNIT, true)));
	}
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onPrefixClicked(Prefix *p) {
	if(!p) return;
	expressionEdit->blockCompletion();
	expressionEdit->insertPlainText(QString::fromStdString(p->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(-1, true)));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onDelClicked() {
	expressionEdit->blockCompletion();
	QTextCursor cur = expressionEdit->textCursor();
	if(cur.atEnd()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onBackspaceClicked() {
	expressionEdit->blockCompletion();
	QTextCursor cur = expressionEdit->textCursor();
	if(!cur.atStart()) cur.deletePreviousChar();
	else cur.deleteChar();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onClearClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->clear();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onEqualsClicked() {
	calculate();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onParenthesesClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->smartParentheses();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onBracketsClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->insertBrackets();
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onLeftClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->moveCursor(QTextCursor::PreviousCharacter);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onRightClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->moveCursor(QTextCursor::NextCharacter);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onStartClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->moveCursor(QTextCursor::Start);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onEndClicked() {
	expressionEdit->blockCompletion();
	expressionEdit->moveCursor(QTextCursor::End);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::onMSClicked() {
	if(expressionEdit->expressionHasChanged()) calculate();
	if(!mstruct) return;
	settings->v_memory->set(*mstruct);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMRClicked() {
	bool b_exec = !expressionEdit->expressionHasChanged();
	onVariableClicked(settings->v_memory);
	if(b_exec) calculate();
}
void QalculateWindow::onMCClicked() {
	settings->v_memory->set(m_zero);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMPlusClicked() {
	if(expressionEdit->expressionHasChanged()) calculate();
	if(!mstruct) return;
	MathStructure m = settings->v_memory->get();
	m.calculateAdd(*mstruct, settings->evalops);
	settings->v_memory->set(m);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onMMinusClicked() {
	if(expressionEdit->expressionHasChanged()) calculate();
	if(!mstruct) return;
	MathStructure m = settings->v_memory->get();
	m.calculateAdd(*mstruct, settings->evalops);
	settings->v_memory->set(m);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
}
void QalculateWindow::onAnswerClicked() {
	if(settings->history_answer.size() > 0) {
		onInsertValueRequested(settings->history_answer.size());
	}
}
void QalculateWindow::onBaseClicked(int v, bool b, bool b_update) {
	if(b && v != settings->evalops.parse_options.base) {
		settings->evalops.parse_options.base = v;
		QAction *action = find_child_data(this, "group_inbase", v);
		if(action) action->setChecked(true);
		expressionFormatUpdated(false);
	}
	if(v != settings->printops.base) {
		settings->printops.base = v;
		to_base = 0;
		to_bits = 0;
		QAction *action = find_child_data(this, "group_outbase", v);
		if(action) action->setChecked(true);
		if(b_update) resultFormatUpdated();
	}
	keypad->updateBase();
}
void QalculateWindow::onFactorizeClicked() {
	executeCommand(COMMAND_FACTORIZE);
}
void QalculateWindow::onExpandClicked() {
	executeCommand(COMMAND_EXPAND);
}
void QalculateWindow::onExpandPartialFractionsClicked() {
	executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
}
void QalculateWindow::serverNewConnection() {
	socket = server->nextPendingConnection();
	if(socket) {
		connect(socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
	}
}
void QalculateWindow::socketReadyRead() {
	QString command = socket->readAll();
	if(!command.isEmpty() && command[0] == '+') {
		settings->window_state = saveState();
		settings->window_geometry = saveGeometry();
		settings->splitter_state = ehSplitter->saveState();
		settings->allow_multiple_instances = true;
		settings->show_bases = basesDock->isVisible();
		settings->savePreferences(false);
		return;
	}
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	qApp->processEvents();
	raise();
	activateWindow();
	if(command.isEmpty()) return;
	if(command[0] == '-') {
		settings->window_state = saveState();
		if(height() != DEFAULT_HEIGHT || width() != DEFAULT_WIDTH) settings->window_geometry = saveGeometry();
		else settings->window_geometry = QByteArray();
		settings->splitter_state = ehSplitter->saveState();
		settings->allow_multiple_instances = false;
		settings->show_bases = basesDock->isVisible();
		settings->savePreferences(false);
		command = command.mid(1).trimmed();
		if(command.isEmpty()) return;
	}
	if(command[0] == 'f') {
		command = command.mid(1).trimmed();
		if(!command.isEmpty()) {
			int i = command.indexOf(";");
			if(i > 0) {
				QString file = command.left(i);
				executeFromFile(file);
				command = command.mid(i);
			}
		}
	} else if(command[0] == 'w') {
		command = command.mid(1).trimmed();
		if(!command.isEmpty()) {
			int i = command.indexOf(";");
			if(i > 0) {
				QString file = command.left(i);
				loadWorkspace(file);
				command = command.mid(i);
			}
		}
	}
	command = command.mid(1).trimmed();
	if(!command.isEmpty()) {expressionEdit->setExpression(command); calculate();}
}
void QalculateWindow::onActivateRequested(const QStringList &arguments, const QString&) {
	if(!arguments.isEmpty()) {
		parser->process(arguments);
		QStringList args = parser->positionalArguments();
		QString command;
		for(int i = 0; i < args.count(); i++) {
			if(i > 0) command += " ";
			command += args.at(i);
		}
		command = command.trimmed();
		if(!command.isEmpty()) {expressionEdit->setExpression(command); calculate();}
		args.clear();
	}
	setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
	show();
	qApp->processEvents();
	raise();
	activateWindow();
}
void QalculateWindow::setCommandLineParser(QCommandLineParser *p) {
	parser = p;
}
void QalculateWindow::calculate() {
	calculateExpression();
}
void QalculateWindow::calculate(const QString &expression) {
	expressionEdit->setExpression(expression);
	calculate();
}

void QalculateWindow::setPreviousExpression() {
	if(settings->rpn_mode) {
		expressionEdit->clear();
	} else {
		expressionEdit->blockCompletion();
		expressionEdit->blockParseStatus();
		expressionEdit->setExpression(QString::fromStdString(previous_expression));
		expressionEdit->selectAll();
		expressionEdit->setExpressionHasChanged(false);
		expressionEdit->blockCompletion(false);
		expressionEdit->blockParseStatus(false);
		historyView->clearTemporary();
	}
}

void QalculateWindow::resultFormatUpdated(int delay) {
	if(rfTimer) rfTimer->stop();
	if(block_result_update) return;
	if(delay > 0) {
		if(!rfTimer) {
			rfTimer = new QTimer();
			rfTimer->setSingleShot(true);
			connect(rfTimer, SIGNAL(timeout()), this, SLOT(resultFormatUpdated()));
		}
		rfTimer->start(delay);
		return;
	}
	settings->updateMessagePrintOptions();
	workspace_changed = true;
	setResult(NULL, true, false, false);
	auto_format_updated = true;
	if(!QToolTip::text().isEmpty() || (settings->status_in_history && expressionEdit->expressionHasChanged())) expressionEdit->displayParseStatus(true);
}
void QalculateWindow::resultDisplayUpdated() {
	resultFormatUpdated();
}
void QalculateWindow::expressionFormatUpdated(bool recalculate) {
	settings->updateMessagePrintOptions();
	if(!expressionEdit->expressionHasChanged() && !recalculate && !settings->rpn_mode) {
		expressionEdit->clear();
	} else if(!settings->rpn_mode && parsed_mstruct) {
		for(size_t i = 0; i < 5; i++) {
			if(parsed_mstruct->contains(settings->vans[i])) expressionEdit->clear();
		}
	}
	workspace_changed = true;
	if(!settings->rpn_mode && recalculate) {
		calculateExpression(false);
	}
	if(expressionEdit->expressionHasChanged()) expressionEdit->displayParseStatus(true, !QToolTip::text().isEmpty());
}
void QalculateWindow::expressionCalculationUpdated(int delay) {
	if(ecTimer) ecTimer->stop();
	if(delay > 0) {
		if(!ecTimer) {
			ecTimer = new QTimer();
			ecTimer->setSingleShot(true);
			connect(ecTimer, SIGNAL(timeout()), this, SLOT(expressionCalculationUpdated()));
		}
		ecTimer->start(delay);
		return;
	}
	workspace_changed = true;
	auto_calculation_updated = true;
	settings->updateMessagePrintOptions();
	if(!settings->rpn_mode) {
		if(parsed_mstruct) {
			for(size_t i = 0; i < 5; i++) {
				if(parsed_mstruct->contains(settings->vans[i])) return;
			}
		}
		calculateExpression(false);
	}
	if(expressionEdit->expressionHasChanged()) expressionEdit->displayParseStatus(true, !QToolTip::text().isEmpty());
}

int s2b(const std::string &str) {
	if(str.empty()) return -1;
	if(equalsIgnoreCase(str, "yes")) return 1;
	if(equalsIgnoreCase(str, "no")) return 0;
	if(equalsIgnoreCase(str, "true")) return 1;
	if(equalsIgnoreCase(str, "false")) return 0;
	if(equalsIgnoreCase(str, "on")) return 1;
	if(equalsIgnoreCase(str, "off")) return 0;
	if(str.find_first_not_of(SPACES NUMBERS) != std::string::npos) return -1;
	int i = s2i(str);
	if(i > 0) return 1;
	return 0;
}
void set_assumption(const std::string &str, AssumptionType &at, AssumptionSign &as, bool last_of_two = false) {
	if(equalsIgnoreCase(str, "none") || str == "0") {
		as = ASSUMPTION_SIGN_UNKNOWN;
		at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "unknown")) {
		if(!last_of_two) as = ASSUMPTION_SIGN_UNKNOWN;
		else at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "real")) {
		at = ASSUMPTION_TYPE_REAL;
	} else if(equalsIgnoreCase(str, "number") || equalsIgnoreCase(str, "complex") || str == "num" || str == "cplx") {
		at = ASSUMPTION_TYPE_NUMBER;
	} else if(equalsIgnoreCase(str, "rational") || str == "rat") {
		at = ASSUMPTION_TYPE_RATIONAL;
	} else if(equalsIgnoreCase(str, "integer") || str == "int") {
		at = ASSUMPTION_TYPE_INTEGER;
	} else if(equalsIgnoreCase(str, "boolean") || str == "bool") {
		at = ASSUMPTION_TYPE_BOOLEAN;
	} else if(equalsIgnoreCase(str, "non-zero") || str == "nz") {
		as = ASSUMPTION_SIGN_NONZERO;
	} else if(equalsIgnoreCase(str, "positive") || str == "pos") {
		as = ASSUMPTION_SIGN_POSITIVE;
	} else if(equalsIgnoreCase(str, "non-negative") || str == "nneg") {
		as = ASSUMPTION_SIGN_NONNEGATIVE;
	} else if(equalsIgnoreCase(str, "negative") || str == "neg") {
		as = ASSUMPTION_SIGN_NEGATIVE;
	} else if(equalsIgnoreCase(str, "non-positive") || str == "npos") {
		as = ASSUMPTION_SIGN_NONPOSITIVE;
	} else {
		CALCULATOR->error(true, "Unrecognized assumption: %s.", str.c_str(), NULL);
	}
}

#define SET_BOOL_D(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; resultDisplayUpdated();}}
#define SET_BOOL_E(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionCalculationUpdated();}}
#define SET_BOOL(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v;}}
#define SET_BOOL_PV(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionFormatUpdated(v);}}
#define SET_BOOL_PT(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionFormatUpdated(true);}}
#define SET_BOOL_PF(x)		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(x != v) {x = v; expressionFormatUpdated(false);}}

void QalculateWindow::setOption(std::string str) {
	remove_blank_ends(str);
	gsub(SIGN_MINUS, "-", str);
	std::string svalue, svar;
	bool empty_value = false;
	size_t i_underscore = str.find("_");
	size_t index;
	if(i_underscore != std::string::npos) {
		index = str.find_first_of(SPACES);
		if(index != std::string::npos && i_underscore > index) i_underscore = std::string::npos;
	}
	if(i_underscore == std::string::npos) index = str.find_last_of(SPACES);
	if(index != std::string::npos) {
		svar = str.substr(0, index);
		remove_blank_ends(svar);
		svalue = str.substr(index + 1);
		remove_blank_ends(svalue);
	} else {
		svar = str;
	}
	if(i_underscore != std::string::npos) gsub("_", " ", svar);
	if(svalue.empty()) {
		empty_value = true;
		svalue = "1";
	}

	if(preferencesDialog) {
		preferencesDialog->hide();
		preferencesDialog->deleteLater();
		preferencesDialog = NULL;
	}

	set_option_place:
	if(equalsIgnoreCase(svar, "base") || equalsIgnoreCase(svar, "input base") || svar == "inbase" || equalsIgnoreCase(svar, "output base") || svar == "outbase") {
		int v = 0;
		bool b_in = equalsIgnoreCase(svar, "input base") || svar == "inbase";
		bool b_out = equalsIgnoreCase(svar, "output base") || svar == "outbase";
		if(equalsIgnoreCase(svalue, "roman")) v = BASE_ROMAN_NUMERALS;
		else if(equalsIgnoreCase(svalue, "bijective") || str == "b26" || str == "B26") v = BASE_BIJECTIVE_26;
		if(equalsIgnoreCase(svalue, "bcd")) v = BASE_BINARY_DECIMAL;
		else if(equalsIgnoreCase(svalue, "fp32") || equalsIgnoreCase(svalue, "binary32") || equalsIgnoreCase(svalue, "float")) {if(b_in) v = 0; else v = BASE_FP32;}
		else if(equalsIgnoreCase(svalue, "fp64") || equalsIgnoreCase(svalue, "binary64") || equalsIgnoreCase(svalue, "double")) {if(b_in) v = 0; else v = BASE_FP64;}
		else if(equalsIgnoreCase(svalue, "fp16") || equalsIgnoreCase(svalue, "binary16")) {if(b_in) v = 0; else v = BASE_FP16;}
		else if(equalsIgnoreCase(svalue, "fp80")) {if(b_in) v = 0; else v = BASE_FP80;}
		else if(equalsIgnoreCase(svalue, "fp128") || equalsIgnoreCase(svalue, "binary128")) {if(b_in) v = 0; else v = BASE_FP128;}
		else if(equalsIgnoreCase(svalue, "time")) {if(b_in) v = 0; else v = BASE_TIME;}
		else if(equalsIgnoreCase(svalue, "hex") || equalsIgnoreCase(svalue, "hexadecimal")) v = BASE_HEXADECIMAL;
		else if(equalsIgnoreCase(svalue, "golden") || equalsIgnoreCase(svalue, "golden ratio") || svalue == "φ") v = BASE_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "supergolden") || equalsIgnoreCase(svalue, "supergolden ratio") || svalue == "ψ") v = BASE_SUPER_GOLDEN_RATIO;
		else if(equalsIgnoreCase(svalue, "pi") || svalue == "π") v = BASE_PI;
		else if(svalue == "e") v = BASE_E;
		else if(svalue == "sqrt(2)" || svalue == "sqrt 2" || svalue == "sqrt2" || svalue == "√2") v = BASE_SQRT2;
		else if(equalsIgnoreCase(svalue, "unicode")) v = BASE_UNICODE;
		else if(equalsIgnoreCase(svalue, "duo") || equalsIgnoreCase(svalue, "duodecimal")) v = 12;
		else if(equalsIgnoreCase(svalue, "bin") || equalsIgnoreCase(svalue, "binary")) v = BASE_BINARY;
		else if(equalsIgnoreCase(svalue, "oct") || equalsIgnoreCase(svalue, "octal")) v = BASE_OCTAL;
		else if(equalsIgnoreCase(svalue, "dec") || equalsIgnoreCase(svalue, "decimal")) v = BASE_DECIMAL;
		else if(equalsIgnoreCase(svalue, "sexa") || equalsIgnoreCase(svalue, "sexagesimal")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL;}
		else if(equalsIgnoreCase(svalue, "sexa2") || equalsIgnoreCase(svalue, "sexagesimal2")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_2;}
		else if(equalsIgnoreCase(svalue, "sexa3") || equalsIgnoreCase(svalue, "sexagesimal3")) {if(b_in) v = 0; else v = BASE_SEXAGESIMAL_3;}
		else if(equalsIgnoreCase(svalue, "latitude")) {if(b_in) v = 0; else v = BASE_LATITUDE;}
		else if(equalsIgnoreCase(svalue, "latitude2")) {if(b_in) v = 0; else v = BASE_LATITUDE_2;}
		else if(equalsIgnoreCase(svalue, "longitude")) {if(b_in) v = 0; else v = BASE_LONGITUDE;}
		else if(equalsIgnoreCase(svalue, "longitude2")) {if(b_in) v = 0; else v = BASE_LONGITUDE_2;}
		else if(!b_in && !b_out && (index = svalue.find_first_of(SPACES)) != std::string::npos) {
			str = svalue;
			svalue = str.substr(index + 1, str.length() - (index + 1));
			remove_blank_ends(svalue);
			svar += " ";
			str = str.substr(0, index);
			remove_blank_ends(str);
			svar += str;
			gsub("_", " ", svar);
			if(equalsIgnoreCase(svar, "base display")) {
				goto set_option_place;
			}
			setOption(std::string("inbase ") + svalue);
			setOption(std::string("outbase ") + str);
			return;
		} else if(!empty_value) {
			MathStructure m;
			EvaluationOptions eo = settings->evalops;
			eo.parse_options.base = 10;
			eo.approximation = APPROXIMATION_TRY_EXACT;
			CALCULATOR->beginTemporaryStopMessages();
			CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(svalue, eo.parse_options), 500, eo);
			if(CALCULATOR->endTemporaryStopMessages()) {
				v = 0;
			} else if(m.isInteger() && m.number() >= 2 && m.number() <= 36) {
				v = m.number().intValue();
			} else if(m.isNumber() && (b_in || ((!m.number().isNegative() || m.number().isInteger()) && (m.number() > 1 || m.number() < -1)))) {
				v = BASE_CUSTOM;
				if(b_in) CALCULATOR->setCustomInputBase(m.number());
				else CALCULATOR->setCustomOutputBase(m.number());
			}
		}
		if(v == 0) {
			CALCULATOR->error(true, "Illegal base: %s.", svalue.c_str(), NULL);
		} else if(b_in) {
			if(v == BASE_CUSTOM || v != settings->evalops.parse_options.base) {
				settings->evalops.parse_options.base = v;
				QAction *action = find_child_data(this, "group_inbase", v);
				if(!action) action = find_child_data(this, "group_inbase", BASE_CUSTOM);
				if(action) {
					action->setChecked(true);
					if(action->data().toInt() == BASE_CUSTOM && (v == BASE_CUSTOM || (v >= 2 && v <= 36))) {
						QSpinBox *w = findChild<QSpinBox*>("spinbox_inbase");
						if(w) {
							w->blockSignals(true);
							w->setValue(v == BASE_CUSTOM ? (CALCULATOR->customInputBase().isZero() ? 10 : CALCULATOR->customInputBase().intValue()) : v);
							w->blockSignals(false);
						}
					}
				}
				expressionFormatUpdated(false);
			}
		} else {
			if(v == BASE_CUSTOM || v != settings->printops.base) {
				settings->printops.base = v;
				to_base = 0;
				to_bits = 0;
				QAction *action = find_child_data(this, "group_outbase", v);
				if(!action) action = find_child_data(this, "group_outbase", BASE_CUSTOM);
				if(action) {
					action->setChecked(true);
					if(action->data().toInt() == BASE_CUSTOM && (v == BASE_CUSTOM || (v >= 2 && v <= 36))) {
						QSpinBox *w = findChild<QSpinBox*>("spinbox_outbase");
						if(w) {
							w->blockSignals(true);
							w->setValue(v == BASE_CUSTOM ? (CALCULATOR->customOutputBase().isZero() ? 10 : CALCULATOR->customOutputBase().intValue()) : v);
							w->blockSignals(false);
						}
					}
				}
				resultFormatUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "assumptions") || svar == "ass" || svar == "asm") {
		size_t i = svalue.find_first_of(SPACES);
		AssumptionType at = CALCULATOR->defaultAssumptions()->type();
		AssumptionSign as = CALCULATOR->defaultAssumptions()->sign();
		if(i != std::string::npos) {
			set_assumption(svalue.substr(0, i), at, as, false);
			set_assumption(svalue.substr(i + 1, svalue.length() - (i + 1)), at, as, true);
		} else {
			set_assumption(svalue, at, as, false);
		}
		QAction *action = find_child_data(this, "group_type", CALCULATOR->defaultAssumptions()->type());
		if(action) {
			action->setChecked(true);
		}
		action = find_child_data(this, "group_sign", CALCULATOR->defaultAssumptions()->sign());
		if(action) {
			action->setChecked(true);
		}
	} else if(equalsIgnoreCase(svar, "all prefixes") || svar == "allpref") SET_BOOL_D(settings->printops.use_all_prefixes)
	else if(equalsIgnoreCase(svar, "complex numbers") || svar == "cplx") SET_BOOL_E(settings->evalops.allow_complex)
	else if(equalsIgnoreCase(svar, "excessive parentheses") || svar == "expar") SET_BOOL_D(settings->printops.excessive_parenthesis)
	else if(equalsIgnoreCase(svar, "functions") || svar == "func") SET_BOOL_PV(settings->evalops.parse_options.functions_enabled)
	else if(equalsIgnoreCase(svar, "infinite numbers") || svar == "inf") SET_BOOL_E(settings->evalops.allow_infinite)
	else if(equalsIgnoreCase(svar, "show negative exponents") || svar == "negexp") SET_BOOL_D(settings->printops.negative_exponents)
	else if(equalsIgnoreCase(svar, "minus last") || svar == "minlast") {
		{int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);} else if(settings->printops.sort_options.minus_last != v) {settings->printops.sort_options.minus_last = v; resultDisplayUpdated();}}
	} else if(equalsIgnoreCase(svar, "assume nonzero denominators") || svar == "nzd") SET_BOOL_E(settings->evalops.assume_denominators_nonzero)
	else if(equalsIgnoreCase(svar, "warn nonzero denominators") || svar == "warnnzd") SET_BOOL_E(settings->evalops.warn_about_denominators_assumed_nonzero)
	else if(equalsIgnoreCase(svar, "prefixes") || svar == "pref") SET_BOOL_D(settings->printops.use_unit_prefixes)
	else if(equalsIgnoreCase(svar, "binary prefixes") || svar == "binpref") {
		bool b = CALCULATOR->usesBinaryPrefixes() > 0;
		SET_BOOL(b)
		if(b != (CALCULATOR->usesBinaryPrefixes() > 0)) {
			CALCULATOR->useBinaryPrefixes(b ? 1 : 0);
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "denominator prefixes") || svar == "denpref") SET_BOOL_D(settings->printops.use_denominator_prefix)
	else if(equalsIgnoreCase(svar, "place units separately") || svar == "unitsep") SET_BOOL_D(settings->printops.place_units_separately)
	else if(equalsIgnoreCase(svar, "calculate variables") || svar == "calcvar") SET_BOOL_E(settings->evalops.calculate_variables)
	else if(equalsIgnoreCase(svar, "calculate functions") || svar == "calcfunc") SET_BOOL_E(settings->evalops.calculate_functions)
	else if(equalsIgnoreCase(svar, "sync units") || svar == "sync") SET_BOOL_E(settings->evalops.sync_units)
	else if(equalsIgnoreCase(svar, "temperature calculation") || svar == "temp")  {
		int v = -1;
		if(equalsIgnoreCase(svalue, "relative")) v = TEMPERATURE_CALCULATION_RELATIVE;
		else if(equalsIgnoreCase(svalue, "hybrid")) v = TEMPERATURE_CALCULATION_HYBRID;
		else if(equalsIgnoreCase(svalue, "absolute")) v = TEMPERATURE_CALCULATION_ABSOLUTE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			CALCULATOR->setTemperatureCalculationMode((TemperatureCalculationMode) v);
			settings->tc_set = true;
			expressionCalculationUpdated();
		}
	} else if(svar == "sinc")  {
		int v = -1;
		if(equalsIgnoreCase(svalue, "unnormalized")) v = 0;
		else if(equalsIgnoreCase(svalue, "normalized")) v = 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			if(v == 0) CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "");
			else CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "pi");
			settings->sinc_set = true;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "round to even") || svar == "rndeven") {
		bool b = (settings->printops.rounding == ROUNDING_HALF_TO_EVEN);
		SET_BOOL(b)
		if(b != (settings->printops.rounding == ROUNDING_HALF_TO_EVEN)) {
			settings->printops.rounding = b ? ROUNDING_HALF_TO_EVEN : ROUNDING_HALF_AWAY_FROM_ZERO;
			resultFormatUpdated();
		}
	} else if(equalsIgnoreCase(svar, "rounding")) {
		int v = -1;
		if(equalsIgnoreCase(svalue, "even") || equalsIgnoreCase(svalue, "round to even") || equalsIgnoreCase(svalue, "half to even")) v = ROUNDING_HALF_TO_EVEN;
		else if(equalsIgnoreCase(svalue, "standard") || equalsIgnoreCase(svalue, "half away from zero")) v = ROUNDING_HALF_AWAY_FROM_ZERO;
		else if(equalsIgnoreCase(svalue, "truncate") || equalsIgnoreCase(svalue, "toward zero")) v = ROUNDING_TOWARD_ZERO;
		else if(equalsIgnoreCase(svalue, "half to odd")) v = ROUNDING_HALF_TO_ODD;
		else if(equalsIgnoreCase(svalue, "half toward zero")) v = ROUNDING_HALF_TOWARD_ZERO;
		else if(equalsIgnoreCase(svalue, "half random")) v = ROUNDING_HALF_RANDOM;
		else if(equalsIgnoreCase(svalue, "half up")) v = ROUNDING_HALF_UP;
		else if(equalsIgnoreCase(svalue, "half down")) v = ROUNDING_HALF_DOWN;
		else if(equalsIgnoreCase(svalue, "up")) v = ROUNDING_UP;
		else if(equalsIgnoreCase(svalue, "down")) v = ROUNDING_DOWN;
		else if(equalsIgnoreCase(svalue, "away from zero")) v = ROUNDING_AWAY_FROM_ZERO;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v == 2) v = ROUNDING_TOWARD_ZERO;
			else if(v > 2 && v <= ROUNDING_TOWARD_ZERO) v--;
		}
		if(v < ROUNDING_HALF_AWAY_FROM_ZERO || v > ROUNDING_DOWN) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v != settings->printops.rounding) {
			settings->printops.rounding = (RoundingMode) v;
			resultFormatUpdated();
		}
	} else if(equalsIgnoreCase(svar, "rpn syntax") || svar == "rpnsyn") {
		bool b = (settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN);
		SET_BOOL(b)
		if(b != (settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN)) {
			if(b) {
				settings->evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
			} else {
				settings->evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
			}
			expressionFormatUpdated(false);
		}
	} else if(equalsIgnoreCase(svar, "rpn") && svalue.find(" ") == std::string::npos) {
		bool b = settings->rpn_mode;
		SET_BOOL(b)
		if(b != settings->rpn_mode) {
			QAction *w = NULL;
			if(b) w = findChild<QAction*>("action_rpnmode");
			else w = findChild<QAction*>("action_normalmode");
			if(w) w->setChecked(true);
			if(b) rpnModeActivated();
			else normalModeActivated();
		}
	} else if(equalsIgnoreCase(svar, "short multiplication") || svar == "shortmul") SET_BOOL_D(settings->printops.short_multiplication)
	else if(equalsIgnoreCase(svar, "simplified percentage") || svar == "percent") SET_BOOL_PT(settings->simplified_percentage)
	else if(equalsIgnoreCase(svar, "lowercase e") || svar == "lowe") {
		bool b = (settings->printops.exp_display == EXP_LOWERCASE_E);
		SET_BOOL(b)
		if(b != (settings->printops.exp_display == EXP_LOWERCASE_E)) {
			if(b) settings->printops.exp_display = EXP_LOWERCASE_E;
			else settings->printops.exp_display = EXP_UPPERCASE_E;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "lowercase numbers") || svar == "lownum") SET_BOOL_D(settings->printops.lower_case_numbers)
	else if(equalsIgnoreCase(svar, "duodecimal symbols") || svar == "duosyms") SET_BOOL_D(settings->printops.duodecimal_symbols)
	else if(equalsIgnoreCase(svar, "imaginary j") || svar == "imgj") {
		Variable *v_i = CALCULATOR->getVariableById(VARIABLE_ID_I);
		if(v_i) {
			bool b = v_i->hasName("j") > 0;
			SET_BOOL(b)
			if(b) {
				ExpressionName ename = v_i->getName(1);
				ename.name = "j";
				ename.reference = false;
				v_i->addName(ename, 1, true);
				v_i->setChanged(false);
			} else {
				v_i->clearNonReferenceNames();
				v_i->setChanged(false);
			}
			expressionFormatUpdated(false);
		}
	} else if(equalsIgnoreCase(svar, "base display") || svar == "basedisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = BASE_DISPLAY_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "normal")) v = BASE_DISPLAY_NORMAL;
		else if(equalsIgnoreCase(svalue, "alternative")) v = BASE_DISPLAY_ALTERNATIVE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.base_display = (BaseDisplay) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "two's complement") || svar == "twos") SET_BOOL_D(settings->printops.twos_complement)
	else if(equalsIgnoreCase(svar, "hexadecimal two's") || svar == "hextwos") SET_BOOL_D(settings->printops.hexadecimal_twos_complement)
	else if(equalsIgnoreCase(svar, "two's complement input") || svar == "twosin") {
		SET_BOOL_PF(settings->evalops.parse_options.twos_complement)
		if(settings->evalops.parse_options.twos_complement != settings->default_signed) settings->default_signed = -1;
	} else if(equalsIgnoreCase(svar, "hexadecimal two's input") || svar == "hextwosin") SET_BOOL_PF(settings->evalops.parse_options.hexadecimal_twos_complement)
	else if(equalsIgnoreCase(svar, "binary bits") || svar == "bits") {
		int v = -1;
		if(empty_value) {
			v = 0;
		} else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v < 0) v = 0;
		}
		if(v < 0 || v == 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.binary_bits = v;
			settings->evalops.parse_options.binary_bits = v;
			settings->default_bits = -1;
			if(settings->evalops.parse_options.twos_complement || settings->evalops.parse_options.hexadecimal_twos_complement) expressionFormatUpdated(true);
			else resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "digit grouping") || svar =="group") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DIGIT_GROUPING_NONE;
		else if(equalsIgnoreCase(svalue, "none")) v = DIGIT_GROUPING_NONE;
		else if(empty_value || equalsIgnoreCase(svalue, "standard") || equalsIgnoreCase(svalue, "on")) v = DIGIT_GROUPING_STANDARD;
		else if(equalsIgnoreCase(svalue, "locale")) v = DIGIT_GROUPING_LOCALE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < DIGIT_GROUPING_NONE || v > DIGIT_GROUPING_LOCALE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.digit_grouping = (DigitGrouping) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "spell out logical") || svar == "spellout") SET_BOOL_D(settings->printops.spell_out_logical_operators)
	else if((equalsIgnoreCase(svar, "ignore dot") || svar == "nodot") && CALCULATOR->getDecimalPoint() != DOT) {
		SET_BOOL_PF(settings->evalops.parse_options.dot_as_separator)
		settings->dot_question_asked = true;
	} else if((equalsIgnoreCase(svar, "ignore comma") || svar == "nocomma") && CALCULATOR->getDecimalPoint() != COMMA) {
		SET_BOOL(settings->evalops.parse_options.comma_as_separator)
		CALCULATOR->useDecimalPoint(settings->evalops.parse_options.comma_as_separator);
		expressionFormatUpdated(false);
	} else if(equalsIgnoreCase(svar, "decimal comma")) {
		int v = -2;
		if(equalsIgnoreCase(svalue, "off")) v = 0;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = 1;
		else if(equalsIgnoreCase(svalue, "locale")) v = -1;
		else if(svalue.find_first_not_of(SPACES MINUS NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < -1 || v > 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->decimal_comma = v;
			if(settings->decimal_comma > 0) CALCULATOR->useDecimalComma();
			else if(settings->decimal_comma == 0) CALCULATOR->useDecimalPoint(settings->evalops.parse_options.comma_as_separator);
			if(v >= 0) {
				expressionFormatUpdated(false);
				resultDisplayUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "limit implicit multiplication") || svar == "limimpl") {
		int v = s2b(svalue); if(v < 0) {CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str());} else {settings->printops.limit_implicit_multiplication = v; settings->evalops.parse_options.limit_implicit_multiplication = v; expressionFormatUpdated(true);}
	} else if(equalsIgnoreCase(svar, "spacious") || svar == "space") SET_BOOL_D(settings->printops.spacious)
	else if(equalsIgnoreCase(svar, "unicode") || svar == "uni") {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str());
		} else {
			settings->printops.use_unicode_signs = v; resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "units") || svar == "unit") SET_BOOL_PV(settings->evalops.parse_options.units_enabled)
	else if(equalsIgnoreCase(svar, "unknowns") || svar == "unknown") SET_BOOL_PV(settings->evalops.parse_options.unknowns_enabled)
	else if(equalsIgnoreCase(svar, "variables") || svar == "var") SET_BOOL_PV(settings->evalops.parse_options.variables_enabled)
	else if(equalsIgnoreCase(svar, "abbreviations") || svar == "abbr" || svar == "abbrev") SET_BOOL_D(settings->printops.abbreviate_names)
	else if(equalsIgnoreCase(svar, "show ending zeroes") || svar == "zeroes") SET_BOOL_D(settings->printops.show_ending_zeroes)
	else if(equalsIgnoreCase(svar, "repeating decimals") || svar == "repdeci") SET_BOOL_D(settings->printops.indicate_infinite_series)
	else if(equalsIgnoreCase(svar, "angle unit") || svar == "angle") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rad") || equalsIgnoreCase(svalue, "radians")) v = ANGLE_UNIT_RADIANS;
		else if(equalsIgnoreCase(svalue, "deg") || equalsIgnoreCase(svalue, "degrees")) v = ANGLE_UNIT_DEGREES;
		else if(equalsIgnoreCase(svalue, "gra") || equalsIgnoreCase(svalue, "gradians")) v = ANGLE_UNIT_GRADIANS;
		else if(equalsIgnoreCase(svalue, "none")) v = ANGLE_UNIT_NONE;
		else if(equalsIgnoreCase(svalue, "custom")) v = ANGLE_UNIT_CUSTOM;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		} else {
			Unit *u = CALCULATOR->getActiveUnit(svalue);
			if(u && u->baseUnit() == CALCULATOR->getRadUnit() && u->baseExponent() == 1 && u->isActive() && u->isRegistered() && !u->isHidden()) {
				if(u == CALCULATOR->getRadUnit()) v = ANGLE_UNIT_RADIANS;
				else if(u == CALCULATOR->getGraUnit()) v = ANGLE_UNIT_GRADIANS;
				else if(u == CALCULATOR->getDegUnit()) v = ANGLE_UNIT_DEGREES;
				else {v = ANGLE_UNIT_CUSTOM; CALCULATOR->setCustomAngleUnit(u);}
			}
		}
		if(v < 0 || v > 4) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()) {
			CALCULATOR->error(true, "Please specify a custom angle unit as argument (e.g. set angle arcsec).", NULL);
		} else {
			QAction *w;
			if(v == ANGLE_UNIT_CUSTOM) {
				w = findChild<QAction*>("action_angle_unit" + QString::fromStdString(CALCULATOR->customAngleUnit()->referenceName()));
			} else {
				w = find_child_data(this, "group_angleunit", v);
			}
			if(w) w->setChecked(true);
			settings->evalops.parse_options.angle_unit = (AngleUnit) v;
			expressionFormatUpdated(true);
		}
	} else if(equalsIgnoreCase(svar, "caret as xor") || equalsIgnoreCase(svar, "xor^")) SET_BOOL_PT(settings->caret_as_xor)
	else if(equalsIgnoreCase(svar, "concise uncertainty") || equalsIgnoreCase(svar, "concise")) {
		bool b = CALCULATOR->conciseUncertaintyInputEnabled();
		SET_BOOL(b)
		if(b != CALCULATOR->conciseUncertaintyInputEnabled()) {
			CALCULATOR->setConciseUncertaintyInputEnabled(b);
			expressionFormatUpdated(false);
		}
	} else if(equalsIgnoreCase(svar, "parsing mode") || svar == "parse" || svar == "syntax") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = PARSING_MODE_ADAPTIVE;
		else if(equalsIgnoreCase(svalue, "implicit first")) v = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
		else if(equalsIgnoreCase(svalue, "conventional")) v = PARSING_MODE_CONVENTIONAL;
		else if(equalsIgnoreCase(svalue, "chain")) v = PARSING_MODE_CHAIN;
		else if(equalsIgnoreCase(svalue, "rpn")) v = PARSING_MODE_RPN;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < PARSING_MODE_ADAPTIVE || v > PARSING_MODE_RPN) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.parse_options.parsing_mode = (ParsingMode) v;
			settings->implicit_question_asked = (settings->evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || settings->evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST);
			expressionFormatUpdated(true);
		}
	} else if(equalsIgnoreCase(svar, "update exchange rates") || svar == "upxrates") {
		int v = -2;
		if(equalsIgnoreCase(svalue, "never")) {
			settings->auto_update_exchange_rates = 0;
		} else if(equalsIgnoreCase(svalue, "ask")) {
			CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
		} else {
			v = s2i(svalue);
			if(empty_value) v = 7;
			if(v < 0) CALCULATOR->error(true, "Unsupported value: %s.", svalue.c_str(), NULL);
			else settings->auto_update_exchange_rates = v;
		}
	} else if(equalsIgnoreCase(svar, "multiplication sign") || svar == "mulsign") {
		int v = -1;
		if(svalue == SIGN_MULTIDOT || svalue == ".") v = MULTIPLICATION_SIGN_DOT;
		else if(svalue == SIGN_MIDDLEDOT) v = MULTIPLICATION_SIGN_ALTDOT;
		else if(svalue == SIGN_MULTIPLICATION || svalue == "x") v = MULTIPLICATION_SIGN_X;
		else if(svalue == "*") v = MULTIPLICATION_SIGN_ASTERISK;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < MULTIPLICATION_SIGN_ASTERISK || v > MULTIPLICATION_SIGN_ALTDOT) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.multiplication_sign = (MultiplicationSign) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "division sign") || svar == "divsign") {
		int v = -1;
		if(svalue == SIGN_DIVISION_SLASH) v = DIVISION_SIGN_DIVISION_SLASH;
		else if(svalue == SIGN_DIVISION) v = DIVISION_SIGN_DIVISION;
		else if(svalue == "/") v = DIVISION_SIGN_SLASH;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.division_sign = (DivisionSign) v;
			resultDisplayUpdated();
		}
	} else if(equalsIgnoreCase(svar, "approximation") || svar == "appr" || svar == "approx") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "exact")) v = APPROXIMATION_EXACT;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(equalsIgnoreCase(svalue, "dual")) v = APPROXIMATION_APPROXIMATE + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "try exact") || svalue == "try") v = APPROXIMATION_TRY_EXACT;
		else if(equalsIgnoreCase(svalue, "approximate") || svalue == "approx") v = APPROXIMATION_APPROXIMATE;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v > APPROXIMATION_APPROXIMATE + 1) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			QAction *w = NULL;
			if(v < 0) w = findChild<QAction*>("action_autoappr");
			else if(v == APPROXIMATION_APPROXIMATE + 1) w = findChild<QAction*>("action_dualappr");
			else if(v == APPROXIMATION_EXACT) w = findChild<QAction*>("action_exact");
			else if(v == APPROXIMATION_TRY_EXACT) w = findChild<QAction*>("action_approximate");
			else if(v == APPROXIMATION_APPROXIMATE) w = findChild<QAction*>("action_approximate");
			if(w) {
				w->setChecked(true);
			}
			if(v < 0) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				settings->dual_approximation = -1;
			} else if(v == APPROXIMATION_APPROXIMATE + 1) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				settings->dual_approximation = 1;
			} else {
				settings->evalops.approximation = (ApproximationMode) v;
				settings->dual_approximation = 0;
			}
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "interval calculation") || svar == "ic" || equalsIgnoreCase(svar, "uncertainty propagation") || svar == "up") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "variance formula") || equalsIgnoreCase(svalue, "variance")) v = INTERVAL_CALCULATION_VARIANCE_FORMULA;
		else if(equalsIgnoreCase(svalue, "interval arithmetic") || svalue == "iv") v = INTERVAL_CALCULATION_INTERVAL_ARITHMETIC;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < INTERVAL_CALCULATION_NONE || v > INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.interval_calculation = (IntervalCalculation) v;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "autoconversion") || svar == "conv") {
		int v = -1;
		MixedUnitsConversion muc = MIXED_UNITS_CONVERSION_DEFAULT;
		if(equalsIgnoreCase(svalue, "none")) {v = POST_CONVERSION_NONE;  muc = MIXED_UNITS_CONVERSION_NONE;}
		else if(equalsIgnoreCase(svalue, "best")) v = POST_CONVERSION_OPTIMAL_SI;
		else if(equalsIgnoreCase(svalue, "optimalsi") || svalue == "si") v = POST_CONVERSION_OPTIMAL_SI;
		else if(empty_value || equalsIgnoreCase(svalue, "optimal")) v = POST_CONVERSION_OPTIMAL;
		else if(equalsIgnoreCase(svalue, "base")) v = POST_CONVERSION_BASE;
		else if(equalsIgnoreCase(svalue, "mixed")) v = POST_CONVERSION_OPTIMAL + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v == 1) v = 3;
			else if(v == 3) v = 1;
		}
		if(v == POST_CONVERSION_OPTIMAL + 1) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_DEFAULT;
		} else if(v == 0) {
			v = POST_CONVERSION_NONE;
			muc = MIXED_UNITS_CONVERSION_NONE;
		}
		if(v < 0 || v > POST_CONVERSION_OPTIMAL) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.auto_post_conversion = (AutoPostConversion) v;
			settings->evalops.mixed_units_conversion = muc;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "currency conversion") || svar == "curconv") SET_BOOL_E(settings->evalops.local_currency_conversion)
	else if(equalsIgnoreCase(svar, "algebra mode") || svar == "alg") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "none")) v = STRUCTURING_NONE;
		else if(equalsIgnoreCase(svalue, "simplify") || equalsIgnoreCase(svalue, "expand")) v = STRUCTURING_SIMPLIFY;
		else if(equalsIgnoreCase(svalue, "factorize") || svalue == "factor") v = STRUCTURING_FACTORIZE;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > STRUCTURING_FACTORIZE) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.structuring = (StructuringMode) v;
			settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "exact")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			setOption("approx exact");
		}
	} else if(equalsIgnoreCase(svar, "ignore locale")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v != settings->ignore_locale) {
			if(v > 0) {
				settings->ignore_locale = true;
			} else {
				settings->ignore_locale = false;
			}
			CALCULATOR->error(false, "Please restart the program for the change to take effect.", NULL);
		}
	} else if(equalsIgnoreCase(svar, "save mode")) {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v > 0) {
			settings->save_mode_on_exit = true;
		} else {
			settings->save_mode_on_exit = false;
		}
	} else if(equalsIgnoreCase(svar, "save definitions") || svar == "save defs") {
		int v = s2b(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else if(v > 0) {
			settings->save_defs_on_exit = true;
		} else {
			settings->save_defs_on_exit = false;
		}
	} else if(equalsIgnoreCase(svar, "scientific notation") || svar == "exp mode" || svar == "exp" || svar == "exp display" || svar == "edisp") {
		int v = -1;
		bool valid = true;
		bool display = (svar == "exp display" || svar == "edisp");
		if(!display && equalsIgnoreCase(svalue, "off")) v = EXP_NONE;
		else if(!display && equalsIgnoreCase(svalue, "auto")) v = EXP_PRECISION;
		else if(!display && equalsIgnoreCase(svalue, "pure")) v = EXP_PURE;
		else if(!display && (empty_value || svalue == "sci" || equalsIgnoreCase(svalue, "scientific"))) v = EXP_SCIENTIFIC;
		else if(!display && (svalue == "eng" || equalsIgnoreCase(svalue, "engineering"))) v = EXP_BASE_3;
		else if(svalue == "E" || (display && empty_value && settings->printops.exp_display == EXP_POWER_OF_10)) {v = EXP_UPPERCASE_E; display = true;}
		else if(svalue == "e") {v = EXP_LOWERCASE_E; display = true;}
		else if((display && svalue == "10") || (display && empty_value && settings->printops.exp_display != EXP_POWER_OF_10) || svalue == "pow" || svalue == "pow10" || equalsIgnoreCase(svalue, "power") || equalsIgnoreCase(svalue, "power of 10")) {
			v = EXP_POWER_OF_10;
			display = 1;
		} else if(svalue.find_first_not_of(SPACES NUMBERS MINUS) == std::string::npos) {
			v = s2i(svalue);
			if(display) v++;
		} else valid = false;
		if(display && valid && (v >= EXP_UPPERCASE_E && v <= EXP_POWER_OF_10)) {
			settings->printops.exp_display = (ExpDisplay) v;
			resultDisplayUpdated();
		} else if(!display && valid) {
			settings->printops.min_exp = v;
			QAction *action = find_child_data(this, "group_general", v);
			if(action) {
				action->setChecked(true);
			}
			resultFormatUpdated();
		} else {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		}
	} else if(equalsIgnoreCase(svar, "precision") || svar == "prec") {
		int v = 0;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		if(v < 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			QSpinBox *w = findChild<QSpinBox*>("spinbox_precision");
			if(w) {
				w->blockSignals(true);
				w->setValue(v);
				w->blockSignals(false);
			}
			CALCULATOR->setPrecision(v);
			settings->previous_precision = 0;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "interval display") || svar == "ivdisp") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "adaptive")) v = 0;
		else if(equalsIgnoreCase(svalue, "significant")) v = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS + 1;
		else if(equalsIgnoreCase(svalue, "interval")) v = INTERVAL_DISPLAY_INTERVAL + 1;
		else if(empty_value || equalsIgnoreCase(svalue, "plusminus")) v = INTERVAL_DISPLAY_PLUSMINUS + 1;
		else if(equalsIgnoreCase(svalue, "midpoint")) v = INTERVAL_DISPLAY_MIDPOINT + 1;
		else if(equalsIgnoreCase(svalue, "upper")) v = INTERVAL_DISPLAY_UPPER + 1;
		else if(equalsIgnoreCase(svalue, "lower")) v = INTERVAL_DISPLAY_LOWER + 1;
		else if(equalsIgnoreCase(svalue, "concise")) v = INTERVAL_DISPLAY_CONCISE + 1;
		else if(equalsIgnoreCase(svalue, "relative")) v = INTERVAL_DISPLAY_RELATIVE + 1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v == 0) {
			settings->adaptive_interval_display = true;
			settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
			resultFormatUpdated();
		} else {
			v--;
			if(v < INTERVAL_DISPLAY_SIGNIFICANT_DIGITS || v > INTERVAL_DISPLAY_RELATIVE) {
				CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
			} else {
				settings->adaptive_interval_display = false;
				settings->printops.interval_display = (IntervalDisplay) v;
				resultFormatUpdated();
			}
		}
	} else if(equalsIgnoreCase(svar, "interval arithmetic") || svar == "ia" || svar == "interval") {
		bool b = CALCULATOR->usesIntervalArithmetic();
		SET_BOOL(b)
		if(b != CALCULATOR->usesIntervalArithmetic()) {
			CALCULATOR->useIntervalArithmetic(b);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "variable units") || svar == "varunits") {
		bool b = CALCULATOR->variableUnitsEnabled();
		SET_BOOL(b)
		if(b != CALCULATOR->variableUnitsEnabled()) {
			CALCULATOR->setVariableUnitsEnabled(b);
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "color")) {
		int v = -1;
		if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		if(v < 0) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->colorize_result = v;
		}
	} else if(equalsIgnoreCase(svar, "max decimals") || svar == "maxdeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		QSpinBox *w = findChild<QSpinBox*>("spinbox_maxdecimals");
		if(w) {
			w->blockSignals(true);
			w->setValue(v < 0 ? -1 : v);
			w->blockSignals(false);
		}
		settings->printops.use_max_decimals = (v >= 0);
		settings->printops.max_decimals = v;
		resultFormatUpdated();
	} else if(equalsIgnoreCase(svar, "min decimals") || svar == "mindeci") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = -1;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) v = s2i(svalue);
		QSpinBox *w = findChild<QSpinBox*>("spinbox_mindecimals");
		if(w) {
			w->blockSignals(true);
			w->setValue(v < 0 ? 0 : v);
			w->blockSignals(false);
		}
		settings->printops.use_min_decimals = (v > 0);
		settings->printops.min_decimals = v;
		resultFormatUpdated();
	} else if(equalsIgnoreCase(svar, "fractions") || svar == "fr") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = FRACTION_DECIMAL;
		else if(equalsIgnoreCase(svalue, "exact")) v = FRACTION_DECIMAL_EXACT;
		else if(empty_value || equalsIgnoreCase(svalue, "on")) v = FRACTION_FRACTIONAL;
		else if(equalsIgnoreCase(svalue, "combined") || equalsIgnoreCase(svalue, "mixed")) v = FRACTION_COMBINED;
		else if(equalsIgnoreCase(svalue, "long")) v = FRACTION_PERMYRIAD + 1;
		else if(equalsIgnoreCase(svalue, "dual")) v = FRACTION_PERMYRIAD + 2;
		else if(equalsIgnoreCase(svalue, "auto")) v = -1;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
			if(v == FRACTION_COMBINED + 1) v = FRACTION_PERMYRIAD + 1;
			else if(v == FRACTION_COMBINED + 2) v = FRACTION_PERMYRIAD + 2;
			else if(v > FRACTION_COMBINED + 2) v -= 2;
		} else {
			Variable *var = CALCULATOR->getActiveVariable(svalue);
			if(var && var->referenceName() == "percent") {
				v = FRACTION_PERCENT;
			} else if(var && var->referenceName() == "permille") {
				v = FRACTION_PERMILLE;
			} else if(var && var->referenceName() == "permyriad") {
				v = FRACTION_PERMYRIAD;
			} else {
				int tofr = 0;
				long int fden = get_fixed_denominator_qt(settings->unlocalizeExpression(svalue), tofr, QString(), true);
				if(fden != 0) {
					if(tofr == 1) v = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
					else v = FRACTION_COMBINED_FIXED_DENOMINATOR;
					if(fden > 0) CALCULATOR->setFixedDenominator(fden);
				}
			}
		}
		if(v > FRACTION_PERMYRIAD + 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->printops.restrict_fraction_length = (v == FRACTION_FRACTIONAL || v == FRACTION_COMBINED);
			if(v < 0) settings->dual_fraction = -1;
			else if(v == FRACTION_PERMYRIAD + 2) settings->dual_fraction = 1;
			else settings->dual_fraction = 0;
			if(v == FRACTION_PERMYRIAD + 1) v = FRACTION_FRACTIONAL;
			else if(v < 0 || v == FRACTION_PERMYRIAD + 2) v = FRACTION_DECIMAL;
			settings->printops.number_fraction_format = (NumberFractionFormat) v;
			resultFormatUpdated();
		}
	} else if(equalsIgnoreCase(svar, "complex form") || svar == "cplxform") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "rectangular") || equalsIgnoreCase(svalue, "cartesian") || svalue == "rect") v = COMPLEX_NUMBER_FORM_RECTANGULAR;
		else if(equalsIgnoreCase(svalue, "exponential") || svalue == "exp") v = COMPLEX_NUMBER_FORM_EXPONENTIAL;
		else if(equalsIgnoreCase(svalue, "polar")) v = COMPLEX_NUMBER_FORM_POLAR;
		else if(equalsIgnoreCase(svalue, "angle") || equalsIgnoreCase(svalue, "phasor")) v = COMPLEX_NUMBER_FORM_CIS + 1;
		else if(svar == "cis") v = COMPLEX_NUMBER_FORM_CIS;
		else if(!empty_value && svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 4) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->complex_angle_form = (v > 3);
			if(v == 4) v--;
			settings->evalops.complex_number_form = (ComplexNumberForm) v;
			expressionCalculationUpdated();
		}
	} else if(equalsIgnoreCase(svar, "read precision") || svar == "readprec") {
		int v = -1;
		if(equalsIgnoreCase(svalue, "off")) v = DONT_READ_PRECISION;
		else if(equalsIgnoreCase(svalue, "always")) v = ALWAYS_READ_PRECISION;
		else if(empty_value || equalsIgnoreCase(svalue, "when decimals") || equalsIgnoreCase(svalue, "on")) v = READ_PRECISION_WHEN_DECIMALS;
		else if(svalue.find_first_not_of(SPACES NUMBERS) == std::string::npos) {
			v = s2i(svalue);
		}
		if(v < 0 || v > 2) {
			CALCULATOR->error(true, "Illegal value: %s.", svalue.c_str(), NULL);
		} else {
			settings->evalops.parse_options.read_precision = (ReadPrecisionMode) v;
			expressionFormatUpdated(true);
		}
	} else {
		if(i_underscore == std::string::npos) {
			if(index != std::string::npos) {
				if((index = svar.find_last_of(SPACES)) != std::string::npos) {
					svar = svar.substr(0, index);
					remove_blank_ends(svar);
					str = str.substr(index + 1);
					remove_blank_ends(str);
					svalue = str;
					gsub("_", " ", svar);
					goto set_option_place;
				}
			}
			if(!empty_value && !svalue.empty()) {
				svar += " ";
				svar += svalue;
				svalue = "1";
				empty_value = true;
				goto set_option_place;
			}
		}
		CALCULATOR->error(true, "Unrecognized option: %s.", svar.c_str(), NULL);
	}
}

void QalculateWindow::calculateRPN(int op) {
	if(expressionEdit->expressionHasChanged()) {
		if(!expressionEdit->toPlainText().trimmed().isEmpty()) {
			calculateExpression(true);
		}
	}
	calculateExpression(true, true, (MathOperation) op, NULL);
}
void QalculateWindow::calculateRPN(MathFunction *f) {
	if(expressionEdit->expressionHasChanged()) {
		if(!expressionEdit->toPlainText().trimmed().isEmpty()) {
			calculateExpression(true);
		}
	}
	calculateExpression(true, true, OPERATION_ADD, f);
}
void QalculateWindow::RPNRegisterAdded(std::string text, int index) {
	rpnView->insertRow(index);
	QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(unhtmlize(text)));
	item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
	rpnView->setItem(index, 0, item);
	rpnCopyAction->setEnabled(true);
	rpnDeleteAction->setEnabled(true);
	rpnClearAction->setEnabled(true);
	if(CALCULATOR->RPNStackSize() >= 2) {
		rpnUpAction->setEnabled(true);
		rpnDownAction->setEnabled(true);
		rpnSwapAction->setEnabled(true);
	}
}
void QalculateWindow::RPNRegisterRemoved(int index) {
	rpnView->removeRow(index);
	if(CALCULATOR->RPNStackSize() == 0) {
		rpnCopyAction->setEnabled(false);
		rpnDeleteAction->setEnabled(false);
		rpnClearAction->setEnabled(false);
	}
	if(CALCULATOR->RPNStackSize() < 2) {
		rpnUpAction->setEnabled(false);
		rpnDownAction->setEnabled(false);
		rpnSwapAction->setEnabled(false);
	}
}
void QalculateWindow::RPNRegisterChanged(std::string text, int index) {
	QTableWidgetItem *item = rpnView->item(index, 0);
	if(item) {
		item->setText(QString::fromStdString(unhtmlize(text)));
	} else {
		item = new QTableWidgetItem(QString::fromStdString(unhtmlize(text)));
		item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		rpnView->setItem(index, 0, item);
	}
}

void QalculateWindow::approximateResult() {
	ApproximationMode appr_bak = settings->evalops.approximation;
	NumberFractionFormat frac_bak = settings->printops.number_fraction_format;
	int dappr_bak = settings->dual_approximation;
	int dfrac_bak = settings->dual_fraction;
	settings->dual_approximation = 0;
	settings->dual_fraction = 0;
	settings->printops.number_fraction_format = FRACTION_DECIMAL;
	settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
	executeCommand(COMMAND_EVAL);
	settings->evalops.approximation = appr_bak;
	settings->printops.number_fraction_format = frac_bak;
	settings->dual_approximation = dappr_bak;
	settings->dual_fraction = dfrac_bak;
}

void QalculateWindow::calculateExpression(bool force, bool do_mathoperation, MathOperation op, MathFunction *f, bool do_stack, size_t stack_index, std::string execute_str, std::string str, bool check_exrates) {

	workspace_changed = true;

	std::string saved_execute_str = execute_str;

	if(b_busy) return;

	if(autoCalculateTimer) autoCalculateTimer->stop();

	expressionEdit->hideCompletion();

	b_busy++;

	bool do_factors = false, do_pfe = false, do_expand = false, do_bases = false, do_calendars = false;
	if(do_stack && !settings->rpn_mode) do_stack = false;
	if(do_stack && do_mathoperation && f && stack_index == 0) do_stack = false;
	if(!do_stack) stack_index = 0;

	if(execute_str.empty()) {
		to_fraction = 0; to_fixed_fraction = 0; to_prefix = 0; to_base = 0; to_bits = 0; to_nbase.clear(); to_caf = -1;
	}
	bool current_expr = false;
	if(str.empty() && !do_mathoperation) {
		if(do_stack) {
			QTableWidgetItem *item = rpnView->item(stack_index, 0);
			if(!item) {b_busy--; return;}
			str = item->text().toStdString();
		} else {
			current_expr = true;
			str = expressionEdit->toPlainText().toStdString();
			if(!force && (expressionEdit->expressionHasChanged() || str.find_first_not_of(SPACES) == std::string::npos)) {
				b_busy--;
				return;
			}
			expressionEdit->setExpressionHasChanged(false);
			if(!do_mathoperation && !str.empty() && !block_expression_history) expressionEdit->addToHistory();
			askDot(str);
		}
	}

	std::string to_str, str_conv;

	if(execute_str.empty()) {
		bool double_tag = false;
		to_str = CALCULATOR->parseComments(str, settings->evalops.parse_options, &double_tag);
		if(!to_str.empty()) {
			if(str.empty()) {
				if(!double_tag) {
					expressionEdit->clear();
					CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
					historyView->addMessages();
					b_busy--;
					return;
				}
				execute_str = CALCULATOR->getFunctionById(FUNCTION_ID_MESSAGE)->referenceName();
				execute_str += "(";
				if(to_str.find("\"") == std::string::npos) {execute_str += "\""; execute_str += to_str; execute_str += "\"";}
				else if(to_str.find("\'") == std::string::npos) {execute_str += "\'"; execute_str += to_str; execute_str += "\'";}
				else execute_str += to_str;
				execute_str += ")";
			} else {
				CALCULATOR->message(MESSAGE_INFORMATION, to_str.c_str(), NULL);
			}
		}
		// qalc command
		bool b_command = false;
		if(str[0] == '/' && str.length() > 1) {
			size_t i = str.find_first_not_of(SPACES, 1);
			if(i != std::string::npos && (signed char) str[i] > 0 && is_not_in(NUMBER_ELEMENTS OPERATORS, str[i])) {
				b_command = true;
			}
		}
		if(b_command) {
			str.erase(0, 1);
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			std::string scom;
			if(ispace == std::string::npos) {
				scom = "";
			} else {
				scom = str.substr(1, ispace);
			}
			if(equalsIgnoreCase(scom, "convert") || equalsIgnoreCase(scom, "to")) {
				str = std::string("to") + str.substr(ispace, slen - ispace);
				b_command = false;
			} else if((str.length() > 2 && str[0] == '-' && str[1] == '>') || (str.length() > 3 && str[0] == '\xe2' && ((str[1] == '\x86' && str[2] == '\x92') || (str[1] == '\x9e' && (unsigned char) str[2] >= 148 && (unsigned char) str[3] <= 191)))) {
				b_command = false;
			} else if(str == "M+" || str == "M-" || str == "M−" || str == "MS" || str == "MC") {
				b_command = false;
			}
		}
		if(b_command) {
			remove_blank_ends(str);
			size_t slen = str.length();
			size_t ispace = str.find_first_of(SPACES);
			std::string scom;
			if(ispace == std::string::npos) {
				scom = "";
			} else {
				scom = str.substr(0, ispace);
			}
			b_busy--;
			if(equalsIgnoreCase(scom, "set")) {
				if(current_expr) setPreviousExpression();
				str = str.substr(ispace + 1, slen - (ispace + 1));
				setOption(str);
			} else if(equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(equalsIgnoreCase(str, "mode")) {
					settings->window_state = saveState();
					if(height() != DEFAULT_HEIGHT || width() != DEFAULT_WIDTH) settings->window_geometry = saveGeometry();
					else settings->window_geometry = QByteArray();
					settings->splitter_state = ehSplitter->saveState();
					settings->show_bases = basesDock->isVisible();
					settings->savePreferences();
					if(current_expr) expressionEdit->clear();
				} else if(equalsIgnoreCase(str, "definitions")) {
					if(!CALCULATOR->saveDefinitions()) {
						QMessageBox::critical(this, tr("Error"), tr("Couldn't write definitions"), QMessageBox::Ok);
					} else {
						if(current_expr) expressionEdit->clear();
					}
				} else {
					std::string name = str, cat, title;
					if(str[0] == '\"') {
						size_t i = str.find('\"', 1);
						if(i != std::string::npos) {
							name = str.substr(1, i - 1);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
					} else {
						size_t i = str.find_first_of(SPACES, 1);
						if(i != std::string::npos) {
							name = str.substr(0, i);
							str = str.substr(i + 1, str.length() - (i + 1));
							remove_blank_ends(str);
						} else {
							str = "";
						}
						bool catset = false;
						if(str.empty()) {
							cat = CALCULATOR->temporaryCategory();
						} else {
							if(str[0] == '\"') {
								size_t i = str.find('\"', 1);
								if(i != std::string::npos) {
									cat = str.substr(1, i - 1);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							} else {
								size_t i = str.find_first_of(SPACES, 1);
								if(i != std::string::npos) {
									cat = str.substr(0, i);
									title = str.substr(i + 1, str.length() - (i + 1));
									remove_blank_ends(title);
								}
							}
							catset = true;
						}
						bool b = true;
						if(!CALCULATOR->variableNameIsValid(name)) {
							CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
							b = false;
						}
						Variable *v = NULL;
						if(b) v = CALCULATOR->getActiveVariable(name, true);
						if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
							CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
							b = false;
						}
						if(b) {
							if(v && v->isLocal() && v->isKnown()) {
								if(catset) v->setCategory(cat);
								if(!title.empty()) v->setTitle(title);
								((KnownVariable*) v)->set(*mstruct);
								if(v->countNames() == 0) {
									ExpressionName ename(name);
									ename.reference = true;
									v->setName(ename, 1);
								} else {
									v->setName(name, 1);
								}
							} else {
								CALCULATOR->addVariable(new KnownVariable(cat, name, *mstruct, title));
							}
							expressionEdit->updateCompletion();
							if(variablesDialog) variablesDialog->updateVariables();
							if(unitsDialog) unitsDialog->updateUnits();
							if(current_expr) expressionEdit->clear();
						}
					}
				}
			} else if(equalsIgnoreCase(scom, "variable")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				std::string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != std::string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != std::string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->variableNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				Variable *v = NULL;
				if(b) v = CALCULATOR->getActiveVariable(name, true);
				if(b && ((!v && CALCULATOR->variableNameTaken(name)) || (v && (!v->isKnown() || !v->isLocal())))) {
					CALCULATOR->error(true, "A unit or variable with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(v && v->isLocal() && v->isKnown()) {
						((KnownVariable*) v)->set(expr);
						if(v->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							v->setName(ename, 1);
						} else {
							v->setName(name, 1);
						}
					} else {
						CALCULATOR->addVariable(new KnownVariable("", name, expr));
					}
					expressionEdit->updateCompletion();
					if(variablesDialog) variablesDialog->updateVariables();
					if(unitsDialog) unitsDialog->updateUnits();
					if(current_expr) expressionEdit->clear();
				}
			} else if(equalsIgnoreCase(scom, "function")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				std::string name = str, expr;
				if(str[0] == '\"') {
					size_t i = str.find('\"', 1);
					if(i != std::string::npos) {
						name = str.substr(1, i - 1);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				} else {
					size_t i = str.find_first_of(SPACES, 1);
					if(i != std::string::npos) {
						name = str.substr(0, i);
						str = str.substr(i + 1, str.length() - (i + 1));
						remove_blank_ends(str);
					} else {
						str = "";
					}
				}
				if(str.length() >= 2 && str[0] == '\"' && str[str.length() - 1] == '\"') str = str.substr(1, str.length() - 2);
				expr = str;
				bool b = true;
				if(!CALCULATOR->functionNameIsValid(name)) {
					CALCULATOR->error(true, "Illegal name: %s.", name.c_str(), NULL);
					b = false;
				}
				MathFunction *f = CALCULATOR->getActiveFunction(name, true);
				if(b && ((!f && CALCULATOR->functionNameTaken(name)) || (f && (!f->isLocal() || f->subtype() != SUBTYPE_USER_FUNCTION)))) {
					CALCULATOR->error(true, "A function with the same name (%s) already exists.", name.c_str(), NULL);
					b = false;
				}
				if(b) {
					if(expr.find("\\") == std::string::npos) {
						gsub("x", "\\x", expr);
						gsub("y", "\\y", expr);
						gsub("z", "\\z", expr);
					}
					if(f && f->isLocal() && f->subtype() == SUBTYPE_USER_FUNCTION) {
						((UserFunction*) f)->setFormula(expr);
						if(f->countNames() == 0) {
							ExpressionName ename(name);
							ename.reference = true;
							f->setName(ename, 1);
						} else {
							f->setName(name, 1);
						}
					} else {
						CALCULATOR->addFunction(new UserFunction("", name, expr));
					}
					expressionEdit->updateCompletion();
					if(functionsDialog) functionsDialog->updateFunctions();
					if(current_expr) expressionEdit->clear();
				}
			} else if(equalsIgnoreCase(scom, "delete")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				Variable *v = CALCULATOR->getActiveVariable(str);
				if(v && v->isLocal()) {
					v->destroy();
					expressionEdit->updateCompletion();
					if(variablesDialog) variablesDialog->updateVariables();
					if(unitsDialog) unitsDialog->updateUnits();
					if(current_expr) expressionEdit->clear();
				} else {
					if(str.length() > 2 && str[str.length() - 2] == '(' && str[str.length() - 1] == ')') str = str.substr(0, str.length() - 2);
					MathFunction *f = CALCULATOR->getActiveFunction(str);
					if(f && f->isLocal()) {
						f->destroy();
						expressionEdit->updateCompletion();
						if(functionsDialog) functionsDialog->updateFunctions();
						if(current_expr) expressionEdit->clear();
					} else {
						CALCULATOR->error(true, "No user-defined variable or function with the specified name (%s) exist.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(scom, "keep") || equalsIgnoreCase(scom, "unkeep")) {
				bool unkeep = equalsIgnoreCase(scom, "unkeep");
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				Variable *v = CALCULATOR->getActiveVariable(str);
				bool b = v && v->isLocal();
				if(b && (v->category() == CALCULATOR->temporaryCategory()) == !unkeep) {
					if(unkeep) v->setCategory(CALCULATOR->temporaryCategory());
					else v->setCategory("");
					expressionEdit->updateCompletion();
					if(variablesDialog) variablesDialog->updateVariables();
					if(unitsDialog) unitsDialog->updateUnits();
					if(current_expr) expressionEdit->clear();
				} else {
					if(str.length() > 2 && str[str.length() - 2] == '(' && str[str.length() - 1] == ')') str = str.substr(0, str.length() - 2);
					MathFunction *f = CALCULATOR->getActiveFunction(str);
					if(f && f->isLocal()) {
						if((f->category() == CALCULATOR->temporaryCategory()) == !unkeep) {
							if(unkeep) f->setCategory(CALCULATOR->temporaryCategory());
							else f->setCategory("");
							expressionEdit->updateCompletion();
							if(functionsDialog) functionsDialog->updateFunctions();
							if(current_expr) expressionEdit->clear();
						}
					} else if(!b) {
						CALCULATOR->error(true, "No user-defined variable or function with the specified name (%s) exist.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(scom, "base")) {
				if(current_expr) setPreviousExpression();
				setOption(str);
			} else if(equalsIgnoreCase(scom, "assume")) {
				if(current_expr) setPreviousExpression();
				std::string str2 = "assumptions ";
				setOption(str2 + str.substr(ispace + 1, slen - (ispace + 1)));
			} else if(equalsIgnoreCase(scom, "rpn")) {
				str = str.substr(ispace + 1, slen - (ispace + 1));
				remove_blank_ends(str);
				if(equalsIgnoreCase(str, "syntax")) {
					settings->evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
					QAction *w = findChild<QAction*>("action_normalmode");
					if(w) w->setChecked(true);
					expressionFormatUpdated(false);
				} else if(equalsIgnoreCase(str, "stack")) {
					if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
						settings->evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
					}
					QAction *w = findChild<QAction*>("action_rpnmode");
					if(w) w->setChecked(true);
				} else {
					int v = s2b(str);
					if(v < 0) {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					} else if(v) {
						QAction *w = findChild<QAction*>("action_rpnmode");
						if(w) w->setChecked(true);
						settings->evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
						expressionFormatUpdated(false);
					} else {
						if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_RPN) {
							settings->evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
						}
						QAction *w = findChild<QAction*>("action_normalmode");
						if(w) w->setChecked(true);
						expressionFormatUpdated(false);
					}
				}
			} else if(equalsIgnoreCase(str, "exrates")) {
				if(current_expr) setPreviousExpression();
				fetchExchangeRates();
			} else if(equalsIgnoreCase(str, "stack")) {
				rpnDock->show();
				rpnDock->raise();
			} else if(equalsIgnoreCase(str, "swap")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					rpnView->selectionModel()->clear();
					registerSwap();
				}
			} else if(equalsIgnoreCase(scom, "swap")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					int index1 = 0, index2 = 0;
					str = str.substr(ispace + 1, slen - (ispace + 1));
					std::string str2 = "";
					remove_blank_ends(str);
					ispace = str.find_first_of(SPACES);
					if(ispace != std::string::npos) {
						str2 = str.substr(ispace + 1, str.length() - (ispace + 1));
						str = str.substr(0, ispace);
						remove_blank_ends(str2);
						remove_blank_ends(str);
					}
					index1 = s2i(str);
					if(str2.empty()) index2 = 1;
					else index2 = s2i(str2);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index2 < 0) index2 = (int) CALCULATOR->RPNStackSize() + 1 + index2;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize() || (!str2.empty() && (index2 <= 0 || index2 > (int) CALCULATOR->RPNStackSize()))) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else if(index2 != 1 && index1 != 1) {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					} else if(index1 != index2) {
						if(index1 == 1) index1 = index2;
						QTableWidgetItem *item = rpnView->item(index1 - 1, 0);
						if(item) {
							rpnView->selectionModel()->clear();
							item->setSelected(true);
							registerSwap();
						}
					}
				}
			} else if(equalsIgnoreCase(scom, "move")) {
				CALCULATOR->error(true, "Unsupported command: %s.", scom.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					rpnView->selectionModel()->clear();
					registerDown();
				}
			} else if(equalsIgnoreCase(scom, "rotate")) {
				if(CALCULATOR->RPNStackSize() > 1) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					if(equalsIgnoreCase(str, "up")) {
						rpnView->selectionModel()->clear();
						registerUp();
					} else if(equalsIgnoreCase(str, "down")) {
						rpnView->selectionModel()->clear();
						registerDown();
					} else {
						CALCULATOR->error(true, "Illegal value: %s.", str.c_str(), NULL);
					}
				}
			} else if(equalsIgnoreCase(str, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					rpnView->selectionModel()->clear();
					copyRegister();
				}
			} else if(equalsIgnoreCase(scom, "copy")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					remove_blank_ends(str);
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						QTableWidgetItem *item = rpnView->item(index1 - 1, 0);
						if(item) {
							rpnView->selectionModel()->clear();
							item->setSelected(true);
							copyRegister();
						}
					}
				}
			} else if(equalsIgnoreCase(str, "clear stack")) {
				if(CALCULATOR->RPNStackSize() > 0) clearStack();
			} else if(equalsIgnoreCase(str, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					rpnView->selectionModel()->clear();
					deleteRegister();
				}
			} else if(equalsIgnoreCase(scom, "pop")) {
				if(CALCULATOR->RPNStackSize() > 0) {
					str = str.substr(ispace + 1, slen - (ispace + 1));
					int index1 = s2i(str);
					if(index1 < 0) index1 = (int) CALCULATOR->RPNStackSize() + 1 + index1;
					if(index1 <= 0 || index1 > (int) CALCULATOR->RPNStackSize()) {
						CALCULATOR->error(true, "Missing stack index: %s.", i2s(index1).c_str(), NULL);
					} else {
						QTableWidgetItem *item = rpnView->item(index1 - 1, 0);
						if(item) {
							rpnView->selectionModel()->clear();
							item->setSelected(true);
							deleteRegister();
						}
					}
				}
			} else if(equalsIgnoreCase(str, "factor")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_FACTORIZE);
			} else if(equalsIgnoreCase(str, "partial fraction")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
			} else if(equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand")) {
				if(current_expr) setPreviousExpression();
				executeCommand(COMMAND_EXPAND);
			} else if(equalsIgnoreCase(str, "exact")) {
				if(current_expr) setPreviousExpression();
				setOption("approx exact");
			} else if(equalsIgnoreCase(str, "approximate") || str == "approx") {
				if(current_expr) setPreviousExpression();
				setOption("approx try exact");
			} else if(equalsIgnoreCase(str, "mode")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(str, "help") || str == "?") {
				help();
				if(current_expr) expressionEdit->clear();
			} else if(equalsIgnoreCase(str, "list")) {
				CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
			} else if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) {
				str = str.substr(ispace + 1);
				remove_blank_ends(str);
				char list_type = 0;
				if(equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find")) {
					size_t i = str.find_first_of(SPACES);
					std::string str1, str2;
					if(i == std::string::npos) {
						str1 = str;
					} else {
						str1 = str.substr(0, i);
						str2 = str.substr(i + 1);
						remove_blank_ends(str2);
					}
					if(equalsIgnoreCase(str1, "currencies")) list_type = 'c';
					else if(equalsIgnoreCase(str1, "functions")) list_type = 'f';
					else if(equalsIgnoreCase(str1, "variables")) list_type = 'v';
					else if(equalsIgnoreCase(str1, "units")) list_type = 'u';
					else if(equalsIgnoreCase(str1, "prefixes")) list_type = 'p';
					if(list_type == 'c') {
						openUnits();
						unitsDialog->selectCategory(CALCULATOR->getUnitById(UNIT_ID_EURO)->category());
						unitsDialog->setSearch(QString::fromStdString(str2));
					} else if(list_type == 'f') {
						openFunctions();
						functionsDialog->selectCategory("All");
						functionsDialog->setSearch(QString::fromStdString(str2));
					} else if(list_type == 'v') {
						openVariables();
						variablesDialog->selectCategory("All");
						variablesDialog->setSearch(QString::fromStdString(str2));
					} else if(list_type == 'u') {
						openUnits();
						unitsDialog->selectCategory("All");
						unitsDialog->setSearch(QString::fromStdString(str2));
					} else if(list_type == 'p') {
						CALCULATOR->error(true, "Unsupported command: %s.", str.c_str(), NULL);
					}
				}
				if(list_type == 0) {
					ExpressionItem *item = CALCULATOR->getActiveExpressionItem(str);
					if(item) {
						if(item->type() == TYPE_UNIT) {
							openUnits();
							unitsDialog->selectCategory("All");
							unitsDialog->setSearch(QString::fromStdString(str));
						} else if(item->type() == TYPE_FUNCTION) {
							openFunctions();
							functionsDialog->selectCategory("All");
							functionsDialog->setSearch(QString::fromStdString(str));
						} else if(item->type() == TYPE_VARIABLE) {
							openVariables();
							variablesDialog->selectCategory("All");
							variablesDialog->setSearch(QString::fromStdString(str));
						}
						if(current_expr) expressionEdit->clear();
					} else {
						CALCULATOR->error(true, "No function, variable, or unit with the specified name (%s) was found.", str.c_str(), NULL);
					}
				} else {
					if(current_expr) expressionEdit->clear();
				}
			} else if(equalsIgnoreCase(str, "clear history")) {
				historyView->editClear();
				expressionEdit->clearHistory();
			} else if(equalsIgnoreCase(str, "clear")) {
				expressionEdit->clear();
			} else if(equalsIgnoreCase(str, "quit") || equalsIgnoreCase(str, "exit")) {
				qApp->closeAllWindows();
				return;
			} else {
				CALCULATOR->error(true, "Unknown command: %s.", str.c_str(), NULL);
			}
			displayMessages();
			return;
		}
	}

	if(execute_str.empty()) {
		if(str == "MC") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMCClicked();
			return;
		} else if(str == "MS") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMSClicked();
			return;
		} else if(str == "M+") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMPlusClicked();
			return;
		} else if(str == "M-" || str == "M−") {
			b_busy--;
			if(current_expr) setPreviousExpression();
			onMMinusClicked();
			return;
		}
	}

	ComplexNumberForm cnf_bak = settings->evalops.complex_number_form;
	ComplexNumberForm cnf = settings->evalops.complex_number_form;
	bool delay_complex = false;
	bool b_units_saved = settings->evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = settings->evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = settings->evalops.mixed_units_conversion;

	had_to_expression = false;
	std::string from_str = str;
	bool last_is_space = !from_str.empty() && is_in(SPACES, from_str[from_str.length() - 1]);
	if(execute_str.empty() && CALCULATOR->separateToExpression(from_str, to_str, settings->evalops, true, !do_stack)) {
		remove_duplicate_blanks(to_str);
		had_to_expression = true;
		std::string str_left;
		std::string to_str1, to_str2;
		bool do_to = false;
		while(true) {
			if(!from_str.empty()) {
				if(last_is_space) to_str += " ";
				CALCULATOR->separateToExpression(to_str, str_left, settings->evalops, true, false);
				remove_blank_ends(to_str);
			} else if(!settings->current_result) {
				b_busy--;
				if(current_expr) setPreviousExpression();
				return;
			}
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != std::string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, tr("hexadecimal").toStdString())) {
				to_base = BASE_HEXADECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, tr("octal").toStdString())) {
				to_base = BASE_OCTAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "dec") || equalsIgnoreCase(to_str, "decimal") || equalsIgnoreCase(to_str, tr("decimal").toStdString())) {
				to_base = BASE_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "duo") || equalsIgnoreCase(to_str, "duodecimal") || equalsIgnoreCase(to_str, tr("duodecimal").toStdString())) {
				to_base = BASE_DUODECIMAL;
				to_duo_syms = false;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "doz") || equalsIgnoreCase(to_str, "dozenal")) {
				to_base = BASE_DUODECIMAL;
				to_duo_syms = true;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, tr("binary").toStdString())) {
				to_base = BASE_BINARY;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, tr("roman").toStdString())) {
				to_base = BASE_ROMAN_NUMERALS;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, tr("bijective").toStdString())) {
				to_base = BASE_BIJECTIVE_26;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bcd")) {
				to_base = BASE_BINARY_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, tr("sexagesimal").toStdString())) {
				to_base = BASE_SEXAGESIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "2")) {
				to_base = BASE_SEXAGESIMAL_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "3")) {
				to_base = BASE_SEXAGESIMAL_3;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "latitude") || equalsIgnoreCase(to_str, tr("latitude").toStdString())) {
				to_base = BASE_LATITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", tr("latitude"), "2")) {
				to_base = BASE_LATITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "longitude") || equalsIgnoreCase(to_str, tr("longitude").toStdString())) {
				to_base = BASE_LONGITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", tr("longitude"), "2")) {
				to_base = BASE_LONGITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
				to_base = BASE_FP32;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
				to_base = BASE_FP64;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
				to_base = BASE_FP16;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp80")) {
				to_base = BASE_FP80;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
				to_base = BASE_FP128;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, tr("time").toStdString())) {
				to_base = BASE_TIME;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "Unicode")) {
				to_base = BASE_UNICODE;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				settings->printops.time_zone = TIME_ZONE_UTC;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_BINARY;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_HEXADECIMAL;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
				to_str = to_str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					CALCULATOR->error(true, tr("Time zone parsing failed.").toUtf8().data(),  NULL);
				}
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = itz;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(to_str == "CET") {
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = 60;
				if(from_str.empty()) {
					b_busy--;
					setResult(NULL, true, false, false); if(current_expr) setPreviousExpression();
					settings->printops.time_zone = TIME_ZONE_LOCAL;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, tr("bases").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					if(current_expr) setPreviousExpression();
					onBasesActivated(true);
					return;
				}
				do_bases = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, tr("calendars").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					if(current_expr) setPreviousExpression();
					openCalendarConversion();
					return;
				}
				do_calendars = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "rectangular") || equalsIgnoreCase(to_str, "cartesian") || equalsIgnoreCase(to_str, tr("rectangular").toStdString()) || equalsIgnoreCase(to_str, tr("cartesian").toStdString())) {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_RECTANGULAR;
			} else if(equalsIgnoreCase(to_str, "exponential") || equalsIgnoreCase(to_str, tr("exponential").toStdString())) {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_EXPONENTIAL;
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_EXPONENTIAL;
			} else if(equalsIgnoreCase(to_str, "polar") || equalsIgnoreCase(to_str, tr("polar").toStdString())) {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_POLAR;
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_POLAR;
			} else if(to_str == "cis") {
				to_caf = 0;
				do_to = true;
				if(from_str.empty()) {
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(equalsIgnoreCase(to_str, "phasor") || equalsIgnoreCase(to_str, tr("phasor").toStdString()) || equalsIgnoreCase(to_str, "angle") || equalsIgnoreCase(to_str, tr("angle").toStdString())) {
				to_caf = 1;
				do_to = true;
				if(from_str.empty()) {
					settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
					b_busy--;
					executeCommand(COMMAND_EVAL);
					if(current_expr) setPreviousExpression();
					settings->evalops.complex_number_form = cnf_bak;
					return;
				}
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, tr("optimal").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_OPTIMAL);
					if(current_expr) setPreviousExpression();
					return;
				}
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "prefix") || equalsIgnoreCase(to_str, tr("prefix").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				to_prefix = 1;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, tr("base").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_BASE);
					if(current_expr) setPreviousExpression();
					return;
				}
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_BASE;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, tr("mixed").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_NONE;
				settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				if(from_str.empty()) {
					b_busy--;
					if(!previous_expression.empty()) calculateExpression(force, do_mathoperation, op, f, do_stack, stack_index, previous_expression);
					if(current_expr) setPreviousExpression();
					settings->evalops.auto_post_conversion = save_auto_post_conversion;
					settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
					settings->evalops.parse_options.units_enabled = b_units_saved;
					return;
				}
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, tr("factors").toStdString()) || equalsIgnoreCase(to_str, "factor")) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_FACTORIZE);
					if(current_expr) setPreviousExpression();
					return;
				}
				do_factors = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, tr("partial fraction").toStdString())) {
				if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_EXPAND_PARTIAL_FRACTIONS);
					if(current_expr) setPreviousExpression();
					return;
				}
				do_pfe = true;
				execute_str = from_str;
			} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, tr("base").toStdString())) {
				base_from_string(to_str2, to_base, to_nbase);
				to_duo_syms = false;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "decimals") || equalsIgnoreCase(to_str, tr("decimals").toStdString())) {
				to_fixed_fraction = 0;
				to_fraction = 3;
				do_to = true;
			} else {
				do_to = true;
				long int fden = get_fixed_denominator_qt(settings->unlocalizeExpression(to_str), to_fraction, tr("fraction"));
				if(fden != 0) {
					if(fden < 0) to_fixed_fraction = 0;
					else to_fixed_fraction = fden;
				} else if(from_str.empty()) {
					b_busy--;
					executeCommand(COMMAND_CONVERT_STRING, true, CALCULATOR->unlocalizeExpression(to_str, settings->evalops.parse_options));
					if(current_expr) setPreviousExpression();
					return;
				} else {
					if(to_str[0] == '?') {
						to_prefix = 1;
					} else if(to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
						to_prefix = to_str[0];
					}
					Unit *u = CALCULATOR->getActiveUnit(to_str);
					if(delay_complex != (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS) && u && u->baseUnit() == CALCULATOR->getRadUnit() && u->baseExponent() == 1) delay_complex = !delay_complex;
					if(!str_conv.empty()) str_conv += " to ";
					str_conv += to_str;
				}
			}
			if(str_left.empty()) break;
			to_str = str_left;
		}
		if(do_to) {
			if(from_str.empty()) {
				b_busy--;
				setResult(NULL, true, false, false);
				if(current_expr) setPreviousExpression();
				return;
			} else {
				execute_str = from_str;
				if(!str_conv.empty()) {
					execute_str += " to ";
					execute_str += str_conv;
				}
			}
		}
	}

	if(!delay_complex || (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS)) {
		settings->evalops.complex_number_form = cnf;
		delay_complex = false;
	} else {
		settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	}

	if(execute_str.empty()) {
		size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
		if(i != std::string::npos) {
			to_str = str.substr(0, i);
			if(to_str == "factor" || equalsIgnoreCase(to_str, "factorize") || equalsIgnoreCase(to_str, tr("factorize").toStdString())) {
				execute_str = str.substr(i + 1);
				do_factors = true;
			} else if(equalsIgnoreCase(to_str, "expand") || equalsIgnoreCase(to_str, tr("expand").toStdString())) {
				execute_str = str.substr(i + 1);
				do_expand = true;
			}
		}
	}

	size_t stack_size = 0;

	if(!settings->simplified_percentage) settings->evalops.parse_options.parsing_mode = (ParsingMode) (settings->evalops.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);

	parsed_tostruct->setUndefined();
	CALCULATOR->resetExchangeRatesUsed();
	CALCULATOR->setSimplifiedPercentageUsed(false);
	if(do_stack) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation && f) {
			CALCULATOR->getRPNRegister(stack_index + 1)->transform(f);
			parsed_mstruct->set(*CALCULATOR->getRPNRegister(stack_index + 1));
			CALCULATOR->calculateRPNRegister(stack_index + 1, 0, settings->evalops);
		} else {
			CALCULATOR->setRPNRegister(stack_index + 1, CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options), 0, settings->evalops, parsed_mstruct, parsed_tostruct);
		}
	} else if(settings->rpn_mode) {
		stack_size = CALCULATOR->RPNStackSize();
		if(do_mathoperation) {
			if(mstruct) {
				lastx = *mstruct;
				QTableWidgetItem *item = rpnView->item(0, 0);
				if(item) lastx_text = item->text();
			}
			rpnLastxAction->setEnabled(true);
			if(f) CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
			else CALCULATOR->calculateRPN(op, 0, settings->evalops, parsed_mstruct);
		} else {
			std::string str2 = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options);
			transform_expression_for_equals_save(str2, settings->evalops.parse_options);
			CALCULATOR->parseSigns(str2);
			remove_blank_ends(str2);
			MathStructure lastx_bak(lastx);
			if(mstruct) lastx = *mstruct;
			if(str2.length() == 1) {
				do_mathoperation = true;
				switch(str2[0]) {
					case 'E': {CALCULATOR->calculateRPN(OPERATION_EXP10, 0, settings->evalops, parsed_mstruct); break;}
					case '^': {CALCULATOR->calculateRPN(OPERATION_RAISE, 0, settings->evalops, parsed_mstruct); break;}
					case '+': {CALCULATOR->calculateRPN(OPERATION_ADD, 0, settings->evalops, parsed_mstruct); break;}
					case '-': {CALCULATOR->calculateRPN(OPERATION_SUBTRACT, 0, settings->evalops, parsed_mstruct); break;}
					case '*': {CALCULATOR->calculateRPN(OPERATION_MULTIPLY, 0, settings->evalops, parsed_mstruct); break;}
					case '/': {CALCULATOR->calculateRPN(OPERATION_DIVIDE, 0, settings->evalops, parsed_mstruct); break;}
					case '&': {CALCULATOR->calculateRPN(OPERATION_BITWISE_AND, 0, settings->evalops, parsed_mstruct); break;}
					case '|': {CALCULATOR->calculateRPN(OPERATION_BITWISE_OR, 0, settings->evalops, parsed_mstruct); break;}
					case '~': {CALCULATOR->calculateRPNBitwiseNot(0, settings->evalops, parsed_mstruct); break;}
					case '!': {CALCULATOR->calculateRPN(CALCULATOR->getFunctionById(FUNCTION_ID_FACTORIAL), 0, settings->evalops, parsed_mstruct); break;}
					case '>': {CALCULATOR->calculateRPN(OPERATION_GREATER, 0, settings->evalops, parsed_mstruct); break;}
					case '<': {CALCULATOR->calculateRPN(OPERATION_LESS, 0, settings->evalops, parsed_mstruct); break;}
					case '=': {CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, settings->evalops, parsed_mstruct); break;}
					case '\\': {
						MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
						if(fdiv) {
							CALCULATOR->calculateRPN(fdiv, 0, settings->evalops, parsed_mstruct);
							break;
						}
						do_mathoperation = false;
						break;
					}
					default: {do_mathoperation = false;}
				}
			} else if(str2.length() == 2) {
				if(str2 == "**") {
					CALCULATOR->calculateRPN(OPERATION_RAISE, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!!") {
					CALCULATOR->calculateRPN(CALCULATOR->getFunctionById(FUNCTION_ID_DOUBLE_FACTORIAL), 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "!=" || str == "=!" || str == "<>") {
					CALCULATOR->calculateRPN(OPERATION_NOT_EQUALS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "<=" || str == "=<") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_LESS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == ">=" || str == "=>") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS_GREATER, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "==") {
					CALCULATOR->calculateRPN(OPERATION_EQUALS, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "&&") {
					CALCULATOR->calculateRPN(OPERATION_LOGICAL_AND, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "||") {
					CALCULATOR->calculateRPN(OPERATION_LOGICAL_OR, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				} else if(str2 == "//") {
					MathFunction *fdiv = CALCULATOR->getActiveFunction("div");
					if(fdiv) {
						CALCULATOR->calculateRPN(fdiv, 0, settings->evalops, parsed_mstruct);
						do_mathoperation = true;
					}
				}
			} else if(str2.length() == 3) {
				if(str2 == "⊻") {
					CALCULATOR->calculateRPN(OPERATION_BITWISE_XOR, 0, settings->evalops, parsed_mstruct);
					do_mathoperation = true;
				}
			}
			if(!do_mathoperation) {
				bool had_nonnum = false, test_function = true;
				int in_par = 0;
				for(size_t i = 0; i < str2.length(); i++) {
					if(is_in(NUMBERS, str2[i])) {
						if(!had_nonnum || in_par) {
							test_function = false;
							break;
						}
					} else if(str2[i] == '(') {
						if(in_par || !had_nonnum) {
							test_function = false;
							break;
						}
						in_par = i;
					} else if(str2[i] == ')') {
						if(i != str2.length() - 1) {
							test_function = false;
							break;
						}
					} else if(str2[i] == ' ') {
						if(!in_par) {
							test_function = false;
							break;
						}
					} else if(is_in(NOT_IN_NAMES, str2[i])) {
						test_function = false;
						break;
					} else {
						if(in_par) {
							test_function = false;
							break;
						}
						had_nonnum = true;
					}
				}
				f = NULL;
				if(test_function) {
					if(in_par) f = CALCULATOR->getActiveFunction(str2.substr(0, in_par));
					else f = CALCULATOR->getActiveFunction(str2);
				}
				if(f && f->minargs() > 0) {
					do_mathoperation = true;
					original_expression = "";
					CALCULATOR->calculateRPN(f, 0, settings->evalops, parsed_mstruct);
				} else {
					original_expression = str2;
					CALCULATOR->RPNStackEnter(str2, 0, settings->evalops, parsed_mstruct, parsed_tostruct);
				}
			}
			if(do_mathoperation) {
				rpnLastxAction->setEnabled(true);
				QTableWidgetItem *item = rpnView->item(0, 0);
				if(item) lastx_text = item->text();
			}
			else lastx = lastx_bak;
		}
	} else {
		original_expression = CALCULATOR->unlocalizeExpression(execute_str.empty() ? str : execute_str, settings->evalops.parse_options);
		transform_expression_for_equals_save(original_expression, settings->evalops.parse_options);
		CALCULATOR->calculate(mstruct, original_expression, 0, settings->evalops, parsed_mstruct, parsed_tostruct);
	}

	bool title_set = false, was_busy = false;

	QProgressDialog *dialog = NULL;

	int i = 0;
	while(CALCULATOR->busy() && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(CALCULATOR->busy() && !was_busy) {
		if(!do_stack && updateWindowTitle(tr("Calculating…"))) title_set = true;
		dialog = new QProgressDialog(tr("Calculating…"), tr("Cancel"), 0, 0, this);
		dialog->setWindowTitle(tr("Calculating…"));
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(CALCULATOR->busy()) {
		qApp->processEvents();
		sleep_ms(100);
	}

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	if(delay_complex) {
		settings->evalops.complex_number_form = cnf;
		CALCULATOR->startControl(100);
		if(!settings->rpn_mode) {
			if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mstruct->complexToCisForm(settings->evalops);
			else if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mstruct->complexToPolarForm(settings->evalops);
		} else if(!do_stack) {
			MathStructure *mreg = CALCULATOR->getRPNRegister(do_stack ? stack_index + 1 : 1);
			if(mreg) {
				if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mreg->complexToCisForm(settings->evalops);
				else if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mreg->complexToPolarForm(settings->evalops);
			}
		}
		CALCULATOR->stopControl();
	}

	if(settings->rpn_mode && !do_stack) {
		mstruct->unref();
		mstruct = CALCULATOR->getRPNRegister(1);
		if(!mstruct) mstruct = new MathStructure();
		else mstruct->ref();
		settings->current_result = NULL;
	}

	if(do_stack) {
	} else if(settings->rpn_mode && do_mathoperation) {
		result_text = tr("RPN Operation").toStdString();
	} else {
		result_text = str;
	}
	settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
	if(settings->rpn_mode && stack_index == 0) {
		expressionEdit->clear();
		while(CALCULATOR->RPNStackSize() < stack_size) {
			RPNRegisterRemoved(1);
			stack_size--;
		}
		if(CALCULATOR->RPNStackSize() > stack_size) {
			RPNRegisterAdded("");
		}
	}

	if(settings->rpn_mode && do_mathoperation && parsed_tostruct && !parsed_tostruct->isUndefined() && parsed_tostruct->isSymbolic()) {
		mstruct->set(CALCULATOR->convert(*mstruct, parsed_tostruct->symbol(), settings->evalops));
	}

	// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
	if(!settings->rpn_mode && (!parsed_tostruct || parsed_tostruct->isUndefined()) && execute_str.empty() && !had_to_expression && (settings->evalops.approximation == APPROXIMATION_EXACT || settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) && parsed_mstruct && mstruct && ((parsed_mstruct->isMultiplication() && parsed_mstruct->size() == 2 && (*parsed_mstruct)[0].isNumber() && (*parsed_mstruct)[1].isUnit_exp() && parsed_mstruct->equals(*mstruct)) || (parsed_mstruct->isNegate() && (*parsed_mstruct)[0].isMultiplication() && (*parsed_mstruct)[0].size() == 2 && (*parsed_mstruct)[0][0].isNumber() && (*parsed_mstruct)[0][1].isUnit_exp() && mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1] == (*parsed_mstruct)[0][1] && (*mstruct)[0].isNumber() && (*parsed_mstruct)[0][0].number() == -(*mstruct)[0].number()) || (parsed_mstruct->isUnit_exp() && parsed_mstruct->equals(*mstruct)))) {
		Unit *u = NULL;
		MathStructure *munit = NULL;
		if(mstruct->isMultiplication()) munit = &(*mstruct)[1];
		else munit = mstruct;
		if(munit->isUnit()) u = munit->unit();
		else u = (*munit)[0].unit();
		if(u && u->isCurrency()) {
			if(settings->evalops.local_currency_conversion && CALCULATOR->getLocalCurrency() && u != CALCULATOR->getLocalCurrency()) {
				ApproximationMode abak = settings->evalops.approximation;
				if(settings->evalops.approximation == APPROXIMATION_EXACT) settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = abak;
			}
		} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
			MathStructure mbak(*mstruct);
			if(settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) {
				if(munit->isUnit() && u->referenceName() == "oF") {
					u = CALCULATOR->getActiveUnit("oC");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, settings->evalops, true, false, false));
				} else if(munit->isUnit() && u->referenceName() == "oC") {
					u = CALCULATOR->getActiveUnit("oF");
					if(u) mstruct->set(CALCULATOR->convert(*mstruct, u, settings->evalops, true, false, false));
				} else {
					mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				}
			}
			if(settings->evalops.approximation == APPROXIMATION_EXACT && ((settings->evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL && settings->evalops.auto_post_conversion != POST_CONVERSION_NONE) || mstruct->equals(mbak))) {
				settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
				if(settings->evalops.auto_post_conversion == POST_CONVERSION_BASE) mstruct->set(CALCULATOR->convertToBaseUnits(*mstruct, settings->evalops));
				else mstruct->set(CALCULATOR->convertToOptimalUnit(*mstruct, settings->evalops, true));
				settings->evalops.approximation = APPROXIMATION_EXACT;
			}
		}
	}

	if(!do_mathoperation && (askTC(*parsed_mstruct) || askSinc(*parsed_mstruct) || askSinc(*mstruct) || askPercent() || (check_exrates && settings->checkExchangeRates(this)))) {
		b_busy--;
		calculateExpression(force, do_mathoperation, op, f, settings->rpn_mode, stack_index, saved_execute_str, str, false);
		settings->evalops.complex_number_form = cnf_bak;
		settings->evalops.auto_post_conversion = save_auto_post_conversion;
		settings->evalops.parse_options.units_enabled = b_units_saved;
		settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
		settings->printops.time_zone = TIME_ZONE_LOCAL;
		if(!settings->simplified_percentage) settings->evalops.parse_options.parsing_mode = (ParsingMode) (settings->evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);
		return;
	}

	//update "ans" variables
	if(stack_index == 0) {
		MathStructure m4(settings->vans[3]->get());
		m4.replace(settings->vans[4], settings->vans[4]->get());
		settings->vans[4]->set(m4);
		MathStructure m3(settings->vans[2]->get());
		m3.replace(settings->vans[3], settings->vans[4]);
		settings->vans[3]->set(m3);
		MathStructure m2(settings->vans[1]->get());
		m2.replace(settings->vans[2], settings->vans[3]);
		settings->vans[2]->set(m2);
		MathStructure m1(settings->vans[0]->get());
		m1.replace(settings->vans[1], settings->vans[2]);
		settings->vans[1]->set(m1);
		mstruct->replace(settings->vans[0], settings->vans[1]);
		settings->vans[0]->set(*mstruct);
	}

	mstruct_exact.setUndefined();

	if((!settings->rpn_mode || (!do_stack && !do_mathoperation)) && (!do_calendars || !mstruct->isDateTime()) && (settings->dual_approximation > 0 || settings->printops.base == BASE_DECIMAL) && !do_bases) {
		long int i_timeleft = 0;
		i_timeleft = mstruct->containsType(STRUCT_COMPARISON) ? 2000 : 1000;
		if(i_timeleft > 0) {
			if(delay_complex) settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
			calculate_dual_exact(mstruct_exact, mstruct, original_expression, parsed_mstruct, settings->evalops, settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_approximation > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), i_timeleft, -1);
			if(delay_complex && !mstruct_exact.isUndefined()) {
				settings->evalops.complex_number_form = cnf;
				CALCULATOR->startControl(100);
				if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mstruct_exact.complexToCisForm(settings->evalops);
				else if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mstruct_exact.complexToPolarForm(settings->evalops);
				CALCULATOR->stopControl();
			}
		}
	}

	b_busy--;

	if(do_factors || do_pfe || do_expand) {
		if(do_stack && stack_index != 0) {
			MathStructure *save_mstruct = mstruct;
			mstruct = CALCULATOR->getRPNRegister(stack_index + 1);
			if(do_factors && (mstruct->isNumber() || mstruct->isVector()) && to_fraction == 0 && to_fixed_fraction < 2) to_fraction = 2;
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			mstruct = save_mstruct;
		} else {
			if(do_factors && mstruct->isInteger() && !parsed_mstruct->isNumber()) prepend_mstruct = *mstruct;
			if(do_factors && (mstruct->isNumber() || mstruct->isVector()) && to_fraction == 0 && to_fixed_fraction < 2) to_fraction = 2;
			executeCommand(do_pfe ? COMMAND_EXPAND_PARTIAL_FRACTIONS  : (do_expand ? COMMAND_EXPAND : COMMAND_FACTORIZE), false);
			if(!prepend_mstruct.isUndefined() && mstruct->isInteger()) prepend_mstruct.setUndefined();
		}
	}

	if(stack_index == 0) {
		if(!mstruct_exact.isUndefined()) settings->history_answer.push_back(new MathStructure(mstruct_exact));
		settings->history_answer.push_back(new MathStructure(*mstruct));
	}

	if(!do_stack) previous_expression = execute_str.empty() ? str : execute_str;
	setResult(NULL, true, stack_index == 0, true, "", do_stack, stack_index);
	prepend_mstruct.setUndefined();
	
	if(do_bases) onBasesActivated(true);
	if(do_calendars) openCalendarConversion();
	
	settings->evalops.complex_number_form = cnf_bak;
	settings->evalops.auto_post_conversion = save_auto_post_conversion;
	settings->evalops.parse_options.units_enabled = b_units_saved;
	settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
	settings->printops.time_zone = TIME_ZONE_LOCAL;
	if(!settings->simplified_percentage) settings->evalops.parse_options.parsing_mode = (ParsingMode) (settings->evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);

	if(stack_index == 0) {
		if(unitsDialog && unitsDialog->isVisible()) {
			Unit *u = CALCULATOR->findMatchingUnit(*mstruct);
			if(u && !u->category().empty()) {
				unitsDialog->selectCategory(u->category());
			}
		}
		expressionEdit->blockCompletion();
		expressionEdit->blockParseStatus();
		if(settings->chain_mode) {
			if(exact_text == "0" || result_text == "0") expressionEdit->clear();
			std::string str = unhtmlize(result_text);
			if(unicode_length(result_text) < 10000) expressionEdit->setExpression(QString::fromStdString(str));
		} else if(settings->replace_expression == CLEAR_EXPRESSION) {
			expressionEdit->clear();
		} else if(settings->replace_expression == REPLACE_EXPRESSION_WITH_RESULT || settings->replace_expression == REPLACE_EXPRESSION_WITH_RESULT_IF_SHORTER) {
			if(settings->replace_expression == REPLACE_EXPRESSION_WITH_RESULT || (!exact_text.empty() && unicode_length(exact_text) < unicode_length(from_str))) {
				if(exact_text == "0" || result_text == "0") expressionEdit->clear();
				else if(exact_text.empty()) {
					std::string str = unhtmlize(result_text);
					if(unicode_length(result_text) < 10000) expressionEdit->setExpression(QString::fromStdString(str));
				} else {
					if(settings->replace_expression != REPLACE_EXPRESSION_WITH_RESULT || unicode_length(exact_text) < 10000) expressionEdit->setExpression(QString::fromStdString(exact_text));
				}
			} else {
				if(!execute_str.empty()) {
					from_str = execute_str;
					CALCULATOR->separateToExpression(from_str, str, settings->evalops, true, true);
				}
				expressionEdit->setExpression(QString::fromStdString(from_str));
			}
		}
		if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
		expressionEdit->selectAll();
		expressionEdit->blockCompletion(false);
		expressionEdit->blockParseStatus(false);
		expressionEdit->setExpressionHasChanged(false);
		if(settings->autocopy_result) {
			if(settings->copy_ascii) {
				std::string str = result_text;
				if(settings->copy_ascii_without_units) {
					size_t i = str.find("<span style=\"color:#008000\">");
					if(i == std::string::npos) i = str.find("<span style=\"color:#BBFFBB\">");
					if(i != std::string::npos && str.find("</span>", i) == str.length() - 7) {
						str = str.substr(0, i);
					}
				}
				QApplication::clipboard()->setText(QString::fromStdString(unformat(unhtmlize(str, true))).trimmed());
			} else {
				QMimeData *qm = new QMimeData();
				qm->setHtml(QString::fromStdString(uncolorize(result_text)));
				qm->setText(QString::fromStdString(unhtmlize(result_text)));
				qm->setObjectName("history_result");
				QApplication::clipboard()->setMimeData(qm);
			}
		}
	}

	if(CALCULATOR->checkSaveFunctionCalled()) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		if(functionsDialog) functionsDialog->updateFunctions();
	}

}

void CommandThread::run() {

	enableAsynchronousCancel();

	while(true) {
		int command_type = 0;
		if(!read(&command_type)) break;
		void *x = NULL;
		if(!read(&x) || !x) break;
		void *x2 = NULL;
		if(!read(&x2)) break;
		CALCULATOR->startControl();
		EvaluationOptions eo2 = settings->evalops;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				if(!((MathStructure*) x)->integerFactorize()) {
					((MathStructure*) x)->structure(STRUCTURING_FACTORIZE, eo2, true);
				}
				if(x2 && !((MathStructure*) x2)->integerFactorize()) {
					eo2.approximation = APPROXIMATION_EXACT;
					((MathStructure*) x2)->structure(STRUCTURING_FACTORIZE, eo2, true);
				}
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				((MathStructure*) x)->expandPartialFractions(eo2);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->expandPartialFractions(eo2);
				break;
			}
			case COMMAND_EXPAND: {
				((MathStructure*) x)->expand(eo2);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->expand(eo2);
				break;
			}
			case COMMAND_CONVERT_STRING: {
				MathStructure pm_tmp(*parsed_mstruct);
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_units_string, eo2, NULL, true, &pm_tmp));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convert(*((MathStructure*) x2), command_convert_units_string, eo2, NULL, true));
				break;
			}
			case COMMAND_CONVERT_UNIT: {
				MathStructure pm_tmp(*parsed_mstruct);
				((MathStructure*) x)->set(CALCULATOR->convert(*((MathStructure*) x), command_convert_unit, eo2, false, true, true, &pm_tmp));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convert(*((MathStructure*) x2), command_convert_unit, eo2, false, true, true));
				break;
			}
			case COMMAND_CONVERT_OPTIMAL: {
				((MathStructure*) x)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x), eo2, true));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convertToOptimalUnit(*((MathStructure*) x2), eo2, true));
				break;
			}
			case COMMAND_CONVERT_BASE: {
				((MathStructure*) x)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x), eo2));
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->set(CALCULATOR->convertToBaseUnits(*((MathStructure*) x2), eo2));
				break;
			}
			case COMMAND_CALCULATE: {
				eo2.calculate_functions = false;
				eo2.sync_units = false;
				((MathStructure*) x)->calculatesub(eo2, eo2, true);
				eo2.approximation = APPROXIMATION_EXACT;
				if(x2) ((MathStructure*) x2)->calculatesub(eo2, eo2, true);
				break;
			}
			case COMMAND_EVAL: {
				((MathStructure*) x)->eval(eo2);
				if(x2) ((MathStructure*) x2)->eval(eo2);
				break;
			}
		}
		b_busy--;
		CALCULATOR->stopControl();

	}
}

void QalculateWindow::executeCommand(int command_type, bool show_result, std::string ceu_str, Unit *u, int run) {

	if(run == 1) {

		if(expressionEdit->expressionHasChanged() && !settings->rpn_mode) {
			calculateExpression();
		}

		if(b_busy) return;

		b_busy++;
		command_aborted = false;

		if(command_type >= COMMAND_CONVERT_UNIT) {
			CALCULATOR->resetExchangeRatesUsed();
			command_convert_units_string = ceu_str;
			command_convert_unit = u;
		}
		if(command_type == COMMAND_CONVERT_UNIT || command_type == COMMAND_CONVERT_STRING || command_type == COMMAND_CONVERT_BASE || command_type == COMMAND_CONVERT_OPTIMAL) {
			to_prefix = 0;
		}
	}

	bool title_set = false, was_busy = false;
	QProgressDialog *dialog = NULL;

	int i = 0;

	MathStructure *mfactor = new MathStructure(*mstruct);
	MathStructure *mfactor2 = NULL;
	if(!mstruct_exact.isUndefined()) mfactor2 = new MathStructure(mstruct_exact);
	MathStructure parsebak(*parsed_mstruct);

	rerun_command:

	if((!commandThread->running && !commandThread->start()) || !commandThread->write(command_type) || !commandThread->write((void *) mfactor) || !commandThread->write((void *) mfactor2)) {
		commandThread->cancel();
		mfactor->unref();
		if(mfactor2) mfactor2->unref();
		b_busy--;
		return;
	}

	while(b_busy && commandThread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(!was_busy && b_busy && commandThread->running) {
		QString progress_str;
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				progress_str = tr("Factorizing…");
				break;
			}
			case COMMAND_EXPAND_PARTIAL_FRACTIONS: {
				progress_str = tr("Expanding partial fractions…");
				break;
			}
			case COMMAND_EXPAND: {
				progress_str = tr("Expanding…");
				break;
			}
			case COMMAND_EVAL: {
				progress_str = tr("Calculating…");
				break;
			}
			default: {
				progress_str = tr("Converting…");
				break;
			}
		}
		if(updateWindowTitle(progress_str)) title_set = true;
		dialog = new QProgressDialog(progress_str, tr("Cancel"), 0, 0, this);
		dialog->setWindowTitle(progress_str);
		connect(dialog, SIGNAL(canceled()), this, SLOT(abortCommand()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(b_busy && commandThread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	if(!commandThread->running) command_aborted = true;

	if(!command_aborted && run == 1 && command_type >= COMMAND_CONVERT_UNIT && settings->checkExchangeRates(this)) {
		b_busy++;
		parsed_mstruct->set(parsebak);
		mfactor->set(*mstruct);
		run = 2;
		goto rerun_command;
	}

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	if(command_type == COMMAND_CONVERT_STRING && !ceu_str.empty()) {
		if(ceu_str[0] == '?') {
			to_prefix = 1;
		} else if(ceu_str.length() > 1 && ceu_str[1] == '?' && (ceu_str[0] == 'b' || ceu_str[0] == 'a' || ceu_str[0] == 'd')) {
			to_prefix = ceu_str[0];
		}
	}

	if(!command_aborted) {
		if(mfactor2) {mstruct_exact.set(*mfactor2); mfactor2->unref();}
		mstruct->set(*mfactor);
		mfactor->unref();
		switch(command_type) {
			case COMMAND_FACTORIZE: {
				settings->printops.allow_factorization = true;
				break;
			}
			case COMMAND_EXPAND: {
				settings->printops.allow_factorization = false;
				break;
			}
			default: {
				settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
			}
		}
		if(show_result) {
			if(!mstruct_exact.isUndefined()) settings->history_answer.push_back(new MathStructure(mstruct_exact));
			settings->history_answer.push_back(new MathStructure(*mstruct));
			setResult(NULL, true, !parsed_mstruct->equals(parsebak, true, true), true, "");
		}
	}

}

void QalculateWindow::updateResultBases() {
	SET_BINARY_BITS
	if(result_bin.length() == (size_t) binary_bits + (binary_bits / 4) - 1) {
		QString sbin = "<table align=\"right\" cellspacing=\"0\" border=\"0\"><tr><td>", sbin_i;
		int i2 = 0;
		size_t pos = 0;
		QString link_color = binEdit->palette().text().color().name();
		for(int i = binary_bits; i >= 0; i -= 8) {
			if(i % 32 == 0 || i == binary_bits) {
				if(!sbin_i.isEmpty()) {
					bool inhtml = false;
					int n = i + 1;
					for(; i2 >= 0; i2--) {
						if(sbin_i[i2] == '>') {
							inhtml = true;
						} else if(sbin_i[i2] == '<') {
							inhtml = false;
						} else if(!inhtml && (sbin_i[i2] == '0' || sbin_i[i2] == '1')) {
							sbin_i.replace(i2, 1, QString("<a href=\"%1\" style=\"text-decoration: none; color: %3\">%2</a>").arg(n).arg(sbin_i[i2]).arg(link_color));
							n++;
						}
					}
					if(pos > 39) sbin += "</td></tr><tr>";
					sbin += sbin_i;
				}
				if(i == 0) break;
				if(binary_bits <= 32) sbin_i = QString::fromStdString(result_bin);
				else sbin_i = QString::fromStdString(result_bin.substr(pos, pos > 0 ? 40 : 39));
				if(pos == 0) pos = 39;
				else pos += 40;
				sbin_i.replace(" ", "&nbsp;</td><td>");
				sbin_i += "</td></tr><tr>";
				i2 = sbin_i.length();
			}
			sbin_i += "<td colspan=\"2\" valign=\"top\"><font color=\"gray\" size=\"-1\">";
			sbin_i += QString::number(i);
			sbin_i += "</font></td>";
		}
		sbin += "</tr><table>";
		binEdit->setText(sbin);
	} else {
		binEdit->setText(QString::fromStdString(result_bin));
	}
	octEdit->setText(QString::fromStdString(result_oct));
	decEdit->setText(QString::fromStdString(result_dec));
	hexEdit->setText(QString::fromStdString(result_hex));
}
void QalculateWindow::resultBasesLinkActivated(const QString &s) {
	size_t n = s.toInt();
	n += (n - 1) / 4;
	if(n > result_bin.length()) return;
	n = result_bin.length() - n;
	if(result_bin[n] == '0') result_bin[n] = '1';
	else if(result_bin[n] == '1') result_bin[n] = '0';
	ParseOptions pa;
	pa.base = BASE_BINARY;
	pa.twos_complement = true;
	PrintOptions po;
	po.base = settings->evalops.parse_options.base;
	po.twos_complement = settings->evalops.parse_options.twos_complement;
	po.min_exp = 0;
	po.preserve_precision = true;
	po.base_display = BASE_DISPLAY_NONE;
	expressionEdit->setPlainText(QString::fromStdString(Number(result_bin, pa).print(po)));
}

void set_result_bases(const MathStructure &m) {
	result_bin = ""; result_oct = "", result_dec = "", result_hex = "";
	SET_BINARY_BITS
	if(max_bases.isZero()) {max_bases = 2; max_bases ^= (binary_bits > 128 ? 128 : binary_bits); min_bases = 2; min_bases ^= (binary_bits > 128 ? 64 : binary_bits / 2); min_bases.negate();}
	if(!CALCULATOR->aborted() && ((m.isNumber() && m.number() < max_bases && m.number() > min_bases) || (m.isNegate() && m[0].isNumber() && m[0].number() < max_bases && m[0].number() > min_bases))) {
		Number nr;
		if(m.isNumber()) {
			nr = m.number();
		} else {
			nr = m[0].number();
			nr.negate();
		}
		if(settings->rounding_mode == 2) nr.trunc();
		else nr.round(settings->printops.round_halfway_to_even);
		PrintOptions po = settings->printops;
		po.is_approximate = NULL;
		po.show_ending_zeroes = false;
		po.base_display = BASE_DISPLAY_NORMAL;
		po.min_exp = 0;
		po.base = 2;
		po.binary_bits = binary_bits;
		result_bin = nr.print(po);
		if(result_bin.length() > po.binary_bits + (po.binary_bits / 4) && result_bin.find("1") >= po.binary_bits + (po.binary_bits / 4)) result_bin.erase(0, po.binary_bits + (po.binary_bits / 4));
		po.base = 8;
		if(po.binary_bits > 128) po.binary_bits = 128;
		result_oct = nr.print(po);
		size_t i = result_oct.find_first_of(NUMBERS);
		if(i != std::string::npos && result_oct.length() > i + 1 && result_oct[i] == '0' && is_in(NUMBERS, result_oct[i + 1])) result_oct.erase(i, 1);
		po.base = 10;
		result_dec = nr.print(po);
		po.base = 16;
		result_hex = nr.print(po);
		gsub("0x", "", result_hex);
		size_t l = result_hex.length();
		size_t i_after_minus = 0;
		if(nr.isNegative()) {
			if(l > 1 && result_hex[0] == '-') i_after_minus = 1;
			else if(result_hex.find("−") == 0) i_after_minus = strlen("−");
		}
		for(int i = (int) l - 2; i > (int) i_after_minus; i -= 2) {
			result_hex.insert(i, 1, ' ');
		}
		if(result_hex.length() > i_after_minus + 1 && result_hex[i_after_minus + 1] == ' ') result_hex.insert(i_after_minus, 1, '0');
	}
}

bool contains_plot_or_save(const std::string &str) {
	if(expression_contains_save_function(CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options), settings->evalops.parse_options, false)) return true;
	MathFunction *f = CALCULATOR->getFunctionById(FUNCTION_ID_PLOT);
	for(size_t i = 1; f && i <= f->countNames(); i++) {
		if(str.find(f->getName(i).name) != std::string::npos) return true;
	}
	return false;
}

void QalculateWindow::onExpressionChanged() {
	toAction_t->setEnabled(expressionEdit->expressionHasChanged() || !settings->history_answer.empty());
	if(!basesDock->isVisible()) return;
	if(!expressionEdit->expressionHasChanged()) {
		if(!result_bin.empty() && !bases_is_result && expressionEdit->document()->isEmpty()) {
			set_result_bases(m_zero);
			bases_is_result = false;
			updateResultBases();
		}
		return;
	}
	MathStructure m;
	EvaluationOptions eo = settings->evalops;
	eo.structuring = STRUCTURING_NONE;
	eo.mixed_units_conversion = MIXED_UNITS_CONVERSION_NONE;
	eo.auto_post_conversion = POST_CONVERSION_NONE;
	eo.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	eo.expand = -2;
	CALCULATOR->beginTemporaryStopMessages();
	bases_is_result = false;
	std::string str = expressionEdit->toPlainText().toStdString();
	if(contains_plot_or_save(str) || !CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 100, eo)) {
		result_bin = ""; result_oct = "", result_dec = "", result_hex = "";
	} else {
		set_result_bases(m);
	}
	CALCULATOR->endTemporaryStopMessages();
	updateResultBases();
}

void QalculateWindow::onHistoryReloaded() {
	if(settings->status_in_history && settings->display_expression_status) {
		historyView->clearTemporary();
		auto_expression = "";
		mauto.setAborted();
		auto_error = false;
		if(autoCalculateTimer) autoCalculateTimer->stop();
		if(!settings->status_in_history) updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true);
		if(expressionEdit->expressionHasChanged()) expressionEdit->displayParseStatus(true);
	}
}
void QalculateWindow::onStatusChanged(QString status, bool is_expression, bool had_error, bool had_warning, bool expression_from_history) {
	if(!settings->status_in_history || settings->rpn_mode) return;
	std::string current_text = expressionEdit->toPlainText().trimmed().toStdString();
	bool last_op = false;
	if(expressionEdit->textCursor().atEnd()) last_op = last_is_operator(current_text);
	if(status.isEmpty()) {
		if(autoCalculateTimer) autoCalculateTimer->stop();
		historyView->clearTemporary();
		auto_expression = "";
		auto_result = "";
		auto_error = false;
		mauto.setAborted();
		updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true);
	} else if(!is_expression || !settings->auto_calculate || contains_plot_or_save(current_text)) {
		if(autoCalculateTimer) autoCalculateTimer->stop();
		if(!had_error && (!had_warning || last_op) && !auto_error && auto_result.empty() && (auto_expression == status.toStdString() || (last_op && auto_expression.empty() && auto_result.empty()))) return;
		auto_error = had_error || had_warning;
		std::vector<std::string> values;
		auto_expression = status.toStdString();
		auto_result = "";
		mauto.setAborted();
		CALCULATOR->addMessages(&expressionEdit->status_messages);
		historyView->addResult(values, current_text, true, auto_expression, false, false, QString(), NULL, 0, 0, true);
		updateWindowTitle(QString(), true);
	} else {
		if(!had_error && (!had_warning || last_op) && !auto_error && !auto_calculation_updated && !auto_format_updated && (auto_expression == status.toStdString() || (last_op && auto_expression.empty() && auto_result.empty()))) {
			if(autoCalculateTimer && autoCalculateTimer->isActive()) {
				autoCalculateTimer->stop();
				autoCalculateTimer->start(settings->auto_calculate_delay);
			}
			return;
		}
		current_status_expression = current_text;
		current_status = status;
		if(autoCalculateTimer) autoCalculateTimer->stop();
		if(had_error || expression_from_history || settings->auto_calculate_delay > 0) {
			if(had_error || expression_from_history || had_warning || auto_error || !auto_result.empty() || auto_expression != status.toStdString()) {
				auto_result = "";
				mauto.setAborted();
				auto_expression = status.toStdString();
				auto_error = had_error || had_warning;
				std::vector<std::string> values;
				CALCULATOR->addMessages(&expressionEdit->status_messages);
				historyView->addResult(values, current_text, true, auto_expression, false, false, QString(), NULL, 0, 0, true);
			}
			if(had_error || expression_from_history) {
				updateWindowTitle(QString(), true);
				return;
			}
			if(!autoCalculateTimer) {
				autoCalculateTimer = new QTimer(this);
				autoCalculateTimer->setSingleShot(true);
				connect(autoCalculateTimer, SIGNAL(timeout()), this, SLOT(autoCalculateTimeout()));
			}
			autoCalculateTimer->start(settings->auto_calculate_delay);
		} else {
			autoCalculateTimeout();
		}
	}
}
void QalculateWindow::autoCalculateTimeout() {
	mauto.setAborted();
	bool is_approximate = false;
	PrintOptions po = settings->printops;
	po.is_approximate = &is_approximate;
	std::string str = current_status_expression, result;

	CALCULATOR->beginTemporaryStopMessages();

	bool do_factors = false, do_pfe = false, do_expand = false, do_bases = false, do_calendars = false;
	to_fraction = 0; to_fixed_fraction = 0; to_prefix = 0; to_base = 0; to_bits = 0; to_nbase.clear(); to_caf = -1;
	std::string to_str, str_conv;
	CALCULATOR->parseComments(str, settings->evalops.parse_options);
	if(str.empty()) {
		CALCULATOR->endTemporaryStopMessages();
		updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true);
		return;
	}
	ComplexNumberForm cnf_bak = settings->evalops.complex_number_form;
	ComplexNumberForm cnf = settings->evalops.complex_number_form;
	bool delay_complex = false;
	bool b_units_saved = settings->evalops.parse_options.units_enabled;
	AutoPostConversion save_auto_post_conversion = settings->evalops.auto_post_conversion;
	MixedUnitsConversion save_mixed_units_conversion = settings->evalops.mixed_units_conversion;

	CALCULATOR->startControl(100);

	had_to_expression = false;
	std::string from_str = str;
	bool last_is_space = !from_str.empty() && is_in(SPACES, from_str[from_str.length() - 1]);
	if(CALCULATOR->separateToExpression(from_str, to_str, settings->evalops, true, true)) {
		if(from_str.empty()) {
			CALCULATOR->stopControl();
			CALCULATOR->endTemporaryStopMessages();
			updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true);
			return;
		}
		remove_duplicate_blanks(to_str);
		had_to_expression = true;
		std::string str_left;
		std::string to_str1, to_str2;
		bool do_to = false;
		while(true) {
			if(last_is_space) to_str += " ";
			CALCULATOR->separateToExpression(to_str, str_left, settings->evalops, true, false);
			remove_blank_ends(to_str);
			size_t ispace = to_str.find_first_of(SPACES);
			if(ispace != std::string::npos) {
				to_str1 = to_str.substr(0, ispace);
				remove_blank_ends(to_str1);
				to_str2 = to_str.substr(ispace + 1);
				remove_blank_ends(to_str2);
			}
			if(equalsIgnoreCase(to_str, "hex") || equalsIgnoreCase(to_str, "hexadecimal") || equalsIgnoreCase(to_str, tr("hexadecimal").toStdString())) {
				to_base = BASE_HEXADECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "oct") || equalsIgnoreCase(to_str, "octal") || equalsIgnoreCase(to_str, tr("octal").toStdString())) {
				to_base = BASE_OCTAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "dec") || equalsIgnoreCase(to_str, "decimal") || equalsIgnoreCase(to_str, tr("decimal").toStdString())) {
				to_base = BASE_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "duo") || equalsIgnoreCase(to_str, "duodecimal") || equalsIgnoreCase(to_str, tr("duodecimal").toStdString())) {
				to_base = BASE_DUODECIMAL;
				to_duo_syms = false;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "doz") || equalsIgnoreCase(to_str, "dozenal")) {
				to_base = BASE_DUODECIMAL;
				to_duo_syms = true;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bin") || equalsIgnoreCase(to_str, "binary") || equalsIgnoreCase(to_str, tr("binary").toStdString())) {
				to_base = BASE_BINARY;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "roman") || equalsIgnoreCase(to_str, tr("roman").toStdString())) {
				to_base = BASE_ROMAN_NUMERALS;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bijective") || equalsIgnoreCase(to_str, tr("bijective").toStdString())) {
				to_base = BASE_BIJECTIVE_26;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bcd")) {
				to_base = BASE_BINARY_DECIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa") || equalsIgnoreCase(to_str, "sexagesimal") || equalsIgnoreCase(to_str, tr("sexagesimal").toStdString())) {
				to_base = BASE_SEXAGESIMAL;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa2") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "2")) {
				to_base = BASE_SEXAGESIMAL_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "sexa3") || EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "sexagesimal", tr("sexagesimal"), "3")) {
				to_base = BASE_SEXAGESIMAL_3;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "latitude") || equalsIgnoreCase(to_str, tr("latitude").toStdString())) {
				to_base = BASE_LATITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "latitude", tr("latitude"), "2")) {
				to_base = BASE_LATITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "longitude") || equalsIgnoreCase(to_str, tr("longitude").toStdString())) {
				to_base = BASE_LONGITUDE;
				do_to = true;
			} else if(EQUALS_IGNORECASE_AND_LOCAL_NR(to_str, "longitude", tr("longitude"), "2")) {
				to_base = BASE_LONGITUDE_2;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp32") || equalsIgnoreCase(to_str, "binary32") || equalsIgnoreCase(to_str, "float")) {
				to_base = BASE_FP32;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp64") || equalsIgnoreCase(to_str, "binary64") || equalsIgnoreCase(to_str, "double")) {
				to_base = BASE_FP64;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp16") || equalsIgnoreCase(to_str, "binary16")) {
				to_base = BASE_FP16;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp80")) {
				to_base = BASE_FP80;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "fp128") || equalsIgnoreCase(to_str, "binary128")) {
				to_base = BASE_FP128;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "time") || equalsIgnoreCase(to_str, tr("time").toStdString())) {
				to_base = BASE_TIME;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "Unicode")) {
				to_base = BASE_UNICODE;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "utc") || equalsIgnoreCase(to_str, "gmt")) {
				settings->printops.time_zone = TIME_ZONE_UTC;
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "bin") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_BINARY;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && equalsIgnoreCase(to_str.substr(0, 3), "hex") && is_in(NUMBERS, to_str[3])) {
				to_base = BASE_HEXADECIMAL;
				int bits = s2i(to_str.substr(3));
				if(bits >= 0) {
					if(bits > 4096) to_bits = 4096;
					else to_bits = bits;
				}
				do_to = true;
			} else if(to_str.length() > 3 && (equalsIgnoreCase(to_str.substr(0, 3), "utc") || equalsIgnoreCase(to_str.substr(0, 3), "gmt"))) {
				to_str = to_str.substr(3);
				remove_blanks(to_str);
				bool b_minus = false;
				if(to_str[0] == '+') {
					to_str.erase(0, 1);
				} else if(to_str[0] == '-') {
					b_minus = true;
					to_str.erase(0, 1);
				} else if(to_str.find(SIGN_MINUS) == 0) {
					b_minus = true;
					to_str.erase(0, strlen(SIGN_MINUS));
				}
				unsigned int tzh = 0, tzm = 0;
				int itz = 0;
				if(!to_str.empty() && sscanf(to_str.c_str(), "%2u:%2u", &tzh, &tzm) > 0) {
					itz = tzh * 60 + tzm;
					if(b_minus) itz = -itz;
				} else {
					CALCULATOR->error(true, tr("Time zone parsing failed.").toUtf8().data(),  NULL);
				}
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = itz;
				do_to = true;
			} else if(to_str == "CET") {
				settings->printops.time_zone = TIME_ZONE_CUSTOM;
				settings->printops.custom_time_zone = 60;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "bases") || equalsIgnoreCase(to_str, tr("bases").toStdString())) {
				do_bases = true;
				str = from_str;
			} else if(equalsIgnoreCase(to_str, "calendars") || equalsIgnoreCase(to_str, tr("calendars").toStdString())) {
				do_calendars = true;
				str = from_str;
			} else if(equalsIgnoreCase(to_str, "rectangular") || equalsIgnoreCase(to_str, "cartesian") || equalsIgnoreCase(to_str, tr("rectangular").toStdString()) || equalsIgnoreCase(to_str, tr("cartesian").toStdString())) {
				to_caf = 0;
				do_to = true;
				cnf = COMPLEX_NUMBER_FORM_RECTANGULAR;
			} else if(equalsIgnoreCase(to_str, "exponential") || equalsIgnoreCase(to_str, tr("exponential").toStdString())) {
				to_caf = 0;
				do_to = true;
				cnf = COMPLEX_NUMBER_FORM_EXPONENTIAL;
			} else if(equalsIgnoreCase(to_str, "polar") || equalsIgnoreCase(to_str, tr("polar").toStdString())) {
				to_caf = 0;
				do_to = true;
				cnf = COMPLEX_NUMBER_FORM_POLAR;
			} else if(to_str == "cis") {
				to_caf = 0;
				do_to = true;
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(equalsIgnoreCase(to_str, "phasor") || equalsIgnoreCase(to_str, tr("phasor").toStdString()) || equalsIgnoreCase(to_str, "angle") || equalsIgnoreCase(to_str, tr("angle").toStdString())) {
				to_caf = 1;
				do_to = true;
				cnf = COMPLEX_NUMBER_FORM_CIS;
			} else if(equalsIgnoreCase(to_str, "optimal") || equalsIgnoreCase(to_str, tr("optimal").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_OPTIMAL_SI;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "prefix") || equalsIgnoreCase(to_str, tr("prefix").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				to_prefix = 1;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "base") || equalsIgnoreCase(to_str, tr("base").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_BASE;
				str_conv = "";
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "mixed") || equalsIgnoreCase(to_str, tr("mixed").toStdString())) {
				settings->evalops.parse_options.units_enabled = true;
				settings->evalops.auto_post_conversion = POST_CONVERSION_NONE;
				settings->evalops.mixed_units_conversion = MIXED_UNITS_CONVERSION_FORCE_INTEGER;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "factors") || equalsIgnoreCase(to_str, tr("factors").toStdString()) || equalsIgnoreCase(to_str, "factor")) {
				do_factors = true;
				str = from_str;
			} else if(equalsIgnoreCase(to_str, "partial fraction") || equalsIgnoreCase(to_str, tr("partial fraction").toStdString())) {
				do_pfe = true;
				str = from_str;
			} else if(equalsIgnoreCase(to_str1, "base") || equalsIgnoreCase(to_str1, tr("base").toStdString())) {
				base_from_string(to_str2, to_base, to_nbase);
				to_duo_syms = false;
				do_to = true;
			} else if(equalsIgnoreCase(to_str, "decimals") || equalsIgnoreCase(to_str, tr("decimals").toStdString())) {
				to_fixed_fraction = 0;
				to_fraction = 3;
				do_to = true;
			} else {
				do_to = true;
				long int fden = get_fixed_denominator_qt(settings->unlocalizeExpression(to_str), to_fraction, tr("fraction"));
				if(fden != 0) {
					if(fden < 0) to_fixed_fraction = 0;
					else to_fixed_fraction = fden;
				} else {
					if(to_str[0] == '?') {
						to_prefix = 1;
					} else if(to_str.length() > 1 && to_str[1] == '?' && (to_str[0] == 'b' || to_str[0] == 'a' || to_str[0] == 'd')) {
						to_prefix = to_str[0];
					}
					Unit *u = CALCULATOR->getActiveUnit(to_str);
					if(delay_complex != (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS) && u && u->baseUnit() == CALCULATOR->getRadUnit() && u->baseExponent() == 1) delay_complex = !delay_complex;
					if(!str_conv.empty()) str_conv += " to ";
					str_conv += to_str;
				}
			}
			if(str_left.empty()) break;
			to_str = str_left;
		}
		if(do_to) {
			str = from_str;
			if(!str_conv.empty()) {
				str += " to ";
				str += str_conv;
			}
		}
	}

	if(!delay_complex || (cnf != COMPLEX_NUMBER_FORM_POLAR && cnf != COMPLEX_NUMBER_FORM_CIS)) {
		settings->evalops.complex_number_form = cnf;
		delay_complex = false;
	} else {
		settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
	}

	size_t i = str.find_first_of(SPACES LEFT_PARENTHESIS);
	if(i != std::string::npos) {
		to_str = str.substr(0, i);
		if(to_str == "factor" || equalsIgnoreCase(to_str, "factorize") || equalsIgnoreCase(to_str, tr("factorize").toStdString())) {
			str = str.substr(i + 1);
			do_factors = true;
		} else if(equalsIgnoreCase(to_str, "expand") || equalsIgnoreCase(to_str, tr("expand").toStdString())) {
			str = str.substr(i + 1);
			do_expand = true;
		}
	}

	if(!settings->simplified_percentage) settings->evalops.parse_options.parsing_mode = (ParsingMode) (settings->evalops.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);

	MathStructure mexact, mto, mparsed;
	mexact.setUndefined();
	settings->printops.allow_factorization = (settings->evalops.structuring == STRUCTURING_FACTORIZE);
	if(do_calendars || do_bases) mauto.setAborted();
	else mauto = CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options), settings->evalops, &mparsed, &mto);
	if(mauto.isAborted() || CALCULATOR->aborted() || mauto.countTotalChildren(false) > 500) {
		mauto.setAborted();
		result = "";
	}
	if(!mauto.isAborted()) {
		if(settings->dual_approximation > 0 || settings->printops.base == BASE_DECIMAL) {
			if(delay_complex) settings->evalops.complex_number_form = COMPLEX_NUMBER_FORM_RECTANGULAR;
			calculate_dual_exact(mexact, &mauto, CALCULATOR->unlocalizeExpression(str, settings->evalops.parse_options), &mparsed, settings->evalops, settings->dual_approximation != 0 ? AUTOMATIC_APPROXIMATION_SINGLE : AUTOMATIC_APPROXIMATION_OFF, 0, -1);
			mexact.setUndefined();
		}
		if(CALCULATOR->aborted()) {
			CALCULATOR->stopControl();
			CALCULATOR->startControl(20);
		}
		if(do_factors || do_pfe || do_expand) {
			if(do_factors) {
				if((mauto.isNumber() || mauto.isVector()) && to_fraction == 0 && to_fixed_fraction == 0) to_fraction = 2;
				if(!mauto.integerFactorize()) {
					mauto.structure(STRUCTURING_FACTORIZE, settings->evalops, true);
					settings->printops.allow_factorization = true;
				}
			} else if(do_pfe) {
				mauto.expandPartialFractions(settings->evalops);
			} else if(do_expand) {
				mauto.expand(settings->evalops);
				settings->printops.allow_factorization = false;
			}
			if(CALCULATOR->aborted() || mauto.countTotalChildren(false) > 500) mauto.setAborted();
		}
	}

	std::vector<std::string> values;
	if(!mauto.isAborted()) {
		// Always perform conversion to optimal (SI) unit when the expression is a number multiplied by a unit and input equals output
		if(mto.isUndefined() && !had_to_expression && (settings->evalops.approximation == APPROXIMATION_EXACT || settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) && ((mparsed.isMultiplication() && mparsed.size() == 2 && mparsed[0].isNumber() && mparsed[1].isUnit_exp() && mparsed.equals(mauto)) || (mparsed.isNegate() && mparsed[0].isMultiplication() && mparsed[0].size() == 2 && mparsed[0][0].isNumber() && mparsed[0][1].isUnit_exp() && mauto.isMultiplication() && mauto.size() == 2 && mauto[1] == mparsed[0][1] && mauto[0].isNumber() && mauto[0][0].number() == -mauto[0].number()) || (mparsed.isUnit_exp() && mparsed.equals(mauto)))) {
			Unit *u = NULL;
			MathStructure *munit = NULL;
			if(mauto.isMultiplication()) munit = &mauto[1];
			else munit = &mauto;
			if(munit->isUnit()) u = munit->unit();
			else u = (*munit)[0].unit();
			if(u && u->isCurrency()) {
				if(settings->evalops.local_currency_conversion && CALCULATOR->getLocalCurrency() && u != CALCULATOR->getLocalCurrency()) {
					ApproximationMode abak = settings->evalops.approximation;
					if(settings->evalops.approximation == APPROXIMATION_EXACT) settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
					mauto.set(CALCULATOR->convertToOptimalUnit(mauto, settings->evalops, true));
					settings->evalops.approximation = abak;
				}
			} else if(u && u->subtype() != SUBTYPE_BASE_UNIT && !u->isSIUnit()) {
				MathStructure mbak(mauto);
				if(settings->evalops.auto_post_conversion == POST_CONVERSION_OPTIMAL || settings->evalops.auto_post_conversion == POST_CONVERSION_NONE) {
					if(munit->isUnit() && u->referenceName() == "oF") {
						u = CALCULATOR->getActiveUnit("oC");
						if(u) mauto.set(CALCULATOR->convert(mauto, u, settings->evalops, true, false, false));
					} else if(munit->isUnit() && u->referenceName() == "oC") {
						u = CALCULATOR->getActiveUnit("oF");
						if(u) mauto.set(CALCULATOR->convert(mauto, u, settings->evalops, true, false, false));
					} else {
						mauto.set(CALCULATOR->convertToOptimalUnit(mauto, settings->evalops, true));
					}
				}
				if(settings->evalops.approximation == APPROXIMATION_EXACT && ((settings->evalops.auto_post_conversion != POST_CONVERSION_OPTIMAL && settings->evalops.auto_post_conversion != POST_CONVERSION_NONE) || mauto.equals(mbak))) {
					settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
					if(settings->evalops.auto_post_conversion == POST_CONVERSION_BASE) mauto.set(CALCULATOR->convertToBaseUnits(mauto, settings->evalops));
					else mauto.set(CALCULATOR->convertToOptimalUnit(mauto, settings->evalops, true));
					settings->evalops.approximation = APPROXIMATION_EXACT;
				}
			}
		}

		if(delay_complex) {
			settings->evalops.complex_number_form = cnf;
			if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_CIS) mauto.complexToCisForm(settings->evalops);
			else if(settings->evalops.complex_number_form == COMPLEX_NUMBER_FORM_POLAR) mauto.complexToPolarForm(settings->evalops);
		}

		Number save_nbase;
		bool custom_base_set = false;
		int save_base = settings->printops.base;
		bool caf_bak = settings->complex_angle_form;
		unsigned int save_bits = settings->printops.binary_bits;
		bool save_pre = settings->printops.use_unit_prefixes;
		bool save_cur = settings->printops.use_prefixes_for_currencies;
		bool save_allu = settings->printops.use_prefixes_for_all_units;
		bool save_all = settings->printops.use_all_prefixes;
		bool save_den = settings->printops.use_denominator_prefix;
		int save_bin = CALCULATOR->usesBinaryPrefixes();
		NumberFractionFormat save_format = settings->printops.number_fraction_format;
		long int save_fden = CALCULATOR->fixedDenominator();
		bool save_restrict_fraction_length = settings->printops.restrict_fraction_length;
		int save_dual = settings->dual_fraction;
		bool save_duo_syms = settings->printops.duodecimal_symbols;
		bool do_to = false;

		if(to_base != 0 || to_fraction > 0 || to_fixed_fraction >= 2 || to_prefix != 0 || (to_caf >= 0 && to_caf != settings->complex_angle_form)) {
			if(to_base != 0 && (to_base != settings->printops.base || to_bits != settings->printops.binary_bits || (to_base == BASE_CUSTOM && to_nbase != CALCULATOR->customOutputBase()) || (to_base == BASE_DUODECIMAL && to_duo_syms != settings->printops.duodecimal_symbols))) {
				settings->printops.base = to_base;
				settings->printops.binary_bits = to_bits;
				settings->printops.duodecimal_symbols = to_duo_syms;
				if(to_base == BASE_CUSTOM) {
					custom_base_set = true;
					save_nbase = CALCULATOR->customOutputBase();
					CALCULATOR->setCustomOutputBase(to_nbase);
				}
				do_to = true;
			}
			if(to_fixed_fraction >= 2) {
				if(to_fraction == 2 || (to_fraction < 0 && !contains_fraction_qt(mauto))) settings->printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
				else settings->printops.number_fraction_format = FRACTION_COMBINED_FIXED_DENOMINATOR;
				CALCULATOR->setFixedDenominator(to_fixed_fraction);
				settings->dual_fraction = 0;
				do_to = true;
			} else if(to_fraction > 0 && (settings->dual_fraction != 0 || settings->printops.restrict_fraction_length || (to_fraction != 2 && settings->printops.number_fraction_format != FRACTION_COMBINED) || (to_fraction == 2 && settings->printops.number_fraction_format != FRACTION_FRACTIONAL) || (to_fraction == 3 && settings->printops.number_fraction_format != FRACTION_DECIMAL))) {
				settings->printops.restrict_fraction_length = false;
				if(to_fraction == 3) settings->printops.number_fraction_format = FRACTION_DECIMAL;
				else if(to_fraction == 2) settings->printops.number_fraction_format = FRACTION_FRACTIONAL;
				else settings->printops.number_fraction_format = FRACTION_COMBINED;
				settings->dual_fraction = 0;
				do_to = true;
			}
			if(to_caf >= 0 && to_caf != settings->complex_angle_form) {
				settings->complex_angle_form = to_caf;
				do_to = true;
			}
			if(to_prefix != 0) {
				bool new_pre = settings->printops.use_unit_prefixes;
				bool new_cur = settings->printops.use_prefixes_for_currencies;
				bool new_allu = settings->printops.use_prefixes_for_all_units;
				bool new_all = settings->printops.use_all_prefixes;
				bool new_den = settings->printops.use_denominator_prefix;
				int new_bin = CALCULATOR->usesBinaryPrefixes();
				new_pre = true;
				if(to_prefix == 'b') {
					int i = has_information_unit(mauto);
					new_bin = (i > 0 ? 1 : 2);
					if(i == 1) {
						new_den = false;
					} else if(i > 1) {
						new_den = true;
					} else {
						new_cur = true;
						new_allu = true;
					}
				} else {
					new_cur = true;
					new_allu = true;
					if(to_prefix == 'a') new_all = true;
					else if(to_prefix == 'd') new_bin = 0;
				}
				if(settings->printops.use_unit_prefixes != new_pre || settings->printops.use_prefixes_for_currencies != new_cur || settings->printops.use_prefixes_for_all_units != new_allu || settings->printops.use_all_prefixes != new_all || settings->printops.use_denominator_prefix != new_den || CALCULATOR->usesBinaryPrefixes() != new_bin) {
					settings->printops.use_unit_prefixes = new_pre;
					settings->printops.use_all_prefixes = new_all;
					settings->printops.use_prefixes_for_currencies = new_cur;
					settings->printops.use_prefixes_for_all_units = new_allu;
					settings->printops.use_denominator_prefix = new_den;
					CALCULATOR->useBinaryPrefixes(new_bin);
					do_to = true;
				}
			}
		}

		PrintOptions po = settings->printops;
		po.allow_non_usable = true;
		print_dual(mauto, str, mparsed, mexact, result, values, po, settings->evalops, settings->dual_fraction == 0 ? AUTOMATIC_FRACTION_OFF : AUTOMATIC_FRACTION_SINGLE, settings->dual_approximation == 0 ? AUTOMATIC_APPROXIMATION_OFF : AUTOMATIC_APPROXIMATION_SINGLE, settings->complex_angle_form, NULL, true, settings->format_result, settings->color, TAG_TYPE_HTML, -1, had_to_expression);
		values.clear();
		if(do_to) {
			settings->complex_angle_form = caf_bak;
			settings->printops.base = save_base;
			settings->printops.duodecimal_symbols = save_duo_syms;
			settings->printops.binary_bits = save_bits;
			if(custom_base_set) CALCULATOR->setCustomOutputBase(save_nbase);
			settings->printops.use_unit_prefixes = save_pre;
			settings->printops.use_all_prefixes = save_all;
			settings->printops.use_prefixes_for_currencies = save_cur;
			settings->printops.use_prefixes_for_all_units = save_allu;
			settings->printops.use_denominator_prefix = save_den;
			CALCULATOR->useBinaryPrefixes(save_bin);
			settings->printops.number_fraction_format = save_format;
			CALCULATOR->setFixedDenominator(save_fden);
			settings->dual_fraction = save_dual;
			settings->printops.restrict_fraction_length = save_restrict_fraction_length;
			settings->evalops.complex_number_form = cnf_bak;
			settings->evalops.auto_post_conversion = save_auto_post_conversion;
			settings->evalops.parse_options.units_enabled = b_units_saved;
			settings->evalops.mixed_units_conversion = save_mixed_units_conversion;
			settings->printops.time_zone = TIME_ZONE_LOCAL;
		}
	}
	CALCULATOR->stopControl();

	if(!settings->simplified_percentage) settings->evalops.parse_options.parsing_mode = (ParsingMode) (settings->evalops.parse_options.parsing_mode & ~PARSE_PERCENT_AS_ORDINARY_CONSTANT);

	auto_format_updated = false;
	auto_calculation_updated = false;
	std::vector<CalculatorMessage> messages;
	CALCULATOR->endTemporaryStopMessages(false, &messages);
	bool b_error = false, b_warning = false;
	for(size_t i = 0; i < messages.size(); i++) {
		if(messages[i].type() == MESSAGE_ERROR) {
			b_error = true;
			break;
		} else if(messages[i].type() == MESSAGE_WARNING) {
			b_warning = true;
		}
	}

	if(!b_error && !b_warning && !auto_error && !auto_calculation_updated && !auto_format_updated && auto_result == result && (settings->auto_calculate_delay > 0 || auto_expression == current_status.toStdString())) {
		if(result.empty()) updateWindowTitle(QString(), true);
		return;
	}
	auto_expression = current_status.toStdString();
	auto_error = b_error || b_warning;
	QString flag;
	if(!b_error && !result.empty() && (mauto.countTotalChildren(false) < 10 || historyView->testTemporaryResultLength(result))) {
		auto_result = result;
		if((mauto.isMultiplication() && mauto.size() == 2 && mauto[1].isUnit() && mauto[1].unit()->isCurrency()) || (mauto.isUnit() && mauto.unit()->isCurrency())) {
			flag = ":/data/flags/" + QString::fromStdString(mauto.isUnit() ? mauto.unit()->referenceName() : mauto[1].unit()->referenceName()) + ".png";
			if(!QFile::exists(flag)) flag.clear();
		}
		gsub("\n", "<br>", result);
		if(mauto.isLogicalOr() && (mauto[0].isLogicalAnd() || mauto[0].isComparison())) {
			// add line break before or
			size_t i = 0;
			std::string or_str = " ";
			if(po.spell_out_logical_operators) or_str += CALCULATOR->logicalORString();
			else or_str += LOGICAL_OR;
			or_str += " ";
			while(true) {
				i = result.find(or_str, i);
				if(i == std::string::npos) break;
				result.replace(i + or_str.length() - 1, 1, "<br>");
				i += or_str.length();
			}
		}
		values.push_back(result);
	} else {
		auto_result = "";
		mauto.setAborted();
		if(settings->auto_calculate_delay > 0) {
			updateWindowTitle(QString(), true);
			return;
		}
	}
	auto_expression = current_status.toStdString();
	CALCULATOR->addMessages(&messages);
	historyView->addResult(values, "", true, auto_expression, !is_approximate && !mauto.isApproximate(), false, flag, NULL, 0, 0, true);
	updateWindowTitle(QString::fromStdString(unhtmlize(auto_result)), true);
}

void QalculateWindow::onBinaryBitsChanged() {
	if(basesDock->isVisible() && bases_is_result && settings->current_result) {
		CALCULATOR->beginTemporaryStopMessages();
		set_result_bases(*settings->current_result);
		CALCULATOR->endTemporaryStopMessages();
		updateResultBases();
	} else {
		onExpressionChanged();
	}
	updateBinEditSize();
}

std::string ellipsize_result(const std::string &result_text, size_t length) {
	length /= 2;
	size_t index1 = result_text.find(SPACE, length);
	if(index1 == std::string::npos || index1 > length * 1.2) {
		index1 = result_text.find(THIN_SPACE, length);
		size_t index1b = result_text.find(NNBSP, length);
		if(index1b != std::string::npos && (index1 == std::string::npos || index1b < index1)) index1 = index1b;
	}
	if(index1 == std::string::npos || index1 > length * 1.2) {
		index1 = length;
		while(index1 > 0 && (signed char) result_text[index1] < 0 && (unsigned char) result_text[index1 + 1] < 0xC0) index1--;
	}
	size_t index2 = result_text.find(SPACE, result_text.length() - length);
	if(index2 == std::string::npos || index2 > result_text.length() - length * 0.8) {
		index2 = result_text.find(THIN_SPACE, result_text.length() - length);
		size_t index2b = result_text.find(NNBSP, result_text.length() - length);
		if(index2b != std::string::npos && (index2 == std::string::npos || index2b < index2)) index2 = index2b;
	}
	if(index2 == std::string::npos || index2 > result_text.length() - length * 0.8) {
		index2 = result_text.length() - length;
		while(index2 > index1 && (signed char) result_text[index2] < 0 && (unsigned char) result_text[index2 + 1] < 0xC0) index2--;
	}
	return result_text.substr(0, index1) + " (…) " + result_text.substr(index2, result_text.length() - index2);
}

void ViewThread::run() {

	while(true) {

		void *x = NULL;
		if(!read(&x) || !x) break;
		MathStructure *mresult = (MathStructure*) x;
		x = NULL;
		bool b_stack = false;
		if(!read(&b_stack)) break;
		if(!read(&x)) break;
		MathStructure *mm = (MathStructure*) x;
		if(!read(&x)) break;
		MathStructure *mparse = (MathStructure*) x;
		CALCULATOR->startControl();
		PrintOptions po;
		if(mparse) {
			if(!read(&po.is_approximate)) break;
			if(!read<bool>(&po.preserve_format)) break;
			po.show_ending_zeroes = settings->evalops.parse_options.read_precision != DONT_READ_PRECISION && CALCULATOR->usesIntervalArithmetic() && settings->evalops.parse_options.base > BASE_CUSTOM;
			po.exp_display = settings->printops.exp_display;
			po.lower_case_numbers = settings->printops.lower_case_numbers;
			po.rounding = settings->printops.rounding;
			po.base_display = settings->printops.base_display;
			po.twos_complement = settings->printops.twos_complement;
			po.hexadecimal_twos_complement = settings->printops.hexadecimal_twos_complement;
			po.base = settings->evalops.parse_options.base;
			po.allow_non_usable = true;
			Number nr_base;
			if(po.base == BASE_CUSTOM && (CALCULATOR->usesIntervalArithmetic() || CALCULATOR->customInputBase().isRational()) && (CALCULATOR->customInputBase().isInteger() || !CALCULATOR->customInputBase().isNegative()) && (CALCULATOR->customInputBase() > 1 || CALCULATOR->customInputBase() < -1)) {
				nr_base = CALCULATOR->customOutputBase();
				CALCULATOR->setCustomOutputBase(CALCULATOR->customInputBase());
			} else if(po.base == BASE_CUSTOM || (po.base < BASE_CUSTOM && !CALCULATOR->usesIntervalArithmetic() && po.base != BASE_UNICODE)) {
				po.base = 10;
				po.min_exp = 6;
				po.use_max_decimals = true;
				po.max_decimals = 5;
				po.preserve_format = false;
			}
			po.abbreviate_names = false;
			po.digit_grouping = settings->printops.digit_grouping;
			po.use_unicode_signs = settings->printops.use_unicode_signs;
			po.multiplication_sign = settings->printops.multiplication_sign;
			po.division_sign = settings->printops.division_sign;
			po.short_multiplication = false;
			po.excessive_parenthesis = true;
			po.improve_division_multipliers = false;
			po.restrict_to_parent_precision = false;
			po.spell_out_logical_operators = settings->printops.spell_out_logical_operators;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			MathStructure mp(*mparse);
			mp.format(po);
			parsed_text = mp.print(po, settings->format_result, settings->color, TAG_TYPE_HTML);
			if(po.base == BASE_CUSTOM) {
				CALCULATOR->setCustomOutputBase(nr_base);
			}
		}

		if(mm && mresult->isMatrix()) {
			PrintOptions po = settings->printops;
			po.allow_non_usable = false;
			mm->set(*mresult);
			MathStructure mm2(*mresult);
			std::string mstr;
			int c = mm->columns(), r = mm->rows();
			for(int index_r = 0; index_r < r; index_r++) {
				for(int index_c = 0; index_c < c; index_c++) {
					mm->getElement(index_r + 1, index_c + 1)->setAborted();
				}
			}
			for(int index_r = 0; index_r < r; index_r++) {
				for(int index_c = 0; index_c < c; index_c++) {
					mm2.getElement(index_r + 1, index_c + 1)->format(po);
					mstr = mm2.getElement(index_r + 1, index_c + 1)->print(po);
					mm->getElement(index_r + 1, index_c + 1)->set(mstr);
				}
			}
		}

		po = settings->printops;

		po.allow_non_usable = true;

		print_dual(*mresult, original_expression, mparse ? *mparse : *parsed_mstruct, mstruct_exact, result_text, alt_results, po, settings->evalops, settings->dual_fraction < 0 ? AUTOMATIC_FRACTION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_FRACTION_DUAL : AUTOMATIC_FRACTION_OFF), settings->dual_approximation < 0 ? AUTOMATIC_APPROXIMATION_AUTO : (settings->dual_fraction > 0 ? AUTOMATIC_APPROXIMATION_DUAL : AUTOMATIC_APPROXIMATION_OFF), settings->complex_angle_form, &exact_comparison, mparse != NULL, settings->format_result, settings->color, TAG_TYPE_HTML, -1, had_to_expression);

		if(!prepend_mstruct.isUndefined() && !CALCULATOR->aborted()) {
			prepend_mstruct.format(po);
			po.min_exp = 0;
			alt_results.insert(alt_results.begin(), prepend_mstruct.print(po, settings->format_result, settings->color, TAG_TYPE_HTML));
		}

		if(!b_stack) {
			set_result_bases(*mresult);
			bases_is_result = true;
		}

		for(size_t i = 0; i < alt_results.size();) {
			if(alt_results[i].length() > 50000) alt_results.erase(alt_results.begin() + i);
			else i++;
		}
		if(mresult->isMatrix() && mresult->rows() * mresult->columns() > 500) {
			if(result_text.length() > 1000000L) {
				result_text = "matrix ("; result_text += i2s(mresult->rows()); result_text += SIGN_MULTIPLICATION; result_text += i2s(mresult->columns()); result_text += ")";
			} else {
				std::string str = unhtmlize(result_text);
				if(str.length() > 5000) {
					result_text = ellipsize_result(str, 1000);
				}
			}
		} else if(result_text.length() > 50000) {
			if(mstruct->isNumber())	{
				result_text = ellipsize_result(result_text, 5000);
			} else {
				std::string str = unhtmlize(result_text);
				if(str.length() > 20000) {
					result_text = ellipsize_result(result_text, 2000);
				}
			}
		}

		if(mresult->isLogicalOr() && (mresult->getChild(1)->isLogicalAnd() || mresult->getChild(1)->isComparison())) {
			// add line break before or
			size_t i = 0;
			std::string or_str = " ";
			if(po.spell_out_logical_operators) or_str += CALCULATOR->logicalORString();
			else or_str += LOGICAL_OR;
			or_str += " ";
			while(true) {
				i = result_text.find(or_str, i);
				if(i == std::string::npos) break;
				result_text.replace(i + or_str.length() - 1, 1, "<br>");
				i += or_str.length();
			}
		}

		b_busy--;
		CALCULATOR->stopControl();

	}
}

int intervals_are_relative(MathStructure &m) {
	int ret = -1;
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_UNCERTAINTY && m.size() == 3) {
		if(m[2].isOne() && m[1].isMultiplication() && m[1].size() > 1 && m[1].last().isVariable() && (m[1].last().variable() == CALCULATOR->getVariableById(VARIABLE_ID_PERCENT) || m[1].last().variable() == CALCULATOR->getVariableById(VARIABLE_ID_PERMILLE) || m[1].last().variable() == CALCULATOR->getVariableById(VARIABLE_ID_PERMYRIAD))) {
			ret = 1;
		} else {
			return 0;
		}
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_INTERVAL) return 0;
	for(size_t i = 0; i < m.size(); i++) {
		int ret_i = intervals_are_relative(m[i]);
		if(ret_i == 0) return 0;
		else if(ret_i > 0) ret = ret_i;
	}
	return ret;
}

void QalculateWindow::setResult(Prefix *prefix, bool update_history, bool update_parse, bool force, std::string transformation, bool do_stack, size_t stack_index, bool register_moved, bool supress_dialog) {

	if(block_result_update) return;

	if(expressionEdit->expressionHasChanged() && (!settings->rpn_mode || CALCULATOR->RPNStackSize() == 0)) {
		if(!force) return;
		calculateExpression();
		if(!prefix) return;
	}

	if(settings->rpn_mode && CALCULATOR->RPNStackSize() == 0) return;

	if(!settings->rpn_mode) {stack_index = 0; do_stack = false;}

	if(!do_stack && (settings->history_answer.empty() || !settings->current_result) && !register_moved && !update_parse && update_history) {
		return;
	}

	if(b_busy) return;

	workspace_changed = true;

	std::string prev_result_text = result_text;
	bool prev_approximate = *settings->printops.is_approximate;

	if(update_parse) {
		parsed_text = "aborted";
	}

	if(do_stack) {
		update_history = true;
		update_parse = false;
	}
	if(register_moved) {
		update_history = true;
		update_parse = false;
	}

	if(update_parse && parsed_mstruct && parsed_mstruct->isFunction() && (parsed_mstruct->function()->id() == FUNCTION_ID_ERROR || parsed_mstruct->function()->id() == FUNCTION_ID_WARNING || parsed_mstruct->function()->id() == FUNCTION_ID_MESSAGE)) {
		expressionEdit->clear();
		historyView->addMessages();
		return;
	}

	b_busy++;

	if(!viewThread->running && !viewThread->start()) {b_busy--; return;}

	bool b_rpn_operation = false;

	if(update_history) {
		if(update_parse || register_moved) {
			if(register_moved) {
				result_text = tr("RPN Register Moved").toStdString();
			} else {
				remove_blank_ends(result_text);
				gsub("\n", " ", result_text);
				if(result_text == tr("RPN Operation").toStdString()) {
					b_rpn_operation = true;
				} else {
					if(settings->adaptive_interval_display) {
						QString expression_str = expressionEdit->toPlainText();
						if((parsed_mstruct && parsed_mstruct->containsFunctionId(FUNCTION_ID_UNCERTAINTY)) || expression_str.contains("+/-") || expression_str.contains("+/" SIGN_MINUS) || expression_str.contains("±")) {
							if(parsed_mstruct && intervals_are_relative(*parsed_mstruct) > 0) settings->printops.interval_display = INTERVAL_DISPLAY_RELATIVE;
							else settings->printops.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
						} else if(parsed_mstruct && parsed_mstruct->containsFunctionId(FUNCTION_ID_INTERVAL)) settings->printops.interval_display = INTERVAL_DISPLAY_INTERVAL;
						else settings->printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
					}
				}
			}
		} else if(!transformation.empty()) {
		}
		result_text = "?";
	}

	if(update_parse) {
		parsed_text = "aborted";
	}

	bool parsed_approx = false;
	bool title_set = false, was_busy = false;

	Number save_nbase;
	bool custom_base_set = false;
	int save_base = settings->printops.base;
	bool caf_bak = settings->complex_angle_form;
	unsigned int save_bits = settings->printops.binary_bits;
	bool save_pre = settings->printops.use_unit_prefixes;
	bool save_cur = settings->printops.use_prefixes_for_currencies;
	bool save_allu = settings->printops.use_prefixes_for_all_units;
	bool save_all = settings->printops.use_all_prefixes;
	bool save_den = settings->printops.use_denominator_prefix;
	int save_bin = CALCULATOR->usesBinaryPrefixes();
	NumberFractionFormat save_format = settings->printops.number_fraction_format;
	long int save_fden = CALCULATOR->fixedDenominator();
	bool save_restrict_fraction_length = settings->printops.restrict_fraction_length;
	int save_dual = settings->dual_fraction;
	bool save_duo_syms = settings->printops.duodecimal_symbols;
	bool do_to = false;

	if(!do_stack) {
		if(to_base != 0 || to_fraction > 0 || to_fixed_fraction >= 2 || to_prefix != 0 || (to_caf >= 0 && to_caf != settings->complex_angle_form)) {
			if(to_base != 0 && (to_base != settings->printops.base || to_bits != settings->printops.binary_bits || (to_base == BASE_CUSTOM && to_nbase != CALCULATOR->customOutputBase()) || (to_base == BASE_DUODECIMAL && to_duo_syms != settings->printops.duodecimal_symbols))) {
				settings->printops.base = to_base;
				settings->printops.binary_bits = to_bits;
				settings->printops.duodecimal_symbols = to_duo_syms;
				if(to_base == BASE_CUSTOM) {
					custom_base_set = true;
					save_nbase = CALCULATOR->customOutputBase();
					CALCULATOR->setCustomOutputBase(to_nbase);
				}
				do_to = true;
			}
			if(to_fixed_fraction >= 2) {
				if(to_fraction == 2 || (to_fraction < 0 && !contains_fraction_qt(*mstruct))) settings->printops.number_fraction_format = FRACTION_FRACTIONAL_FIXED_DENOMINATOR;
				else settings->printops.number_fraction_format = FRACTION_COMBINED_FIXED_DENOMINATOR;
				CALCULATOR->setFixedDenominator(to_fixed_fraction);
				settings->dual_fraction = 0;
				do_to = true;
			} else if(to_fraction > 0 && (settings->dual_fraction != 0 || settings->printops.restrict_fraction_length || (to_fraction != 2 && settings->printops.number_fraction_format != FRACTION_COMBINED) || (to_fraction == 2 && settings->printops.number_fraction_format != FRACTION_FRACTIONAL) || (to_fraction == 3 && settings->printops.number_fraction_format != FRACTION_DECIMAL))) {
				settings->printops.restrict_fraction_length = false;
				if(to_fraction == 3) settings->printops.number_fraction_format = FRACTION_DECIMAL;
				else if(to_fraction == 2) settings->printops.number_fraction_format = FRACTION_FRACTIONAL;
				else settings->printops.number_fraction_format = FRACTION_COMBINED;
				settings->dual_fraction = 0;
				do_to = true;
			}
			if(to_caf >= 0 && to_caf != settings->complex_angle_form) {
				settings->complex_angle_form = to_caf;
				do_to = true;
			}
			if(to_prefix != 0 && !prefix) {
				bool new_pre = settings->printops.use_unit_prefixes;
				bool new_cur = settings->printops.use_prefixes_for_currencies;
				bool new_allu = settings->printops.use_prefixes_for_all_units;
				bool new_all = settings->printops.use_all_prefixes;
				bool new_den = settings->printops.use_denominator_prefix;
				int new_bin = CALCULATOR->usesBinaryPrefixes();
				new_pre = true;
				if(to_prefix == 'b') {
					int i = has_information_unit(*mstruct);
					new_bin = (i > 0 ? 1 : 2);
					if(i == 1) {
						new_den = false;
					} else if(i > 1) {
						new_den = true;
					} else {
						new_cur = true;
						new_allu = true;
					}
				} else {
					new_cur = true;
					new_allu = true;
					if(to_prefix == 'a') new_all = true;
					else if(to_prefix == 'd') new_bin = 0;
				}
				if(settings->printops.use_unit_prefixes != new_pre || settings->printops.use_prefixes_for_currencies != new_cur || settings->printops.use_prefixes_for_all_units != new_allu || settings->printops.use_all_prefixes != new_all || settings->printops.use_denominator_prefix != new_den || CALCULATOR->usesBinaryPrefixes() != new_bin) {
					settings->printops.use_unit_prefixes = new_pre;
					settings->printops.use_all_prefixes = new_all;
					settings->printops.use_prefixes_for_currencies = new_cur;
					settings->printops.use_prefixes_for_all_units = new_allu;
					settings->printops.use_denominator_prefix = new_den;
					CALCULATOR->useBinaryPrefixes(new_bin);
					do_to = true;
				}
			}
		}
	}

	settings->printops.prefix = prefix;

	if(!do_stack) {
		if(!viewThread->write((void*) mstruct)) {b_busy--; viewThread->cancel(); return;}
	} else {
		MathStructure *mreg = CALCULATOR->getRPNRegister(stack_index + 1);
		if(!viewThread->write((void*) mreg)) {b_busy--; viewThread->cancel(); return;}
	}
	if(!viewThread->write(do_stack)) {b_busy--; viewThread->cancel(); return;}
	if(do_stack) {
		if(!viewThread->write((void*) NULL)) {b_busy--; viewThread->cancel(); return;}
	} else {
		matrix_mstruct.clear();
		if(!mstruct->isMatrix() || mstruct->rows() * mstruct->columns() <= 16 || mstruct->columns() * mstruct->rows() > 10000) {
			if(!viewThread->write((void*) NULL)) {b_busy--; viewThread->cancel(); return;}
		} else {
			if(!viewThread->write((void*) &matrix_mstruct)) {b_busy--; viewThread->cancel(); return;}
		}
	}
	if(update_parse) {
		if(!viewThread->write((void*) parsed_mstruct)) {b_busy--; viewThread->cancel(); return;}
		bool *parsed_approx_p = &parsed_approx;
		if(!viewThread->write(parsed_approx_p)) {b_busy--; viewThread->cancel(); return;}
		if(!viewThread->write(!b_rpn_operation)) {b_busy--; viewThread->cancel(); return;}
	} else {
		if(settings->printops.base != BASE_DECIMAL && settings->dual_approximation <= 0) mstruct_exact.setUndefined();
		if(!viewThread->write((void*) NULL)) {b_busy--; viewThread->cancel(); return;}
	}

	QProgressDialog *dialog = NULL;

	int i = 0;
	while(b_busy && viewThread->running && i < 50) {
		sleep_ms(10);
		i++;
	}
	i = 0;

	if(b_busy && viewThread->running) {
		if(!do_stack && updateWindowTitle(tr("Processing…"))) title_set = true;
		dialog = new QProgressDialog(tr("Processing…"), tr("Cancel"), 0, 0, this);
		dialog->setWindowTitle(tr("Processing…"));
		connect(dialog, SIGNAL(canceled()), this, SLOT(abort()));
		dialog->setWindowModality(Qt::WindowModal);
		dialog->show();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		was_busy = true;
	}
	while(b_busy && viewThread->running) {
		qApp->processEvents();
		sleep_ms(100);
	}
	b_busy++;

	if(was_busy) {
		QApplication::restoreOverrideCursor();
		dialog->hide();
		dialog->deleteLater();
		if(title_set) updateWindowTitle();
	}

	if(!do_stack) {
		if(basesDock->isVisible()) updateResultBases();
		updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true);
	}
	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}
	if(settings->history_answer.empty()) {
		update_parse = true;
	}
	if(do_stack) {
		RPNRegisterChanged(result_text, stack_index);
		if(supress_dialog) CALCULATOR->clearMessages();
		else displayMessages();
	} else if(update_history) {
		if(update_parse) {}
	} else {
		if(supress_dialog) CALCULATOR->clearMessages();
		else displayMessages();
	}

	if(register_moved) {
		update_parse = true;
		parsed_text = result_text;
	}

	bool implicit_warning = false;

	if(do_stack) {
		RPNRegisterChanged(result_text, stack_index);
	} else {
		if(settings->rpn_mode && !register_moved) {
			RPNRegisterChanged(result_text, stack_index);
		}
		if(mstruct->isAborted() || settings->printops.base != settings->evalops.parse_options.base || (settings->printops.base > 32 || settings->printops.base < 2)) {
			exact_text = "";
		} else if(!mstruct->isApproximate() && !(*settings->printops.is_approximate)) {
			exact_text = unhtmlize(result_text);
		} else if(!alt_results.empty()) {
			exact_text = unhtmlize(alt_results[0]);
		} else {
			exact_text = "";
		}
		alt_results.push_back(result_text);
		for(size_t i = 0; i < alt_results.size(); i++) {
			gsub("\n", "<br>", alt_results[i]);
		}
		QString flag;
		if((mstruct->isMultiplication() && mstruct->size() == 2 && (*mstruct)[1].isUnit() && (*mstruct)[1].unit()->isCurrency()) || (mstruct->isUnit() && mstruct->unit()->isCurrency())) {
			flag = ":/data/flags/" + QString::fromStdString(mstruct->isUnit() ? mstruct->unit()->referenceName() : (*mstruct)[1].unit()->referenceName()) + ".png";
			if(!QFile::exists(flag)) flag.clear();
		}
		int b_exact = (update_parse || !prev_approximate) && (exact_comparison || (!(*settings->printops.is_approximate) && !mstruct->isApproximate()));
		if(alt_results.size() == 1 && (mstruct->isComparison() || ((mstruct->isLogicalAnd() || mstruct->isLogicalOr()) && mstruct->containsType(STRUCT_COMPARISON, true, false, false))) && (exact_comparison || b_exact || result_text.find(SIGN_ALMOST_EQUAL) != std::string::npos)) b_exact = -1;
		size_t index = settings->v_expression.size();
		bool b_add = true;
		if(index > 0 && settings->current_result && !CALCULATOR->message() && (!update_parse || (settings->history_answer.size() > (mstruct_exact.isUndefined() ? 1 : 2) && !settings->rpn_mode && mstruct->equals(*settings->history_answer[settings->history_answer.size() - 1], true, true) && (mstruct_exact.isUndefined() || (settings->history_answer.size() > 1 && mstruct_exact.equals(*settings->history_answer[settings->history_answer.size() - 2], true, true))) && parsed_text == settings->v_parse[index - 1] && prev_result_text == settings->v_expression[index - 1] && parsed_approx != settings->v_pexact[index - 1] && !contains_rand_function(*parsed_mstruct))) && alt_results.size() <= settings->v_result[index - 1].size()) {
			b_add = false;
			for(size_t i = 0; i < alt_results.size(); i++) {
				if(settings->v_exact[index - 1][i] != (b_exact || i < alt_results.size() - 1) || settings->v_result[index - 1][i] != alt_results[i]) {
					b_add = true; break;
				}
			}
		}
		if(b_add) {
			auto_expression = "";
			auto_result = "";
			mauto.setAborted();
			auto_error = false;
			if(autoCalculateTimer) autoCalculateTimer->stop();
			historyView->addResult(alt_results, update_parse ? prev_result_text : "", !parsed_approx, update_parse ? parsed_text : "", b_exact, alt_results.size() > 1 && !mstruct_exact.isUndefined(), flag, !supress_dialog && update_parse && settings->evalops.parse_options.parsing_mode <= PARSING_MODE_CONVENTIONAL && update_history ? &implicit_warning : NULL);
		} else if(update_parse) {
			settings->history_answer.pop_back();
			if(!mstruct_exact.isUndefined()) settings->history_answer.pop_back();
			historyView->clearTemporary();
		}
	}

	if(do_to) {
		settings->complex_angle_form = caf_bak;
		settings->printops.base = save_base;
		settings->printops.duodecimal_symbols = save_duo_syms;
		settings->printops.binary_bits = save_bits;
		if(custom_base_set) CALCULATOR->setCustomOutputBase(save_nbase);
		settings->printops.use_unit_prefixes = save_pre;
		settings->printops.use_all_prefixes = save_all;
		settings->printops.use_prefixes_for_currencies = save_cur;
		settings->printops.use_prefixes_for_all_units = save_allu;
		settings->printops.use_denominator_prefix = save_den;
		CALCULATOR->useBinaryPrefixes(save_bin);
		settings->printops.number_fraction_format = save_format;
		CALCULATOR->setFixedDenominator(save_fden);
		settings->dual_fraction = save_dual;
		settings->printops.restrict_fraction_length = save_restrict_fraction_length;
	}
	settings->printops.prefix = NULL;

	settings->current_result = mstruct;

	b_busy--;

	if(implicit_warning && askImplicit()) {
		calculateExpression(true);
		return;
	}

	if(!supress_dialog && !register_moved && !do_stack && mstruct->isMatrix() && matrix_mstruct.isMatrix()) {
		QDialog *dialog = new QDialog(this);
		if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
		dialog->setWindowTitle(tr("Matrix"));
		QVBoxLayout *box = new QVBoxLayout(dialog);
		MatrixWidget *w = new MatrixWidget(dialog);
		w->setMatrixStrings(matrix_mstruct);
		w->setEditable(false);
		box->addWidget(w);
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, dialog);
		box->addWidget(buttonBox);
		connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), dialog, SLOT(reject()));
		dialog->exec();
		dialog->deleteLater();
	}

}

bool QalculateWindow::eventFilter(QObject*, QEvent *e) {
	if(e->type() == QEvent::ToolTip) return true;
	return false;
}

void QalculateWindow::onColorSchemeChanged() {
#if defined _WIN32 && (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
	settings->updatePalette(true);
#endif
}
void QalculateWindow::changeEvent(QEvent *e) {
	if(e->type() == QEvent::PaletteChange || e->type() == QEvent::ApplicationPaletteChange) {
		QColor c = QApplication::palette().base().color();
		if(c.red() + c.green() + c.blue() < 255) settings->color = 2;
		else settings->color = 1;
		menuAction_t->setIcon(LOAD_ICON("menu"));
		toAction_t->setIcon(LOAD_ICON("convert"));
		storeAction_t->setIcon(LOAD_ICON("document-save"));
		functionsAction_t->setIcon(LOAD_ICON("function"));
		unitsAction_t->setIcon(LOAD_ICON("units"));
		if(plotAction_t) plotAction_t->setIcon(LOAD_ICON("plot"));
		keypadAction_t->setIcon(LOAD_ICON("keypad"));
		basesAction->setIcon(LOAD_ICON("number-bases"));
		modeAction_t->setIcon(LOAD_ICON("configure"));
		rpnUpAction->setIcon(LOAD_ICON("go-up"));
		rpnDownAction->setIcon(LOAD_ICON("go-down"));
		rpnSwapAction->setIcon(LOAD_ICON("rpn-swap"));
		rpnCopyAction->setIcon(LOAD_ICON("edit-copy"));
		rpnLastxAction->setIcon(LOAD_ICON("edit-undo"));
		rpnDeleteAction->setIcon(LOAD_ICON("edit-delete"));
		rpnClearAction->setIcon(LOAD_ICON("edit-clear"));
	} else if(e->type() == QEvent::FontChange || e->type() == QEvent::ApplicationFontChange) {
		QFont font(QApplication::font());
		updateBinEditSize(&font);
		if(!settings->use_custom_expression_font) {
			if(font.pixelSize() >= 0) font.setPixelSize(font.pixelSize() * 1.35);
			else font.setPointSize(font.pointSize() * 1.35);
			expressionEdit->setFont(font);
		}
	}
	QMainWindow::changeEvent(e);
}

void QalculateWindow::updateBinEditSize(QFont *font) {
	QFontMetrics fm2(font ? *font : binEdit->font());
	SET_BINARY_BITS
	int rows = binary_bits / 32;
	if(binary_bits % 32 > 0) rows++;
	QString row_string;
	for(int i = binary_bits > 32 ? 32 : binary_bits; i > 0; i -= 4) {
		if(i < 32 && i < binary_bits) row_string += " ";
		row_string += "0000";
	}
	binEdit->setMinimumWidth(fm2.boundingRect(row_string).width() + binEdit->frameWidth() * 2 + binEdit->contentsMargins().left() + binEdit->contentsMargins().right());
	binEdit->setMinimumHeight(fm2.lineSpacing() * rows * 2 + binEdit->frameWidth() * 2 + binEdit->contentsMargins().top() + binEdit->contentsMargins().bottom());
}

void QalculateWindow::fetchExchangeRates() {
	CALCULATOR->clearMessages();
	settings->fetchExchangeRates(15, -1, this);
	CALCULATOR->loadExchangeRates();
	displayMessages();
	expressionCalculationUpdated();
}
void QalculateWindow::abort() {
	CALCULATOR->abort();
}
void QalculateWindow::abortCommand() {
	CALCULATOR->abort();
	int msecs = 5000;
	while(b_busy && msecs > 0) {
		sleep_ms(10);
		msecs -= 10;
	}
	if(b_busy) {
		commandThread->cancel();
		b_busy--;
		CALCULATOR->stopControl();
		command_aborted = true;
	}
}
bool contains_temperature_unit_qt(const MathStructure &m) {
	if(m.isUnit()) {
		return m.unit() == CALCULATOR->getUnitById(UNIT_ID_CELSIUS) || m.unit() == CALCULATOR->getUnitById(UNIT_ID_FAHRENHEIT);
	}
	if(m.isVariable() && m.variable()->isKnown()) {
		return contains_temperature_unit_qt(((KnownVariable*) m.variable())->get());
	}
	if(m.isFunction() && m.function()->id() == FUNCTION_ID_STRIP_UNITS) return false;
	for(size_t i = 0; i < m.size(); i++) {
		if(contains_temperature_unit_qt(m[i])) return true;
	}
	return false;
}
bool QalculateWindow::askTC(MathStructure &m) {
	if(settings->tc_set || !contains_temperature_unit_qt(m)) return false;
	MathStructure *mp = &m;
	if(m.isMultiplication() && m.size() == 2 && m[0].isMinusOne()) mp = &m[1];
	else if(m.isNegate()) mp = &m[0];
	if(mp->isUnit_exp()) return false;
	if(mp->isMultiplication() && mp->size() > 0 && mp->last().isUnit_exp()) {
		bool b = false;
		for(size_t i = 0; i < mp->size() - 1; i++) {
			if(contains_temperature_unit_qt((*mp)[i])) {b = true; break;}
		}
		if(!b) return false;
	}
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Temperature Calculation Mode"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("The expression is ambiguous.\nPlease select temperature calculation mode\n(the mode can later be changed in preferences).")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QRadioButton *w_abs = new QRadioButton(tr("Absolute"));
	group->addButton(w_abs);
	grid->addWidget(w_abs, 1, 0, Qt::AlignTop);
	grid->addWidget(new QLabel("<i>1 °C + 1 °C ≈ 274 K + 274 K ≈ 548 K<br>1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K<br>2 °C − 1 °C = 1 K<br>1 °C − 5 °F = 16 K<br>1 °C + 1 K = 2 °C</i>"), 1, 1);
	QRadioButton *w_relative = new QRadioButton(tr("Relative"));
	group->addButton(w_relative);
	grid->addWidget(w_relative, 2, 0, Qt::AlignTop);
	grid->addWidget(new QLabel("<i>1 °C + 1 °C = 2 °C<br>1 °C + 5 °F = 1 °C + 5 °R ≈ 4 °C ≈ 277 K<br>2 °C − 1 °C = 1 °C<br>1 °C − 5 °F = 1 °C - 5 °R ≈ −2 °C<br>1 °C + 1 K = 2 °C</i>"), 2, 1);
	QRadioButton *w_hybrid = new QRadioButton(tr("Hybrid"));
	group->addButton(w_hybrid);
	grid->addWidget(w_hybrid, 3, 0, Qt::AlignTop);
	grid->addWidget(new QLabel("<i>1 °C + 1 °C ≈ 2 °C<br>1 °C + 5 °F ≈ 274 K + 258 K ≈ 532 K<br>2 °C − 1 °C = 1 °C<br>1 °C − 5 °F = 16 K<br>1 °C + 1 K = 2 °C</i>"), 3, 1);
	switch(CALCULATOR->getTemperatureCalculationMode()) {
		case TEMPERATURE_CALCULATION_ABSOLUTE: {w_abs->setChecked(true); break;}
		case TEMPERATURE_CALCULATION_RELATIVE: {w_relative->setChecked(true); break;}
		default: {w_hybrid->setChecked(true); break;}
	}
	dialog->exec();
	TemperatureCalculationMode tc_mode = TEMPERATURE_CALCULATION_HYBRID;
	if(w_abs->isChecked()) tc_mode = TEMPERATURE_CALCULATION_ABSOLUTE;
	else if(w_relative->isChecked()) tc_mode = TEMPERATURE_CALCULATION_RELATIVE;
	dialog->deleteLater();
	settings->tc_set = true;
	if(tc_mode != CALCULATOR->getTemperatureCalculationMode()) {
		CALCULATOR->setTemperatureCalculationMode(tc_mode);
		if(preferencesDialog) preferencesDialog->updateTemperatureCalculation();
		return true;
	}
	return false;
}
bool QalculateWindow::askSinc(MathStructure &m) {
	if(settings->sinc_set || !m.containsFunctionId(FUNCTION_ID_SINC)) return false;
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Temperature Calculation Mode"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("Please select desired variant of the sinc function.")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QRadioButton *w_1 = new QRadioButton(tr("Unnormalized"));
	group->addButton(w_1);
	grid->addWidget(w_1, 1, 0, Qt::AlignTop);
	grid->addWidget(new QLabel("<i>sinc(x) = sin(x)/x</i>"), 1, 1);
	QRadioButton *w_pi = new QRadioButton(tr("Normalized"));
	group->addButton(w_pi);
	grid->addWidget(w_pi, 2, 0, Qt::AlignTop);
	grid->addWidget(new QLabel("<i>sinc(x) = sinc(πx)/(πx)</i>"), 2, 1);
	w_1->setChecked(true);
	dialog->exec();
	bool b_pi = w_pi->isChecked();
	dialog->deleteLater();
	settings->sinc_set = true;
	if(b_pi) {
		CALCULATOR->getFunctionById(FUNCTION_ID_SINC)->setDefaultValue(2, "pi");
		return true;
	}
	return false;
}
bool QalculateWindow::askDot(const std::string &str) {
	if(settings->dot_question_asked || CALCULATOR->getDecimalPoint() == DOT) return false;
	size_t i = 0;
	bool b = false;
	while(true) {
		i = str.find(DOT, i);
		if(i == std::string::npos) return false;
		i = str.find_first_not_of(SPACES, i + 1);
		if(i == std::string::npos) return false;
		if(is_in(NUMBERS, str[i])) {
			b = true;
			break;
		}
	}
	if(!b) return false;
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Interpretation of dots"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("Please select interpretation of dots (\".\")\n(this can later be changed in preferences).")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QRadioButton *w_bothdeci = new QRadioButton(tr("Both dot and comma as decimal separators"));
	group->addButton(w_bothdeci);
	grid->addWidget(w_bothdeci, 1, 0);
	grid->addWidget(new QLabel("<i>(1.2 = 1,2)</i>"), 1, 1);
	QRadioButton *w_ignoredot = new QRadioButton(tr("Dot as thousands separator"));
	group->addButton(w_ignoredot);
	grid->addWidget(w_ignoredot, 2, 0);
	grid->addWidget(new QLabel("<i>(1.000.000 = 1000000)</i>"), 2, 1);
	QRadioButton *w_dotdeci = new QRadioButton(tr("Only dot as decimal separator"));
	group->addButton(w_dotdeci);
	grid->addWidget(w_dotdeci, 3, 0);
	grid->addWidget(new QLabel("<i>(1.2 + root(16, 4) = 3.2)</i>"), 3, 1);
	if(settings->evalops.parse_options.dot_as_separator) w_ignoredot->setChecked(true);
	else w_bothdeci->setChecked(true);
	dialog->exec();
	settings->dot_question_asked = true;
	bool das = settings->evalops.parse_options.dot_as_separator;
	if(w_dotdeci->isChecked()) {
		settings->evalops.parse_options.dot_as_separator = false;
		settings->evalops.parse_options.comma_as_separator = false;
		settings->decimal_comma = false;
		CALCULATOR->useDecimalPoint(false);
		das = !settings->evalops.parse_options.dot_as_separator;
	} else if(w_ignoredot->isChecked()) {
		settings->evalops.parse_options.dot_as_separator = true;
	} else {
		settings->evalops.parse_options.dot_as_separator = false;
	}
	if(preferencesDialog) preferencesDialog->updateDot();
	dialog->deleteLater();
	return das != settings->evalops.parse_options.dot_as_separator;
}
bool QalculateWindow::askImplicit() {
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Parsing Mode"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("The expression is ambiguous.\nPlease select interpretation of expressions with implicit multiplication\n(this can later be changed in preferences).")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QLabel *label = new QLabel("<i>1/2x = 1/(2x)</i>");
	int h = label->sizeHint().height();
	QRadioButton *w_implicitfirst = new QRadioButton(tr("Implicit multiplication first"));
	group->addButton(w_implicitfirst);
	if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) w_implicitfirst->setChecked(true);
	grid->addWidget(w_implicitfirst, 1, 0, Qt::AlignTop | Qt::AlignLeft);
	w_implicitfirst->setMinimumHeight(h);
	label->setText("<i>1/2x = 1/(2x)<br>5 m/2 s = (5 m)/(2 s)</i>");
	grid->addWidget(label, 1, 1);
	QRadioButton *w_conventional = new QRadioButton(tr("Conventional"));
	group->addButton(w_conventional);
	if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL) w_conventional->setChecked(true);
	grid->addWidget(w_conventional, 2, 0, Qt::AlignTop | Qt::AlignLeft);
	w_conventional->setMinimumHeight(h);
	grid->addWidget(new QLabel("<i>1/2x = (1/2)x<br>5 m/2 s = (5 m/2)s</i>"), 2, 1);
	QRadioButton *w_adaptive = new QRadioButton(tr("Adaptive"));
	group->addButton(w_adaptive);
	grid->addWidget(w_adaptive, 3, 0, Qt::AlignTop | Qt::AlignLeft);
	w_adaptive->setMinimumHeight(h);
	grid->addWidget(new QLabel("<i>1/2x = 1/(2x); 1/2 x = (1/2)x<br>5 m/2 s = (5 m)/(2 s)</i>"), 3, 1);
	if(settings->evalops.parse_options.parsing_mode == PARSING_MODE_ADAPTIVE) w_adaptive->setChecked(true);
	dialog->exec();
	settings->implicit_question_asked = true;
	ParsingMode pm_bak = settings->evalops.parse_options.parsing_mode;
	if(w_implicitfirst->isChecked()) {
		settings->evalops.parse_options.parsing_mode = PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST;
	} else if(w_conventional->isChecked()) {
		settings->evalops.parse_options.parsing_mode = PARSING_MODE_CONVENTIONAL;
	} else {
		settings->evalops.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
	}
	if(preferencesDialog) preferencesDialog->updateParsingMode();
	dialog->deleteLater();
	return pm_bak != settings->evalops.parse_options.parsing_mode;
}
bool QalculateWindow::askPercent() {
	if(settings->simplified_percentage >= 0 || !CALCULATOR->simplifiedPercentageUsed()) return false;
	QDialog *dialog = new QDialog(this);
	QVBoxLayout *box = new QVBoxLayout(dialog);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Percentage Interpretation"));
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	box->addWidget(buttonBox);
	grid->addWidget(new QLabel(tr("Please select interpretation of percentage addition")), 0, 0, 1, 2);
	QButtonGroup *group = new QButtonGroup(dialog);
	group->setExclusive(true);
	QRadioButton *w_1 = new QRadioButton(tr("Add percentage of original value"));
	group->addButton(w_1);
	grid->addWidget(w_1, 1, 0, Qt::AlignTop);
	std::string s_eg = "<i>100 + 10% = 100 "; s_eg += settings->multiplicationSign(); s_eg += " 110% = 110</i>)";
	grid->addWidget(new QLabel(QString::fromStdString(s_eg)), 1, 1);
	QRadioButton *w_0 = new QRadioButton(tr("Add percentage multiplied by 1/100"));
	group->addButton(w_0);
	grid->addWidget(w_0, 2, 0, Qt::AlignTop);
	s_eg = "<i>100 + 10% = 100 + (10 "; s_eg += settings->multiplicationSign(); s_eg += " 0.01) = 100.1</i>)";
	grid->addWidget(new QLabel(QString::fromStdString(CALCULATOR->localizeExpression(s_eg, settings->evalops.parse_options))), 2, 1);
	w_1->setChecked(true);
	dialog->exec();
	if(w_0->isChecked()) settings->simplified_percentage = 0;
	else settings->simplified_percentage = 1;
	dialog->deleteLater();
	return settings->simplified_percentage == 0;
}

void QalculateWindow::keyPressEvent(QKeyEvent *e) {
	if(e->matches(QKeySequence::Undo)) {
		expressionEdit->editUndo();
		return;
	}
	if(e->matches(QKeySequence::Redo)) {
		expressionEdit->editRedo();
		return;
	}
	QMainWindow::keyPressEvent(e);
}
void QalculateWindow::closeEvent(QCloseEvent *e) {
	if(!settings->current_workspace.empty()) {
		int i = askSaveWorkspace();
		if(i < 0) return;
	}
	settings->window_state = saveState();
	if(height() != DEFAULT_HEIGHT || width() != DEFAULT_WIDTH) settings->window_geometry = saveGeometry();
	else settings->window_geometry = QByteArray();
	settings->splitter_state = ehSplitter->saveState();
	settings->show_bases = basesDock->isVisible();
	if(settings->savePreferences(settings->save_mode_on_exit) < 0) return;
	if(settings->save_defs_on_exit) {
		while(!CALCULATOR->saveDefinitions()) {
			int answer = QMessageBox::critical(this, tr("Error"), tr("Couldn't write definitions"), QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Cancel);
			if(answer == QMessageBox::Ignore) break;
			else if(answer == QMessageBox::Cancel) return;
		}
	}
	CALCULATOR->abort();
	CALCULATOR->terminateThreads();
	if(commandThread->running) {
		commandThread->write((int) 0);
		commandThread->write(NULL);
	}
	if(viewThread->running) {
		viewThread->write((int) 0);
		viewThread->write(NULL);
	}
	QMainWindow::closeEvent(e);
	qApp->closeAllWindows();
}

void QalculateWindow::onToActivated(bool button) {
	QTextCursor cur = expressionEdit->textCursor();
	bool b_result = !expressionEdit->expressionHasChanged() && settings->current_result;
	bool b_addto = !b_result;
	if(b_addto && button && CALCULATOR->hasToExpression(expressionEdit->toPlainText().toStdString(), true, settings->evalops)) {
		std::string str = expressionEdit->toPlainText().toStdString(), to_str;
		CALCULATOR->separateToExpression(str, to_str, settings->evalops, true, true);
		if(to_str.empty()) b_addto = false;
	}
	if(!b_result) expressionEdit->blockCompletion(true, button);
	if(b_addto) {
		expressionEdit->blockParseStatus();
		expressionEdit->moveCursor(QTextCursor::End);
		expressionEdit->insertPlainText("➞");
		expressionEdit->blockParseStatus(false);
		expressionEdit->displayParseStatus(true, false);
	}
	if(b_result || button) {
		expressionEdit->complete(b_result ? mstruct : NULL, b_result ? parsed_mstruct : NULL, toMenu);
		if(!toMenu->isEmpty()) {
			toAction_t->setMenu(toMenu);
			toAction_t->showMenu();
			toAction_t->setMenu(NULL);
			toMenu->clear();
		}
	} else {
		expressionEdit->complete();
	}
	if(!b_result) expressionEdit->blockCompletion(false);
}
void QalculateWindow::onToConversionRequested(std::string str) {
	str.insert(0, "➞");
	if(str[str.length() - 1] == ' ' || str[str.length() - 1] == '/') expressionEdit->insertPlainText(QString::fromStdString(str));
	else calculateExpression(true, false, OPERATION_ADD, NULL, false, 0, "", str);
}
void QalculateWindow::importCSV() {
	size_t n = CALCULATOR->variables.size();
	if(CSVDialog::importCSVFile(this)) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		for(size_t i = n; i < CALCULATOR->variables.size(); i++) {
			if(!CALCULATOR->variables[i]->isHidden()) settings->favourite_variables.push_back(CALCULATOR->variables[i]);
		}
		variablesMenu->clear();
	}
}
void QalculateWindow::exportCSV() {
	CSVDialog::exportCSVFile(this, !auto_result.empty() ? &mauto : mstruct);
}
void QalculateWindow::onStoreActivated() {
	ExpressionItem *replaced_item = NULL;
	KnownVariable *v = VariableEditDialog::newVariable(this, !auto_result.empty() ? &mauto : (expressionEdit->expressionHasChanged() || settings->history_answer.empty() ? NULL : (mstruct_exact.isUndefined() ? mstruct : &mstruct_exact)), !auto_result.empty() ? QString::fromStdString(unhtmlize(auto_result)) : (expressionEdit->expressionHasChanged() ? expressionEdit->toPlainText() : QString::fromStdString(exact_text)), &replaced_item);
	if(v) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		if(v != replaced_item && !v->isHidden()) settings->favourite_variables.push_back(v);
		variablesMenu->clear();
	}
}
void QalculateWindow::newVariable() {
	ExpressionItem *replaced_item = NULL;
	KnownVariable *v = VariableEditDialog::newVariable(this, NULL, NULL, &replaced_item);
	if(v) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		if(v != replaced_item && !v->isHidden()) settings->favourite_variables.push_back(v);
		variablesMenu->clear();
	}
}
void QalculateWindow::newMatrix() {
	ExpressionItem *replaced_item = NULL;
	KnownVariable *v = VariableEditDialog::newMatrix(this, &replaced_item);
	if(v) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		if(v != replaced_item && !v->isHidden()) settings->favourite_variables.push_back(v);
		variablesMenu->clear();
	}
}
void QalculateWindow::newUnknown() {
	ExpressionItem *replaced_item = NULL;
	UnknownVariable *v = UnknownEditDialog::newVariable(this, &replaced_item);
	if(v) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		if(v != replaced_item && !v->isHidden()) settings->favourite_variables.push_back(v);
		variablesMenu->clear();
	}
}
void QalculateWindow::newUnit() {
	ExpressionItem *replaced_item = NULL;
	Unit *u = UnitEditDialog::newUnit(this, &replaced_item);
	if(u) {
		expressionEdit->updateCompletion();
		if(variablesDialog) variablesDialog->updateVariables();
		if(unitsDialog) unitsDialog->updateUnits();
		if(u != replaced_item && !u->isHidden()) settings->favourite_units.push_back(u);
		unitsMenu->clear();
	}
}

void QalculateWindow::newFunction() {
	MathFunction *replaced_item = NULL;
	MathFunction *f = FunctionEditDialog::newFunction(this, &replaced_item);
	if(f) {
		expressionEdit->updateCompletion();
		if(functionsDialog) functionsDialog->updateFunctions();
		if(f != replaced_item && !f->isHidden()) settings->favourite_functions.push_back(f);
		functionsMenu->clear();
	}
}

void QalculateWindow::showNumpad(bool b) {
	keypad->hideNumpad(!b);
	settings->hide_numpad = !b;
	workspace_changed = true;
	nKeypadAction->setEnabled(!b);
	if(b && settings->keypad_type == KEYPAD_NUMBERPAD) gKeypadAction->trigger();
}
void QalculateWindow::showSeparateKeypadMenuButtons(bool b) {
	settings->separate_keypad_menu_buttons = b;
	keypad->showSeparateKeypadMenuButtons(b);
}
void QalculateWindow::resetKeypadPosition() {
	keypadDock->setFloating(false);
	if(dockWidgetArea(keypadDock) != Qt::BottomDockWidgetArea) {
		bool b = keypadDock->isVisible();
		removeDockWidget(keypadDock);
		addDockWidget(Qt::BottomDockWidgetArea, keypadDock);
		keypadDock->setVisible(b);
	}
}
void QalculateWindow::initFinished() {
	init_in_progress = false;
}
#define DOCK_IN_WINDOW(dock) (!dock->isFloating() && (dockWidgetArea(dock) == Qt::BottomDockWidgetArea || dockWidgetArea(dock) == Qt::TopDockWidgetArea))
#define DOCK_VISIBLE_IN_WINDOW(dock) (dock->isVisible() && DOCK_IN_WINDOW(dock))
void QalculateWindow::beforeShowDockCleanUp(QDockWidget*) {
	if(emhTimer && emhTimer->isActive()) {
		emhTimer->stop();
		onEMHTimeout();
	}
	if(resizeTimer && resizeTimer->isActive()) {
		resizeTimer->stop();
		onResizeTimeout();
	}
}
void QalculateWindow::beforeShowDock(QDockWidget *w, bool b) {
	beforeShowDockCleanUp(w);
	if(!init_in_progress && b && DOCK_IN_WINDOW(w)) {
		expressionEdit->setMinimumHeight(expressionEdit->height());
		if(!w->isVisible() && (settings->preserve_history_height > 0 || (settings->preserve_history_height < 0 && historyView->height() < w->sizeHint().height()))) {
			if(settings->preserve_history_height < 0) {
				if(w == keypadDock) settings->keypad_appended = true;
				else if(w == basesDock) settings->bases_appended = true;
			}
			historyView->setMinimumHeight(historyView->height());
		} else {
			if(w == keypadDock) settings->keypad_appended = false;
			else if(w == basesDock) settings->bases_appended = false;
		}
	}
}
void QalculateWindow::afterShowDock(QDockWidget *w) {
	if(!init_in_progress && w->isVisible()) {
		if(!emhTimer) {
			emhTimer = new QTimer();
			emhTimer->setSingleShot(true);
			connect(emhTimer, SIGNAL(timeout()), this, SLOT(onEMHTimeout()));
		}
		emhTimer->start(100);
	}
}
void QalculateWindow::onEMHTimeout() {
	expressionEdit->updateMinimumHeight();
	historyView->updateMinimumHeight();
}
void QalculateWindow::onResizeTimeout() {
	if(DOCK_VISIBLE_IN_WINDOW(basesDock)) basesDock->setMaximumHeight(QWIDGETSIZE_MAX);
	if(DOCK_VISIBLE_IN_WINDOW(keypadDock)) keypadDock->setMaximumHeight(QWIDGETSIZE_MAX);
	setMinimumWidth(0);
	setMaximumWidth(QWIDGETSIZE_MAX);
	centralWidget()->setMaximumHeight(QWIDGETSIZE_MAX);
	centralWidget()->setMinimumHeight(0);
}
// minimum width when dock is shown
void QalculateWindow::keypadTypeActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	beforeShowDock(keypadDock, v >= 0 || !keypadDock->isVisible());
	if(v < 0) {
		bool b = !keypadDock->isVisible();
		bool b_resize = (settings->preserve_history_height > 0 || (settings->preserve_history_height < 0 && settings->keypad_appended)) && !b && !init_in_progress && DOCK_IN_WINDOW(keypadDock);
		if(b_resize) {
			centralWidget()->setFixedHeight(centralWidget()->height());
			if(DOCK_VISIBLE_IN_WINDOW(basesDock)) basesDock->setMaximumHeight(basesDock->height());
		}
		keypadDock->setVisible(b);
		if(b) keypadDock->raise();
		if(b_resize) {
			setFixedWidth(width());
			adjustSize();
			if(!resizeTimer) {
				resizeTimer = new QTimer();
				resizeTimer->setSingleShot(true);
				connect(resizeTimer, SIGNAL(timeout()), this, SLOT(onResizeTimeout()));
			}
			resizeTimer->start(100);
		}
		settings->show_keypad = b ? 1 : 0;
	} else {
		if(settings->keypad_type == v && keypadDock->isVisible()) v = KEYPAD_GENERAL;
		settings->keypad_type = v;
		keypad->setKeypadType(v);
		keypadDock->setVisible(true);
		keypadDock->raise();
		settings->show_keypad = 1;
		updateKeypadTitle();
	}
	afterShowDock(keypadDock);
	resetKeypadPositionAction->setEnabled(keypadDock->isVisible() && (keypadDock->isFloating() || dockWidgetArea(keypadDock) != Qt::BottomDockWidgetArea));
	workspace_changed = true;
}
void QalculateWindow::onKeypadVisibilityChanged(bool b) {
	beforeShowDockCleanUp(keypadDock);
	QAction *action = find_child_data(this, "group_keypad", b ? settings->keypad_type : -1);
	if(action) action->setChecked(true);
	settings->show_keypad = b ? 1 : 0;
	resetKeypadPositionAction->setEnabled(keypadDock->isVisible() && (keypadDock->isFloating() || dockWidgetArea(keypadDock) != Qt::BottomDockWidgetArea));
	if(!b && settings->keypad_type == KEYPAD_PROGRAMMING && settings->programming_base_changed) {
		settings->programming_base_changed = false;
		onBaseClicked(BASE_DECIMAL, true, false);
	}
	if((settings->preserve_history_height > 0 || (settings->preserve_history_height < 0 && settings->keypad_appended)) && !b && !init_in_progress && DOCK_IN_WINDOW(keypadDock)) {
		centralWidget()->setFixedHeight(centralWidget()->height());
		if(DOCK_VISIBLE_IN_WINDOW(basesDock)) basesDock->setMaximumHeight(basesDock->height());
		setFixedWidth(width());
		adjustSize();
		if(!resizeTimer) {
			resizeTimer = new QTimer();
			resizeTimer->setSingleShot(true);
			connect(resizeTimer, SIGNAL(timeout()), this, SLOT(onResizeTimeout()));
		}
		resizeTimer->start(100);
	}
}
void QalculateWindow::onToolbarVisibilityChanged(bool b) {
	expressionEdit->setMenuAndToolbarItems(b ? NULL : modeAction_t->menu(), b ? NULL : menuAction_t->menu(), b ? NULL : tbAction);
	historyView->setMenuAndToolbarItems(b ? NULL : modeAction_t->menu(), b ? NULL : menuAction_t->menu(), b ? NULL : tbAction);
	tbAction->setChecked(b);
}
void QalculateWindow::onBasesActivated(bool b) {
	if(b == basesDock->isVisible()) return;
	beforeShowDock(basesDock, b);
	bool b_resize = (settings->preserve_history_height > 0 || (settings->preserve_history_height < 0 && settings->bases_appended)) && !b && !init_in_progress && DOCK_IN_WINDOW(basesDock);
	if(b_resize) {
		centralWidget()->setFixedHeight(centralWidget()->height());
		if(DOCK_VISIBLE_IN_WINDOW(keypadDock)) keypadDock->setMaximumHeight(keypadDock->height());
	}
	basesDock->setVisible(b);
	if(b) basesDock->raise();
	if(b_resize) {
		setFixedWidth(width());
		adjustSize();
		if(!resizeTimer) {
			resizeTimer = new QTimer();
			resizeTimer->setSingleShot(true);
			connect(resizeTimer, SIGNAL(timeout()), this, SLOT(onResizeTimeout()));
		}
		resizeTimer->start(100);
	}
	afterShowDock(basesDock);
}
void QalculateWindow::onBasesVisibilityChanged(bool b) {
	beforeShowDockCleanUp(basesDock);
	basesAction->setChecked(b);
	if((settings->preserve_history_height > 0 || (settings->preserve_history_height < 0 && settings->bases_appended)) && !b && !init_in_progress && DOCK_IN_WINDOW(basesDock)) {
		centralWidget()->setFixedHeight(centralWidget()->height());
		if(DOCK_VISIBLE_IN_WINDOW(keypadDock)) keypadDock->setMaximumHeight(keypadDock->height());
		setFixedWidth(width());
		adjustSize();
		if(!resizeTimer) {
			resizeTimer = new QTimer();
			resizeTimer->setSingleShot(true);
			connect(resizeTimer, SIGNAL(timeout()), this, SLOT(onResizeTimeout()));
		}
		resizeTimer->start(100);
	}
	if(b && expressionEdit->expressionHasChanged()) onExpressionChanged();
	else if(b && !settings->history_answer.empty()) updateResultBases();
}
bool QalculateWindow::displayMessages() {
	return settings->displayMessages(this);
}
bool QalculateWindow::updateWindowTitle(const QString &str, bool is_result, bool type_change) {
	if(title_modified) return false;
	if(type_change) {
		if(settings->title_type == TITLE_APP_RESULT || settings->title_type == TITLE_APP_WORKSPACE || settings->title_type == TITLE_APP) {
			qApp->setApplicationDisplayName("Qalculate!");
			setWindowTitle(QString());
		} else if(settings->title_type == TITLE_RESULT) {
			qApp->setApplicationDisplayName(QString());
			setWindowTitle(QString());
		}
		if(settings->title_type == TITLE_RESULT || settings->title_type == TITLE_APP_RESULT) {
			if(result_text.empty()) {
				if(!auto_result.empty()) setWindowTitle(QString::fromStdString(unhtmlize(auto_result)));
				else if(settings->title_type == TITLE_RESULT) setWindowTitle("Qalculate!");
			} else {
				setWindowTitle(QString::fromStdString(unhtmlize(result_text)));
			}
		}
	}
	switch(settings->title_type) {
		case TITLE_RESULT: {
			if(str.isEmpty()) {
				if(is_result) {
					setWindowTitle("Qalculate!");
					return true;
				}
				return false;
			}
			setWindowTitle(str);
			break;
		}
		case TITLE_APP_RESULT: {
			if(str.isEmpty() && !is_result) {
				return false;
			}
			setWindowTitle(str);
			break;
		}
		case TITLE_WORKSPACE: {
			if(is_result) return false;
			if(settings->workspaceTitle().isEmpty()) qApp->setApplicationDisplayName("Qalculate!");
			else qApp->setApplicationDisplayName(settings->workspaceTitle());
			setWindowTitle(str);
			break;
		}
		case TITLE_APP_WORKSPACE: {
			if(is_result) return false;
			if(str.isEmpty()) setWindowTitle(settings->workspaceTitle());
			else setWindowTitle(str);
			break;
		}
		default: {
			if(is_result) return false;
			setWindowTitle(str);
		}
	}
	return true;
}

void QalculateWindow::angleUnitActivated() {
	QAction *action = qobject_cast<QAction*>(sender());
	settings->evalops.parse_options.angle_unit = (AngleUnit) action->data().toInt();
	if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM) {
		CALCULATOR->setCustomAngleUnit(CALCULATOR->getActiveUnit(action->objectName().mid(18).toStdString()));
	}
	expressionFormatUpdated(true);
}
void QalculateWindow::normalActivated() {
	settings->printops.sort_options.minus_last = true;
	settings->printops.min_exp = EXP_PRECISION;
	settings->printops.show_ending_zeroes = true;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = true;
	settings->printops.negative_exponents = false;
	resultFormatUpdated();
}
void QalculateWindow::scientificActivated() {
	settings->printops.sort_options.minus_last = false;
	settings->printops.min_exp = EXP_SCIENTIFIC;
	settings->printops.show_ending_zeroes = true;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = false;
	settings->printops.negative_exponents = true;
	resultFormatUpdated();
}
void QalculateWindow::engineeringActivated() {
	settings->printops.sort_options.minus_last = false;
	settings->printops.min_exp = EXP_BASE_3;
	settings->printops.show_ending_zeroes = true;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = false;
	settings->printops.negative_exponents = false;
	resultFormatUpdated();
}
void QalculateWindow::simpleActivated() {
	settings->printops.sort_options.minus_last = true;
	settings->printops.min_exp = EXP_NONE;
	settings->printops.show_ending_zeroes = false;
	if(settings->prefixes_default) settings->printops.use_unit_prefixes = true;
	settings->printops.negative_exponents = false;
	resultFormatUpdated();
}
void QalculateWindow::onPrecisionChanged(int v) {
	CALCULATOR->setPrecision(v);
	settings->previous_precision = 0;
	expressionCalculationUpdated(500);
}
void QalculateWindow::syncDecimals() {
	if(settings->printops.use_min_decimals && settings->printops.use_max_decimals && settings->printops.max_decimals >= 0 && settings->printops.min_decimals > 0 && settings->printops.min_decimals > settings->printops.max_decimals) {
		settings->printops.min_decimals = settings->printops.max_decimals;
		minDecimalsEdit->blockSignals(true);
		minDecimalsEdit->setValue(settings->printops.min_decimals);
		minDecimalsEdit->blockSignals(false);
	}
}
void QalculateWindow::onMinDecimalsChanged(int v) {
	settings->printops.use_min_decimals = (v > 0);
	settings->printops.min_decimals = v;
	if(decimalsTimer) decimalsTimer->stop();
	if(settings->printops.use_min_decimals && settings->printops.use_max_decimals && settings->printops.max_decimals >= 0 && settings->printops.min_decimals > 0 && settings->printops.min_decimals > settings->printops.max_decimals) {
		settings->printops.max_decimals = settings->printops.min_decimals;
		maxDecimalsEdit->blockSignals(true);
		maxDecimalsEdit->setValue(settings->printops.max_decimals);
		maxDecimalsEdit->blockSignals(false);
	}
	resultFormatUpdated(500);
}
void QalculateWindow::onMaxDecimalsChanged(int v) {
	settings->printops.use_max_decimals = (v >= 0);
	settings->printops.max_decimals = v;
	if(decimalsTimer) {
		decimalsTimer->stop();
	} else {
		decimalsTimer = new QTimer();
		decimalsTimer->setSingleShot(true);
		connect(decimalsTimer, SIGNAL(timeout()), this, SLOT(syncDecimals()));
	}
	decimalsTimer->start(2000);
	resultFormatUpdated(500);
}
void QalculateWindow::approximationActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	if(v < 0) {
		settings->evalops.approximation = APPROXIMATION_TRY_EXACT;
		if(v == -2) {settings->dual_approximation = 1; settings->dual_fraction = 1;}
		else {settings->dual_approximation = -1; settings->dual_fraction = -1;}
	} else {
		settings->evalops.approximation = (ApproximationMode) v;
		settings->dual_fraction = 0;
		settings->dual_approximation = 0;
	}
	if(settings->evalops.approximation == APPROXIMATION_EXACT) settings->printops.number_fraction_format = FRACTION_DECIMAL_EXACT;
	else settings->printops.number_fraction_format = FRACTION_DECIMAL;
	expressionCalculationUpdated();
}
void QalculateWindow::outputBaseActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	to_base = 0;
	to_bits = 0;
	if(v == BASE_CUSTOM) {
		v = customOutputBaseEdit->value();
		if(v > 2 && v <= 36) {
			settings->printops.base = v;
		} else {
			settings->printops.base = BASE_CUSTOM;
			CALCULATOR->setCustomOutputBase(v);
		}
	} else {
		settings->printops.base = v;
	}
	keypad->updateBase();
	resultFormatUpdated();
}
void QalculateWindow::onCustomOutputBaseChanged(int v) {
	customOutputBaseAction->setChecked(true);
	to_base = 0;
	to_bits = 0;
	if(v > 2 && v <= 36) {
		settings->printops.base = v;
	} else {
		settings->printops.base = BASE_CUSTOM;
		CALCULATOR->setCustomOutputBase(Number(v, 1));
	}
	keypad->updateBase();
	resultFormatUpdated();
}
void QalculateWindow::inputBaseActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	if(v == BASE_CUSTOM) {
		v = customOutputBaseEdit->value();
		if(v > 2 && v <= 36) {
			settings->evalops.parse_options.base = v;
		} else {
			settings->evalops.parse_options.base = BASE_CUSTOM;
			CALCULATOR->setCustomInputBase(v);
		}
	} else {
		settings->evalops.parse_options.base = v;
	}
	keypad->updateBase();
	expressionFormatUpdated(false);
}
void QalculateWindow::onCustomInputBaseChanged(int v) {
	customInputBaseAction->setChecked(true);
	if(v > 2 && v <= 36) {
		settings->evalops.parse_options.base = v;
	} else {
		settings->evalops.parse_options.base = BASE_CUSTOM;
		CALCULATOR->setCustomInputBase(Number(v, 1));
	}
	keypad->updateBase();
	expressionFormatUpdated(false);
}
void QalculateWindow::assumptionsTypeActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
	for(int i = 0; i < 5; i++) {
		if(assumptionSignActions[i]->data().toInt() == CALCULATOR->defaultAssumptions()->sign()) {
			assumptionSignActions[i]->setChecked(true);
			break;
		}
	}
	expressionCalculationUpdated();
}
void QalculateWindow::assumptionsSignActivated() {
	int v = qobject_cast<QAction*>(sender())->data().toInt();
	CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
	for(int i = 0; i < 4; i++) {
		if(assumptionTypeActions[i]->data().toInt() == CALCULATOR->defaultAssumptions()->type()) {
			assumptionTypeActions[i]->setChecked(true);
			break;
		}
	}
	expressionCalculationUpdated();
}
void QalculateWindow::onAlwaysOnTopChanged() {
	if(settings->always_on_top) {
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		if(datasetsDialog) datasetsDialog->setWindowFlags(datasetsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(functionsDialog) functionsDialog->setWindowFlags(functionsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(variablesDialog) variablesDialog->setWindowFlags(variablesDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(unitsDialog) unitsDialog->setWindowFlags(unitsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(fpConversionDialog) fpConversionDialog->setWindowFlags(fpConversionDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(percentageDialog) percentageDialog->setWindowFlags(percentageDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(plotDialog) plotDialog->setWindowFlags(plotDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(calendarConversionDialog) calendarConversionDialog->setWindowFlags(calendarConversionDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(preferencesDialog) preferencesDialog->setWindowFlags(preferencesDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		if(historyView->searchDialog) historyView->searchDialog->setWindowFlags(historyView->searchDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	} else {
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(datasetsDialog) datasetsDialog->setWindowFlags(datasetsDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(functionsDialog) functionsDialog->setWindowFlags(functionsDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(variablesDialog) variablesDialog->setWindowFlags(variablesDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(unitsDialog) unitsDialog->setWindowFlags(unitsDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(fpConversionDialog) fpConversionDialog->setWindowFlags(fpConversionDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(percentageDialog) percentageDialog->setWindowFlags(percentageDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(plotDialog) plotDialog->setWindowFlags(plotDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(calendarConversionDialog) calendarConversionDialog->setWindowFlags(calendarConversionDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(preferencesDialog) preferencesDialog->setWindowFlags(preferencesDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
		if(historyView->searchDialog) historyView->searchDialog->setWindowFlags(historyView->searchDialog->windowFlags() & ~Qt::WindowStaysOnTopHint);
	}
	if(periodicTableDialog) periodicTableDialog->onAlwaysOnTopChanged();
	show();
}
void QalculateWindow::onEnableTooltipsChanged() {
	if(settings->enable_tooltips == 0) qApp->installEventFilter(this);
	else qApp->removeEventFilter(this);
	QList<QAbstractButton*> buttons = keypad->findChildren<QAbstractButton*>();
	for(int i = 0; i < buttons.count(); i++) {
		if(settings->enable_tooltips > 1) buttons.at(i)->installEventFilter(this);
		else buttons.at(i)->removeEventFilter(this);
	}
}
void QalculateWindow::onTitleTypeChanged() {
	title_modified = false;
	updateWindowTitle(QString(), false, true);
}
void QalculateWindow::onPreferencesClosed() {
	preferencesDialog->deleteLater();
	preferencesDialog = NULL;
}
void QalculateWindow::onResultFontChanged() {
	if(settings->use_custom_result_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_result_font)); historyView->setFont(font); rpnView->setFont(font);}
	else {historyView->setFont(QApplication::font());  rpnView->setFont(QApplication::font());}
}
void QalculateWindow::onExpressionFontChanged() {
	if(settings->use_custom_expression_font) {
		QFont font; font.fromString(QString::fromStdString(settings->custom_expression_font)); expressionEdit->setFont(font);
	} else {
		QFont font = QApplication::font();
		if(font.pixelSize() >= 0) font.setPixelSize(font.pixelSize() * 1.35);
		else font.setPointSize(font.pointSize() * 1.35);
		expressionEdit->setFont(font);
	}
}
void QalculateWindow::onKeypadFontChanged() {
	if(settings->use_custom_keypad_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_keypad_font)); keypad->setFont(font);}
	else keypad->setFont(QApplication::font());
}
void QalculateWindow::onAppFontTimer() {
	onAppFontChanged();
	loadInitialHistory();
}
void QalculateWindow::onAppFontChanged() {
	if(settings->use_custom_app_font) {QFont font; font.fromString(QString::fromStdString(settings->custom_app_font)); QApplication::setFont(font);}
	else QApplication::setFont(settings->saved_app_font);
	if(!settings->use_custom_expression_font) {
		QFont font = QApplication::font();
		if(font.pixelSize() >= 0) font.setPixelSize(font.pixelSize() * 1.35);
		else font.setPointSize(font.pointSize() * 1.35);
		expressionEdit->setFont(font);
		if(expressionEdit->completionInitialized()) expressionEdit->updateCompletion();
	}
	if(!settings->use_custom_result_font) {
		historyView->setFont(QApplication::font());
		rpnView->setFont(QApplication::font());
	}
	if(!settings->use_custom_keypad_font) keypad->setFont(QApplication::font());
}
void QalculateWindow::onExpressionStatusModeChanged(bool b) {
	if(preferencesDialog) preferencesDialog->updateExpressionStatus();
	if(b) {
		if(!settings->status_in_history) historyView->clearTemporary();
		auto_expression = "";
		auto_error = "";
		mauto.setAborted();
		if(!settings->status_in_history) updateWindowTitle(QString::fromStdString(unhtmlize(result_text)), true);
		if(autoCalculateTimer) autoCalculateTimer->stop();
		expressionEdit->displayParseStatus(true);
	}
}
void QalculateWindow::removeShortcutClicked() {
	QTreeWidgetItem *item = shortcutList->currentItem();
	if(!item) return;
	keyboard_shortcut *ks = (keyboard_shortcut*) item->data(0, Qt::UserRole).value<void*>();
	if(!ks) return;
	keyboardShortcutRemoved(ks);
	for(size_t i = 0; i < settings->keyboard_shortcuts.size(); i++) {
		if(settings->keyboard_shortcuts[i] == ks) {
			settings->keyboard_shortcuts.erase(settings->keyboard_shortcuts.begin() + i);
			break;
		}
	}
	settings->default_shortcuts = false;
	delete ks;
	delete item;
}
void QalculateWindow::updateShortcutActionOK() {
	QTreeWidgetItem *item = shortcutActionList->currentItem();
	if(!item || !item->isSelected()) {
		shortcutActionOKButton->setEnabled(edited_keyboard_shortcut->type.size() > 0);
		shortcutActionAddButton->setEnabled(false);
	} else if(!SHORTCUT_REQUIRES_VALUE(item->data(0, Qt::UserRole).toInt()) || !shortcutActionValueEdit->currentText().isEmpty()) {
		shortcutActionOKButton->setEnabled(true);
		shortcutActionAddButton->setEnabled(true);
	} else {
		shortcutActionOKButton->setEnabled(false);
		shortcutActionAddButton->setEnabled(false);
	}
}
void QalculateWindow::shortcutActionOKClicked() {
	QString value = shortcutActionValueEdit->currentText();
	QTreeWidgetItem *item = shortcutActionList->currentItem();
	if(!item || !item->isSelected()) {
		if(edited_keyboard_shortcut->type.size() > 0) shortcutActionDialog->accept();
		return;
	}
	if(settings->testShortcutValue(item->data(0, Qt::UserRole).toInt(), value, shortcutActionDialog)) {
		edited_keyboard_shortcut->type.push_back((shortcut_type) shortcutActionList->currentItem()->data(0, Qt::UserRole).toInt());
		edited_keyboard_shortcut->value.push_back(value.toStdString());
		shortcutActionDialog->accept();
	} else {
		shortcutActionValueEdit->setFocus();
	}
	if(item->data(0, Qt::UserRole).toInt() == SHORTCUT_TYPE_COPY_RESULT) {
		int v = value.toInt();
		if(v >= 0 && v < settings->copy_action_value_texts.size()) shortcutActionValueEdit->setCurrentText(settings->copy_action_value_texts[v]);
	} else {
		shortcutActionValueEdit->setCurrentText(value);
	}
}
void QalculateWindow::shortcutActionAddClicked() {
	QString value = shortcutActionValueEdit->currentText();
	QTreeWidgetItem *item = shortcutActionList->currentItem();
	if(!item || !item->isSelected()) return;
	if(settings->testShortcutValue(item->data(0, Qt::UserRole).toInt(), value, shortcutActionDialog)) {
		edited_keyboard_shortcut->type.push_back((shortcut_type) shortcutActionList->currentItem()->data(0, Qt::UserRole).toInt());
		edited_keyboard_shortcut->value.push_back(value.toStdString());
		shortcutActionValueEdit->clear();
		shortcutActionList->setCurrentItem(NULL);
		shortcutActionAddButton->setText(tr("Add Action (%1)").arg(edited_keyboard_shortcut->type.size() + 1));
	} else {
		shortcutActionValueEdit->setFocus();
		if(item->data(0, Qt::UserRole).toInt() == SHORTCUT_TYPE_COPY_RESULT) {
			int v = value.toInt();
			if(v >= 0 && v < settings->copy_action_value_texts.size()) shortcutActionValueEdit->setCurrentText(settings->copy_action_value_texts[v]);
		} else {
			shortcutActionValueEdit->setCurrentText(value);
		}
	}
}
void QalculateWindow::currentShortcutActionChanged() {
	QTreeWidgetItem *item = shortcutActionList->currentItem();
	if(!item || !item->isSelected() || !SHORTCUT_USES_VALUE(item->data(0, Qt::UserRole).toInt())) {
		shortcutActionValueEdit->clear();
		shortcutActionValueEdit->clearEditText();
		shortcutActionValueEdit->setEnabled(false);
		shortcutActionValueLabel->setEnabled(false);
		return;
	}
	int i = item->data(0, Qt::UserRole).toInt();
	shortcutActionValueEdit->setEnabled(true);
	shortcutActionValueLabel->setEnabled(true);
	if(i == SHORTCUT_TYPE_FUNCTION || i == SHORTCUT_TYPE_FUNCTION_WITH_DIALOG) {
		QStringList citems;
		for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
			MathFunction *f = CALCULATOR->functions[i];
			if(f->isActive() && !f->isHidden()) citems << QString::fromStdString(f->referenceName());
		}
		if(shortcutActionValueEdit->count() != citems.count()) {
			shortcutActionValueEdit->clear();
			citems.sort(Qt::CaseInsensitive);
			shortcutActionValueEdit->addItems(citems);
			shortcutActionValueEdit->clearEditText();
		}
	} else {
		shortcutActionValueEdit->clear();
		if(i == SHORTCUT_TYPE_UNIT || i == SHORTCUT_TYPE_CONVERT_TO) {
			QStringList citems;
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				Unit *u = CALCULATOR->units[i];
				if(u->isActive() && !u->isHidden()) citems << QString::fromStdString(u->referenceName());
			}
			citems.sort(Qt::CaseInsensitive);
			shortcutActionValueEdit->addItems(citems);
		} else if(i == SHORTCUT_TYPE_VARIABLE) {
			QStringList citems;
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				Variable *v = CALCULATOR->variables[i];
				if(v->isActive() && !v->isHidden()) citems << QString::fromStdString(v->referenceName());
			}
			citems.sort(Qt::CaseInsensitive);
			shortcutActionValueEdit->addItems(citems);
		} else if(i == SHORTCUT_TYPE_OPERATOR) {
			QStringList citems;
			citems << "+" << (settings->printops.use_unicode_signs ? SIGN_MINUS : "-") << settings->multiplicationSign(false) << settings->divisionSign(false) << "^" << ".+" << (QString(".") + (settings->printops.use_unicode_signs ? SIGN_MINUS : "-")) << (QString(".") + settings->multiplicationSign(false)) << (QString(".") + settings->divisionSign(false)) << ".^" << "mod" << "rem" << "//" << "&" << "|" << "<<" << ">>" << "&&" << "||" << "xor" << "=" << SIGN_NOT_EQUAL << "<" << SIGN_LESS_OR_EQUAL << SIGN_GREATER_OR_EQUAL << ">";
			shortcutActionValueEdit->addItems(citems);
		} else if(i == SHORTCUT_TYPE_COPY_RESULT) {
			settings->updateActionValueTexts();
			shortcutActionValueEdit->addItems(settings->copy_action_value_texts);
		}
		if(i == SHORTCUT_TYPE_COPY_RESULT) shortcutActionValueEdit->setCurrentText(settings->copy_action_value_texts[0]);
		else shortcutActionValueEdit->clearEditText();
	}
}
class QalculateKeySequenceEdit : public QKeySequenceEdit {
	public:
		QalculateKeySequenceEdit(QWidget *parent) : QKeySequenceEdit(parent) {
			installEventFilter(this);
		}
	protected:
		bool eventFilter(QObject*, QEvent *event) {
			if(event->type() == QEvent::KeyPress) {
				QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
				if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
					keyPressEvent(keyEvent);
					return true;
				}
				return false;
			} else {
				return false;
			}
		}
};

bool QalculateWindow::editKeyboardShortcut(keyboard_shortcut *new_ks, keyboard_shortcut *ks, int type) {
	if(!ks) type = 0;
	QDialog *dialog = NULL;
	shortcutActionList = NULL;
	shortcutActionValueEdit = NULL;
	shortcutActionDialog = NULL;
	new_ks->type.clear();
	new_ks->value.clear();
	edited_keyboard_shortcut = new_ks;
	if(type != 2) {
		dialog = new QDialog(this);
		shortcutActionDialog = dialog;
		if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
		dialog->setWindowTitle(ks ? tr("Edit Keyboard Shortcut") : tr("New Keyboard Shortcut"));
		QVBoxLayout *box = new QVBoxLayout(dialog);
		QGridLayout *grid = new QGridLayout();
		box->addLayout(grid);
		shortcutActionList = new QTreeWidget(dialog);
		shortcutActionList->setColumnCount(1);
		shortcutActionList->setHeaderLabel(tr("Action"));
		shortcutActionList->setSelectionMode(QAbstractItemView::SingleSelection);
		shortcutActionList->setRootIsDecorated(false);
		shortcutActionList->header()->setVisible(true);
		shortcutActionList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		shortcutActionList->setSortingEnabled(true);
		shortcutActionList->sortByColumn(-1, Qt::AscendingOrder);
		grid->addWidget(shortcutActionList, 0, 0, 1, 2);
		for(int i = SHORTCUT_TYPE_FUNCTION; i <= SHORTCUT_TYPE_QUIT; i++) {
			if(i < SHORTCUT_TYPE_EXPRESSION_CLEAR || i > SHORTCUT_TYPE_CALCULATE_EXPRESSION) {
				QTreeWidgetItem *item = new QTreeWidgetItem(shortcutActionList, QStringList(settings->shortcutTypeText((shortcut_type) i)));
				item->setData(0, Qt::UserRole, i);
				if(new_ks->type.size() == 0 && ((!ks && i == 0) || (ks && i == ks->type[0]))) shortcutActionList->setCurrentItem(item);
			}
			if(i == SHORTCUT_TYPE_HISTORY_SEARCH) {
				QTreeWidgetItem *item = new QTreeWidgetItem(shortcutActionList, QStringList(settings->shortcutTypeText(SHORTCUT_TYPE_HISTORY_CLEAR)));
				item->setData(0, Qt::UserRole, SHORTCUT_TYPE_HISTORY_CLEAR);
				if(new_ks->type.size() == 0 && ks && ks->type[0] == SHORTCUT_TYPE_HISTORY_CLEAR) shortcutActionList->setCurrentItem(item);
			} else if(i == SHORTCUT_TYPE_SIMPLE_NOTATION) {
				for(int i2 = SHORTCUT_TYPE_PRECISION; i2 <= SHORTCUT_TYPE_MINMAX_DECIMALS; i2++) {
					QTreeWidgetItem *item = new QTreeWidgetItem(shortcutActionList, QStringList(settings->shortcutTypeText((shortcut_type) i2)));
					item->setData(0, Qt::UserRole, i2);
					if(new_ks->type.size() == 0 && ks && i2 == ks->type[0]) shortcutActionList->setCurrentItem(item);
				}
			} else if(i == SHORTCUT_TYPE_MENU) {
				for(int i2 = SHORTCUT_TYPE_FUNCTIONS_MENU; i2 <= SHORTCUT_TYPE_VARIABLES_MENU; i2++) {
					QTreeWidgetItem *item = new QTreeWidgetItem(shortcutActionList, QStringList(settings->shortcutTypeText((shortcut_type) i2)));
					item->setData(0, Qt::UserRole, i2);
					if(new_ks->type.size() == 0 && ks && i2 == ks->type[0]) shortcutActionList->setCurrentItem(item);
				}
			}
		}
		shortcutActionValueLabel = new QLabel(tr("Value:"), dialog);
		grid->addWidget(shortcutActionValueLabel, 1, 0);
		shortcutActionValueEdit = new QComboBox(dialog);
		shortcutActionValueEdit->setEditable(true);
		shortcutActionValueEdit->setLineEdit(new MathLineEdit());
		grid->addWidget(shortcutActionValueEdit, 1, 1);
		grid->setColumnStretch(1, 1);
		QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
		buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
		buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
		shortcutActionAddButton = buttonBox->addButton(tr("Add Action (%1)").arg(1), QDialogButtonBox::ApplyRole);
		connect(shortcutActionAddButton, SIGNAL(clicked()), this, SLOT(shortcutActionAddClicked()));
		box->addWidget(buttonBox);
		connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(shortcutActionOKClicked()));
		connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
		connect(shortcutActionList, SIGNAL(itemSelectionChanged()), this, SLOT(currentShortcutActionChanged()), Qt::QueuedConnection);
		connect(shortcutActionList, SIGNAL(itemSelectionChanged()), this, SLOT(updateShortcutActionOK()), Qt::QueuedConnection);
		connect(shortcutActionValueEdit, SIGNAL(currentTextChanged(const QString&)), this, SLOT(updateShortcutActionOK()));
		shortcutActionOKButton = buttonBox->button(QDialogButtonBox::Ok);
		currentShortcutActionChanged();
		if(ks) {
			if(ks->type[0] == SHORTCUT_TYPE_COPY_RESULT) {
				int v = s2i(ks->value[0]);
				if(v >= 0 && v < settings->copy_action_value_texts.size()) shortcutActionValueEdit->setCurrentText(settings->copy_action_value_texts[v]);
			} else {
				shortcutActionValueEdit->setCurrentText(QString::fromStdString(ks->value[0]));
			}
		}
		updateShortcutActionOK();
		shortcutActionList->setFocus();
		dialog->resize(dialog->sizeHint().width(), dialog->sizeHint().width() * 1.25);
		if(dialog->exec() != QDialog::Accepted) {
			dialog->deleteLater();
			return false;
		}
		dialog->deleteLater();
	}
	if(type == 1) {
		new_ks->key = ks->key;
		new_ks->action = NULL;
		new_ks->new_action = false;
		return true;
	}
	dialog = new QDialog(this);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Set key combination"));
	QVBoxLayout *box = new QVBoxLayout(dialog);
	QGridLayout *grid = new QGridLayout();
	grid->addWidget(new QLabel("<i>" + tr("Press the key combination you wish to use for the action.") + "</i>", dialog), 0, 0);
	QKeySequenceEdit *keyEdit = new QalculateKeySequenceEdit(dialog);
	grid->addWidget(keyEdit, 1, 0);
	box->addLayout(grid);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, dialog);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	connect(keyEdit, SIGNAL(editingFinished()), dialog, SLOT(accept()));
	keyEdit->setFocus();
	while(dialog->exec() == QDialog::Accepted && !keyEdit->keySequence().isEmpty()) {
		QString key = keyEdit->keySequence().toString();
		if(keyEdit->keySequence() == QKeySequence::Undo || keyEdit->keySequence() == QKeySequence::Redo || keyEdit->keySequence() == QKeySequence::Copy || keyEdit->keySequence() == QKeySequence::Paste || keyEdit->keySequence() == QKeySequence::Delete || keyEdit->keySequence() == QKeySequence::Cut || keyEdit->keySequence() == QKeySequence::SelectAll ||
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		keyEdit->keySequence() == QKeySequence::Backspace || (keyEdit->keySequence().count() == 1 && keyEdit->keySequence()[0].keyboardModifiers() == Qt::NoModifier && keyEdit->keySequence()[0].key() != Qt::Key_Tab && keyEdit->keySequence()[0].key() != Qt::Key_Backtab && (keyEdit->keySequence()[0].key() < Qt::Key_F1 || (keyEdit->keySequence()[0].key() >= Qt::Key_Space && keyEdit->keySequence()[0].key() <= Qt::Key_ydiaeresis) || (keyEdit->keySequence()[0].key() >= Qt::Key_Multi_key && keyEdit->keySequence()[0].key() < Qt::Key_Back)))
#else
		(keyEdit->keySequence().count() == 1 && keyEdit->keySequence()[0] != Qt::Key_Tab && keyEdit->keySequence()[0] != Qt::Key_Backtab && (keyEdit->keySequence()[0] < Qt::Key_F1 || (keyEdit->keySequence()[0] >= Qt::Key_Space && keyEdit->keySequence()[0] <= Qt::Key_ydiaeresis) || (keyEdit->keySequence()[0] >= Qt::Key_Multi_key && keyEdit->keySequence()[0] < Qt::Key_Back)))
#endif
		) {
			QMessageBox::critical(this, tr("Error"), tr("Reserved key combination"), QMessageBox::Ok);
			keyEdit->clear();
			keyEdit->setFocus();
			continue;
		}
		for(size_t i = 0; i < settings->keyboard_shortcuts.size(); i++) {
			if(settings->keyboard_shortcuts[i] != ks && settings->keyboard_shortcuts[i]->key == key) {
				if(QMessageBox::question(this, tr("Question"), tr("The key combination is already in use.\nDo you wish to replace the current action (%1)?").arg(settings->shortcutText(settings->keyboard_shortcuts[i]->type, settings->keyboard_shortcuts[i]->value)), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
					for(int index = 0; index < shortcutList->topLevelItemCount(); index++) {
						if(shortcutList->topLevelItem(index)->data(0, Qt::UserRole).value<void*>() == (void*) settings->keyboard_shortcuts[i]) {
							delete shortcutList->topLevelItem(index);
							break;
						}
					}
					keyboardShortcutRemoved(settings->keyboard_shortcuts[i]);
					delete settings->keyboard_shortcuts[i];
					settings->keyboard_shortcuts.erase(settings->keyboard_shortcuts.begin() + i);
					settings->default_shortcuts = false;
					break;
				} else {
					dialog->deleteLater();
					return false;
				}
			}
		}
		new_ks->key = key;
		if(type == 2) {
			new_ks->type = ks->type;
			new_ks->value = ks->value;
		}
		new_ks->action = NULL;
		new_ks->new_action = false;
		dialog->deleteLater();
		return true;
	}
	dialog->deleteLater();
	return false;
}
void QalculateWindow::addShortcutClicked() {
	keyboard_shortcut *ks = new keyboard_shortcut;
	if(editKeyboardShortcut(ks, NULL)) {
		settings->keyboard_shortcuts.push_back(ks);
		size_t i = settings->keyboard_shortcuts.size() - 1;
		keyboardShortcutAdded(settings->keyboard_shortcuts[i]);
		QTreeWidgetItem *item = new QTreeWidgetItem(shortcutList);
		item->setText(0, settings->shortcutText(settings->keyboard_shortcuts[i]->type, settings->keyboard_shortcuts[i]->value));
		item->setText(1, QKeySequence::fromString(settings->keyboard_shortcuts[i]->key).toString());
		item->setData(0, Qt::UserRole, QVariant::fromValue((void*) settings->keyboard_shortcuts[i]));
		settings->default_shortcuts = false;
	} else {
		delete ks;
	}
}
void QalculateWindow::editShortcutClicked() {
	shortcutDoubleClicked(shortcutList->currentItem(), -1);
}
void QalculateWindow::currentShortcutChanged(QTreeWidgetItem *item, QTreeWidgetItem*) {
	editShortcutButton->setEnabled(item != NULL);
	removeShortcutButton->setEnabled(item != NULL);
}
void QalculateWindow::shortcutDoubleClicked(QTreeWidgetItem *item, int c) {
	if(!item) return;
	keyboard_shortcut *ks_old = (keyboard_shortcut*) item->data(0, Qt::UserRole).value<void*>();
	keyboard_shortcut *ks = new keyboard_shortcut;
	if(editKeyboardShortcut(ks, ks_old, c + 1)) {
		keyboardShortcutRemoved(ks_old);
		for(size_t i = 0; i < settings->keyboard_shortcuts.size(); i++) {
			if(settings->keyboard_shortcuts[i] == ks_old) {
				settings->keyboard_shortcuts.erase(settings->keyboard_shortcuts.begin() + i);
				break;
			}
		}
		settings->keyboard_shortcuts.push_back(ks);
		size_t i = settings->keyboard_shortcuts.size() - 1;
		keyboardShortcutAdded(settings->keyboard_shortcuts[i]);
		item->setText(0, settings->shortcutText(settings->keyboard_shortcuts[i]->type, settings->keyboard_shortcuts[i]->value));
		item->setText(1, QKeySequence::fromString(settings->keyboard_shortcuts[i]->key).toString());
		item->setData(0, Qt::UserRole, QVariant::fromValue((void*) settings->keyboard_shortcuts[i]));
		settings->default_shortcuts = false;
	} else {
		delete ks;
	}
}
void QalculateWindow::editKeyboardShortcuts() {
	if(shortcutsDialog) {
		shortcutsDialog->setWindowState((shortcutsDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		shortcutsDialog->show();
		qApp->processEvents();
		shortcutsDialog->raise();
		shortcutsDialog->activateWindow();
		return;
	}
	shortcutsDialog = new QDialog(this);
	try_resize(shortcutsDialog, 700, 500);
	if(settings->always_on_top) shortcutsDialog->setWindowFlags(shortcutsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	shortcutsDialog->setWindowTitle(tr("Keyboard Shortcuts"));
	QVBoxLayout *box = new QVBoxLayout(shortcutsDialog);
	QGridLayout *grid = new QGridLayout();
	box->addLayout(grid);
	shortcutList = new QTreeWidget(shortcutsDialog);
	shortcutList->setSelectionMode(QAbstractItemView::SingleSelection);
	shortcutList->setRootIsDecorated(false);
	shortcutList->setColumnCount(2);
	QStringList list; list << tr("Action"); list << tr("Key combination");
	shortcutList->setHeaderLabels(list);
	shortcutList->header()->setVisible(true);
	shortcutList->header()->setStretchLastSection(false);
	shortcutList->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	shortcutList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

	for(size_t i = 0; i < settings->keyboard_shortcuts.size(); i++) {
		QTreeWidgetItem *item = new QTreeWidgetItem(shortcutList);
		item->setText(0, settings->shortcutText(settings->keyboard_shortcuts[i]->type, settings->keyboard_shortcuts[i]->value));
		item->setText(1, QKeySequence::fromString(settings->keyboard_shortcuts[i]->key).toString());
		item->setData(0, Qt::UserRole, QVariant::fromValue((void*) settings->keyboard_shortcuts[i]));
	}
	shortcutList->setCurrentItem(NULL);
	shortcutList->setSortingEnabled(true);
	shortcutList->sortByColumn(1, Qt::AscendingOrder);
	connect(shortcutList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(shortcutDoubleClicked(QTreeWidgetItem*, int)));
	connect(shortcutList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(currentShortcutChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	grid->addWidget(shortcutList);
	QVBoxLayout *vbox = new QVBoxLayout();
	grid->addLayout(vbox, 0, 1);
	addShortcutButton = new QPushButton(tr("Add…"), this);
	connect(addShortcutButton, SIGNAL(clicked()), this, SLOT(addShortcutClicked()));
	vbox->addWidget(addShortcutButton);
	editShortcutButton = new QPushButton(tr("Edit…"), this);
	editShortcutButton->setEnabled(false);
	connect(editShortcutButton, SIGNAL(clicked()), this, SLOT(editShortcutClicked()));
	vbox->addWidget(editShortcutButton);
	removeShortcutButton = new QPushButton(tr("Remove"), this);
	removeShortcutButton->setEnabled(false);
	connect(removeShortcutButton, SIGNAL(clicked()), this, SLOT(removeShortcutClicked()));
	vbox->addWidget(removeShortcutButton);
	vbox->addStretch(1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, shortcutsDialog);
	box->addWidget(buttonBox);
	connect(buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), shortcutsDialog, SLOT(reject()));
	shortcutsDialog->show();
}
void QalculateWindow::editPreferences() {
	if(preferencesDialog) {
		preferencesDialog->setWindowState((preferencesDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		preferencesDialog->show();
		qApp->processEvents();
		preferencesDialog->raise();
		preferencesDialog->activateWindow();
		return;
	}
	preferencesDialog = new PreferencesDialog(this);
	connect(preferencesDialog, SIGNAL(resultFormatUpdated()), this, SLOT(resultFormatUpdated()));
	connect(preferencesDialog, SIGNAL(resultDisplayUpdated()), this, SLOT(resultDisplayUpdated()));
	connect(preferencesDialog, SIGNAL(expressionFormatUpdated(bool)), this, SLOT(expressionFormatUpdated(bool)));
	connect(preferencesDialog, SIGNAL(expressionCalculationUpdated(int)), this, SLOT(expressionCalculationUpdated(int)));
	connect(preferencesDialog, SIGNAL(alwaysOnTopChanged()), this, SLOT(onAlwaysOnTopChanged()));
	connect(preferencesDialog, SIGNAL(enableTooltipsChanged()), this, SLOT(onEnableTooltipsChanged()));
	connect(preferencesDialog, SIGNAL(titleTypeChanged()), this, SLOT(onTitleTypeChanged()));
	connect(preferencesDialog, SIGNAL(resultFontChanged()), this, SLOT(onResultFontChanged()));
	connect(preferencesDialog, SIGNAL(expressionFontChanged()), this, SLOT(onExpressionFontChanged()));
	connect(preferencesDialog, SIGNAL(keypadFontChanged()), this, SLOT(onKeypadFontChanged()));
	connect(preferencesDialog, SIGNAL(appFontChanged()), this, SLOT(onAppFontChanged()));
	connect(preferencesDialog, SIGNAL(symbolsUpdated()), keypad, SLOT(updateSymbols()));
	connect(preferencesDialog, SIGNAL(historyExpressionTypeChanged()), historyView, SLOT(reloadHistory()));
	connect(preferencesDialog, SIGNAL(binaryBitsChanged()), this, SLOT(onBinaryBitsChanged()));
	connect(preferencesDialog, SIGNAL(statusModeChanged()), this, SLOT(onExpressionStatusModeChanged()));
	connect(preferencesDialog, SIGNAL(dialogClosed()), this, SLOT(onPreferencesClosed()));
	if(settings->always_on_top) preferencesDialog->setWindowFlags(preferencesDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	preferencesDialog->show();
}
void QalculateWindow::onDatasetsChanged() {
	expressionEdit->updateCompletion();
	if(functionsDialog) functionsDialog->updateFunctions();
	functionsMenu->clear();
}
void QalculateWindow::onFunctionsChanged() {
	expressionEdit->updateCompletion();
	if(datasetsDialog) datasetsDialog->updateDatasets();
	functionsMenu->clear();
}
void QalculateWindow::insertProperty(DataObject *o, DataProperty *dp) {
	expressionEdit->blockCompletion();
	DataSet *ds = dp->parentSet();
	std::string str = ds->preferredDisplayName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true);
	str += "(";
	str += o->getProperty(ds->getPrimaryKeyProperty());
	str += CALCULATOR->getComma();
	str += " ";
	str += dp->getName();
	str += ")";
	expressionEdit->insertPlainText(QString::fromStdString(str));
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::openDatasets() {
	if(datasetsDialog) {
		datasetsDialog->setWindowState((datasetsDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		datasetsDialog->show();
		qApp->processEvents();
		datasetsDialog->raise();
		datasetsDialog->activateWindow();
		return;
	}
	datasetsDialog = new DataSetsDialog();
	connect(datasetsDialog, SIGNAL(itemsChanged()), this, SLOT(onDatasetsChanged()));
	connect(datasetsDialog, SIGNAL(insertPropertyRequest(DataObject*, DataProperty*)), this, SLOT(insertProperty(DataObject*, DataProperty*)));
	if(settings->always_on_top) datasetsDialog->setWindowFlags(datasetsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	datasetsDialog->show();
}
void QalculateWindow::applyFunction(MathFunction *f) {
	if(b_busy) return;
	if(settings->rpn_mode) {
		calculateRPN(f);
		return;
	}
	QString str = QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true));
	if(f->args() == 0) {
		str += "()";
	} else {
		str += "(";
		str += expressionEdit->toPlainText();
		str += ")";
	}
	expressionEdit->blockParseStatus();
	expressionEdit->setExpression(str);
	expressionEdit->blockParseStatus(false);
	calculate();
}
void QalculateWindow::openFunctions() {
	if(functionsDialog) {
		functionsDialog->setWindowState((functionsDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		functionsDialog->show();
		qApp->processEvents();
		functionsDialog->raise();
		functionsDialog->activateWindow();
		return;
	}
	functionsDialog = new FunctionsDialog();
	connect(functionsDialog, SIGNAL(itemsChanged()), this, SLOT(onFunctionsChanged()));
	connect(functionsDialog, SIGNAL(favouritesChanged()), this, SLOT(updateFavouriteFunctions()));
	connect(functionsDialog, SIGNAL(applyFunctionRequest(MathFunction*)), this, SLOT(applyFunction(MathFunction*)));
	connect(functionsDialog, SIGNAL(applyFunctionRequest(MathFunction*)), this, SLOT(addToRecentFunctions(MathFunction*)));
	connect(functionsDialog, SIGNAL(insertFunctionRequest(MathFunction*)), this, SLOT(onInsertFunctionRequested(MathFunction*)));
	connect(functionsDialog, SIGNAL(insertFunctionRequest(MathFunction*)), this, SLOT(addToRecentFunctions(MathFunction*)));
	connect(functionsDialog, SIGNAL(calculateFunctionRequest(MathFunction*)), this, SLOT(onCalculateFunctionRequested(MathFunction*)));
	connect(functionsDialog, SIGNAL(calculateFunctionRequest(MathFunction*)), this, SLOT(addToRecentFunctions(MathFunction*)));
	if(settings->always_on_top) functionsDialog->setWindowFlags(functionsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	functionsDialog->show();
}
void QalculateWindow::onUnitRemoved(Unit *u) {
	if(unitsDialog) unitsDialog->unitRemoved(u);
}
void QalculateWindow::onUnitDeactivated(Unit *u) {
	if(unitsDialog) unitsDialog->unitDeactivated(u);
}
void QalculateWindow::openVariables() {
	if(variablesDialog) {
		variablesDialog->setWindowState((variablesDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		variablesDialog->show();
		qApp->processEvents();
		variablesDialog->raise();
		variablesDialog->activateWindow();
		return;
	}
	variablesDialog = new VariablesDialog();
	connect(variablesDialog, SIGNAL(itemsChanged()), expressionEdit, SLOT(updateCompletion()));
	connect(variablesDialog, SIGNAL(itemsChanged()), unitsMenu, SLOT(clear()));
	connect(variablesDialog, SIGNAL(itemsChanged()), variablesMenu, SLOT(clear()));
	connect(variablesDialog, SIGNAL(favouritesChanged()), this, SLOT(updateFavouriteVariables()));
	connect(variablesDialog, SIGNAL(unitRemoved(Unit*)), this, SLOT(onUnitRemoved(Unit*)));
	connect(variablesDialog, SIGNAL(unitDeactivated(Unit*)), this, SLOT(onUnitDeactivated(Unit*)));
	connect(variablesDialog, SIGNAL(insertVariableRequest(Variable*)), this, SLOT(onVariableClicked(Variable*)));
	connect(variablesDialog, SIGNAL(insertVariableRequest(Variable*)), this, SLOT(addToRecentVariables(Variable*)));
	if(settings->always_on_top) variablesDialog->setWindowFlags(variablesDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	variablesDialog->show();
}
void QalculateWindow::onVariableRemoved(Variable *v) {
	if(variablesDialog) variablesDialog->variableRemoved(v);
}
void QalculateWindow::onVariableDeactivated(Variable *v) {
	if(variablesDialog) variablesDialog->variableDeactivated(v);
}
void QalculateWindow::openUnits() {
	Unit *u = NULL;
	if(!expressionEdit->expressionHasChanged() && !settings->history_answer.empty()) {
		u = CALCULATOR->findMatchingUnit(*mstruct);
	} else if(!auto_result.empty()) {
		u = CALCULATOR->findMatchingUnit(mauto);
	}
	if(unitsDialog) {
		if(u && !u->category().empty()) unitsDialog->selectCategory(u->category());
		else unitsDialog->selectCategory("All");
		unitsDialog->setWindowState((unitsDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		unitsDialog->show();
		qApp->processEvents();
		unitsDialog->raise();
		unitsDialog->activateWindow();
		return;
	}
	unitsDialog = new UnitsDialog();
	if(u && !u->category().empty()) unitsDialog->selectCategory(u->category());
	connect(unitsDialog, SIGNAL(itemsChanged()), expressionEdit, SLOT(updateCompletion()));
	connect(unitsDialog, SIGNAL(itemsChanged()), unitsMenu, SLOT(clear()));
	connect(unitsDialog, SIGNAL(itemsChanged()), variablesMenu, SLOT(clear()));
	connect(unitsDialog, SIGNAL(favouritesChanged()), this, SLOT(updateFavouriteUnits()));
	connect(unitsDialog, SIGNAL(variableRemoved(Variable*)), this, SLOT(onVariableRemoved(Variable*)));
	connect(unitsDialog, SIGNAL(variableDeactivated(Variable*)), this, SLOT(onVariableDeactivated(Variable*)));
	connect(unitsDialog, SIGNAL(insertUnitRequest(Unit*)), this, SLOT(onUnitClicked(Unit*)));
	connect(unitsDialog, SIGNAL(insertUnitRequest(Unit*)), this, SLOT(addToRecentUnits(Unit*)));
	connect(unitsDialog, SIGNAL(convertToUnitRequest(Unit*)), this, SLOT(convertToUnit(Unit*)));
	connect(unitsDialog, SIGNAL(unitActivated(Unit*)), this, SLOT(onUnitActivated(Unit*)));
	if(settings->always_on_top) unitsDialog->setWindowFlags(unitsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	unitsDialog->show();
}
void remove_nonunits(MathStructure &m) {
	if(m.isUnit()) {
		if(m.unit()->defaultPrefix() != 0) m.setPrefix(CALCULATOR->getExactDecimalPrefix(m.unit()->defaultPrefix()));
		else m.setPrefix(NULL);
	}
	if(m.size() > 0) {
		for(size_t i = 0; i < m.size();) {
			if(!m.isUnit_exp() && !m[i].containsType(STRUCT_UNIT, true)) {
				m.delChild(i + 1);
			} else {
				remove_nonunits(m[i]);
				i++;
			}
		}
		if(m.size() == 1) m.setToChild(1);
		else if(m.size() == 0) m.clear();
	}
}
void QalculateWindow::onUnitActivated(Unit *u) {
	if(expressionEdit->expressionHasChanged() || settings->history_answer.empty() || ((settings->replace_expression != CLEAR_EXPRESSION || !expressionEdit->document()->isEmpty()) && (expressionEdit->document()->isEmpty() || expressionEdit->selectedText() != expressionEdit->toPlainText())) || !mstruct) {
		onUnitClicked(u);
		addToRecentUnits(u);
	} else {
		if(!mstruct->containsType(STRUCT_UNIT, true)) {
			MathStructure m_u(u);
			m_u.unformat(settings->evalops);
			MathStructure moptimal(CALCULATOR->convertToOptimalUnit(m_u, settings->evalops, true));
			remove_nonunits(moptimal);
			moptimal.unformat(settings->evalops);
			if(moptimal.compare(m_u) == COMPARISON_RESULT_EQUAL) {
				onUnitClicked(u);
				return;
			}
		}
		convertToUnit(u);
	}
}
void QalculateWindow::openFPConversion() {
	if(fpConversionDialog) {
		fpConversionDialog->setWindowState((fpConversionDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		fpConversionDialog->show();
		qApp->processEvents();
		fpConversionDialog->raise();
		fpConversionDialog->activateWindow();
	} else {
		fpConversionDialog = new FPConversionDialog(this);
		if(settings->always_on_top) fpConversionDialog->setWindowFlags(fpConversionDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		fpConversionDialog->show();
	}
	if(!expressionEdit->expressionHasChanged() && !settings->history_answer.empty()) {
		if(mstruct && mstruct->isNumber()) {
			fpConversionDialog->setValue(*mstruct);
		}
	} else if(!auto_result.empty()) {
		if(mauto.isNumber()) {
			fpConversionDialog->setValue(mauto);
		}
	} else {
		QString str = expressionEdit->selectedText(true).trimmed();
		int base = settings->evalops.parse_options.base;
		if(base <= BASE_FP16 && base >= BASE_FP80) base = BASE_BINARY;
		if(str.isEmpty()) {
			fpConversionDialog->clear();
		} else {
			switch(base) {
				case BASE_BINARY: {
					fpConversionDialog->setBin(str);
					break;
				}
				case BASE_HEXADECIMAL: {
					fpConversionDialog->setHex(str);
					break;
				}
				case BASE_DECIMAL: {
					fpConversionDialog->setValue(str);
					break;
				}
				default: {
					fpConversionDialog->clear();
					break;
				}
			}
		}
	}
}
void QalculateWindow::openPercentageCalculation() {
	if(percentageDialog) {
		percentageDialog->setWindowState((percentageDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		percentageDialog->show();
		qApp->processEvents();
		percentageDialog->raise();
		percentageDialog->activateWindow();
	} else {
		percentageDialog = new PercentageCalculationDialog(this);
		if(settings->always_on_top) percentageDialog->setWindowFlags(percentageDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		percentageDialog->show();
	}
	QString str;
	if(!expressionEdit->expressionHasChanged() && !settings->history_answer.empty()) {
		if(mstruct && mstruct->isNumber()) {
			str = QString::fromStdString(unhtmlize(result_text));
		}
	} else if(!auto_result.empty()) {
		if(mauto.isNumber()) {
			str = QString::fromStdString(unhtmlize(auto_result));
		}
	} else {
		str = expressionEdit->selectedText(true);
	}
	percentageDialog->resetValues(str);
}
void QalculateWindow::openPlot() {
	if(!CALCULATOR->canPlot()) {
		QMessageBox::critical(this, tr("Gnuplot was not found"), tr("%1 (%2) needs to be installed separately, and found in the executable search path, for plotting to work.").arg("Gnuplot").arg("<a href=\"http://www.gnuplot.info/\">http://www.gnuplot.info/</a>"), QMessageBox::Ok);
		return;
	}
	if(plotDialog) {
		plotDialog->setWindowState((plotDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		plotDialog->show();
		qApp->processEvents();
		plotDialog->raise();
		plotDialog->activateWindow();
	} else {
		plotDialog = new PlotDialog(this);
		if(settings->always_on_top) plotDialog->setWindowFlags(plotDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		plotDialog->show();
	}
	if(settings->evalops.parse_options.base == 10) {
		std::string str = expressionEdit->selectedText(true).toStdString(), str2;
		CALCULATOR->separateToExpression(str, str2, settings->evalops, true);
		remove_blank_ends(str);
		plotDialog->setExpression(QString::fromStdString(str));
	} else {
		plotDialog->setExpression(QString());
	}
}
void QalculateWindow::openPeriodicTable() {
	if(periodicTableDialog) {
		periodicTableDialog->setWindowState((periodicTableDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		periodicTableDialog->show();
		qApp->processEvents();
		periodicTableDialog->raise();
		periodicTableDialog->activateWindow();
	} else {
		periodicTableDialog = new PeriodicTableDialog(this);
		connect(periodicTableDialog, SIGNAL(insertPropertyRequest(DataObject*, DataProperty*)), this, SLOT(insertProperty(DataObject*, DataProperty*)));
		if(settings->always_on_top) periodicTableDialog->setWindowFlags(periodicTableDialog->windowFlags() | Qt::WindowStaysOnTopHint);
		periodicTableDialog->show();
	}
}
void QalculateWindow::openCalendarConversion() {
	if(calendarConversionDialog) {
		if(!auto_result.empty() && mauto.isDateTime()) calendarConversionDialog->setDate(*mauto.datetime());
		else if(auto_result.empty() && mstruct && mstruct->isDateTime()) calendarConversionDialog->setDate(*mstruct->datetime());
		calendarConversionDialog->setWindowState((calendarConversionDialog->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		calendarConversionDialog->show();
		qApp->processEvents();
		calendarConversionDialog->raise();
		calendarConversionDialog->activateWindow();
		return;
	}
	calendarConversionDialog = new CalendarConversionDialog(this);
	if(settings->always_on_top) calendarConversionDialog->setWindowFlags(calendarConversionDialog->windowFlags() | Qt::WindowStaysOnTopHint);
	QalculateDateTime dt;
	if(!auto_result.empty() && mauto.isDateTime()) dt.set(*mauto.datetime());
	else if(auto_result.empty() && mstruct && mstruct->isDateTime()) dt.set(*mstruct->datetime());
	else dt.setToCurrentDate();
	calendarConversionDialog->setDate(dt);
	calendarConversionDialog->show();
}

struct FunctionDialog {
	MathFunction *f;
	QDialog *dialog;
	QPushButton *b_cancel, *b_exec, *b_insert;
	QCheckBox *b_keepopen;
	QLabel *w_result;
	QScrollArea *w_scrollresult;
	std::vector<QLabel*> label;
	std::vector<QWidget*> entry;
	std::vector<QRadioButton*> boolean_buttons;
	std::vector<int> boolean_index;
	bool add_to_menu, keep_open, rpn;
	int args;
};

MathSpinBox::MathSpinBox(QWidget *parent) : QSpinBox(parent) {
	setLineEdit(new MathLineEdit(this));
	connect(lineEdit(), SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));
#ifndef _WIN32
	setAttribute(Qt::WA_InputMethodEnabled, settings->enable_input_method);
#endif
}
MathSpinBox::~MathSpinBox() {}

int MathSpinBox::valueFromText(const QString &text) const {
	if(settings->evalops.parse_options.base != BASE_DECIMAL) return QSpinBox::valueFromText(text);
	std::string str = text.toStdString();
	if(str.find_first_not_of(NUMBERS) == std::string::npos) {
		return text.toInt();
	}
	MathStructure value;
	CALCULATOR->beginTemporaryStopMessages();
	EvaluationOptions eo = settings->evalops;
	eo.parse_options.base = 10;
	CALCULATOR->calculate(&value, CALCULATOR->unlocalizeExpression(str, eo.parse_options), 100, eo);
	CALCULATOR->endTemporaryStopMessages();
	return value.number().intValue();
}
QValidator::State MathSpinBox::validate(QString &text, int &pos) const {
	if(settings->evalops.parse_options.base != BASE_DECIMAL) return QSpinBox::validate(text, pos);
	std::string str = text.trimmed().toStdString();
	if(str.empty()) return QValidator::Intermediate;
	return QValidator::Acceptable;
}
QLineEdit *MathSpinBox::entry() const {
	return lineEdit();
}
void MathSpinBox::keyPressEvent(QKeyEvent *event) {
	QSpinBox::keyPressEvent(event);
	if(event->key() == Qt::Key_Return) event->accept();
}

class MathDateTimeEdit : public QDateTimeEdit {

	public:

		MathDateTimeEdit(QWidget *parent = NULL) : QDateTimeEdit(parent) {}
		virtual ~MathDateTimeEdit() {}

		QLineEdit *entry() const {return lineEdit();}

	protected:

		void keyPressEvent(QKeyEvent *event) override {
			QDateTimeEdit::keyPressEvent(event);
			if(event->key() == Qt::Key_Return) event->accept();
		}

};

void QalculateWindow::onCalculateFunctionRequested(MathFunction *f) {
	insertFunction(f, functionsDialog);
}
void QalculateWindow::onInsertFunctionRequested(MathFunction *f) {
	expressionEdit->blockCompletion();
	if(!expressionEdit->expressionHasChanged()) {
		expressionEdit->blockUndo(true);
		expressionEdit->clear();
		expressionEdit->blockUndo(false);
	}
	QTextCursor cur = expressionEdit->textCursor();
	expressionEdit->wrapSelection(QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true)), true, true);
	if(!expressionEdit->hasFocus()) expressionEdit->setFocus();
	expressionEdit->blockCompletion(false);
}
void QalculateWindow::insertFunction(MathFunction *f, QWidget *parent) {
	if(!f) return;

	//if function takes no arguments, do not display dialog and insert function directly
	if(f->args() == 0) {
		expressionEdit->insertPlainText(QString::fromStdString(f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true)) + "()");
		//function_inserted(f);
		return;
	}

	FunctionDialog *fd = new FunctionDialog;

	int args = 0;
	bool has_vector = false;
	if(f->args() > 0) {
		args = f->args();
	} else if(f->minargs() > 0) {
		args = f->minargs();
		while(!f->getDefaultValue(args + 1).empty()) args++;
		args++;
	} else {
		args = 1;
		has_vector = true;
	}
	fd->args = args;
	fd->rpn = settings->rpn_mode && expressionEdit->document()->isEmpty() && CALCULATOR->RPNStackSize() >= (f->minargs() <= 0 ? 1 : (size_t) f->minargs());
	fd->add_to_menu = true;
	fd->f = f;

	std::string f_title = f->title(true, settings->printops.use_unicode_signs, &can_display_unicode_string_function, (void*) this);
	fd->dialog = new QDialog(parent ? parent : this);
	if(settings->always_on_top) fd->dialog->setWindowFlags(fd->dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	fd->dialog->setWindowTitle(QString::fromStdString(f_title));

	QVBoxLayout *box = new QVBoxLayout(fd->dialog);

	box->addSpacing(box->spacing());
	QLabel *titleLabel = new QLabel("<big>" + QString::fromStdString(f_title) + "</big>");
	box->addWidget(titleLabel);
	box->addSpacing(box->spacing());

	QGridLayout *table = new QGridLayout();
	box->addLayout(table);

	box->addSpacing(box->spacing());
	if(!f->description().empty() || !f->example(true).empty()) {
		QPlainTextEdit *descr = new QPlainTextEdit();
		descr->setReadOnly(true);
		box->addWidget(descr);
		QString str = QString::fromStdString(f->description());
		if(!f->example(true).empty()) {
			if(!str.isEmpty()) str += "\n\n";
			str += tr("Example:", "Example of function usage");
			str += " ";
			str += QString::fromStdString(f->example(false));
		}
		str.replace(">=", SIGN_GREATER_OR_EQUAL);
		str.replace("<=", SIGN_LESS_OR_EQUAL);
		str.replace("!=", SIGN_NOT_EQUAL);
		str.replace("...", "…");
		descr->setPlainText(str);
		int n = str.count("\n");
		if(n < 5) n = 5;
		QFontMetrics fm(descr->font());
		descr->setFixedHeight(fm.lineSpacing() * n + descr->frameWidth() * 2 + descr->contentsMargins().top() + descr->contentsMargins().bottom());
	}

	fd->w_scrollresult = new QScrollArea();
	fd->w_scrollresult->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	fd->w_result = new QLabel();
	fd->w_result->setTextInteractionFlags(Qt::TextSelectableByMouse);
	fd->w_result->setWordWrap(true);
	fd->w_result->setAlignment(Qt::AlignRight);
	QFont font(fd->w_result->font());
	font.setWeight(QFont::Bold);
	QFontMetrics fm(font);
	fd->w_scrollresult->setMinimumWidth(fm.averageCharWidth() * 40);
	titleLabel->setMinimumWidth(fm.averageCharWidth() * 40);
	fd->w_scrollresult->setWidget(fd->w_result);
	fd->w_scrollresult->setWidgetResizable(true);
	fd->w_scrollresult->setFrameShape(QFrame::NoFrame);
	fd->w_scrollresult->setFixedHeight(fm.lineSpacing() * 2 + fd->w_scrollresult->frameWidth() * 2 + fd->w_scrollresult->contentsMargins().top() + fd->w_scrollresult->contentsMargins().bottom());
	box->addWidget(fd->w_scrollresult, Qt::AlignRight);

	QHBoxLayout *hbox = new QHBoxLayout();
	box->addLayout(hbox);
	fd->b_keepopen = new QCheckBox(tr("Keep open"));
	fd->keep_open = settings->keep_function_dialog_open;
	fd->b_keepopen->setChecked(fd->keep_open);
	hbox->addWidget(fd->b_keepopen);
	hbox->addStretch(1);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, fd->dialog);
	hbox->addWidget(buttonBox);
	fd->b_cancel = buttonBox->button(QDialogButtonBox::Close);
	fd->b_exec = buttonBox->addButton(settings->rpn_mode ? tr("Enter", "RPN Enter") : tr("Calculate"), QDialogButtonBox::ApplyRole);
	fd->b_insert = buttonBox->addButton(settings->rpn_mode ? tr("Apply to Stack") : tr("Insert"), QDialogButtonBox::AcceptRole);
	if(settings->rpn_mode && CALCULATOR->RPNStackSize() < (f->minargs() <= 0 ? 1 : (size_t) f->minargs())) fd->b_insert->setEnabled(false);

	fd->label.resize(args, NULL);
	fd->entry.resize(args, NULL);
	fd->boolean_index.resize(args, 0);

	int bindex = 0;
	QString argstr, typestr, defstr;
	QString freetype = QString::fromStdString(Argument().printlong());
	Argument *arg;
	int r = 0;
	for(int i = 0; i < args; i++) {
		arg = f->getArgumentDefinition(i + 1);
		if(!arg || arg->name().empty()) {
			if(args == 1) {
				argstr = tr("Value");
			} else {
				argstr = tr("Argument");
				if(i > 0 || f->maxargs() != 1) {
					argstr += " ";
					argstr += QString::number(i + 1);
				}
			}
		} else {
			argstr = QString::fromStdString(arg->name());
		}
		typestr = "";
		ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
		defstr = QString::fromStdString(CALCULATOR->localizeExpression(f->getDefaultValue(i + 1), pa));
		if(arg && (arg->suggestsQuotes() || arg->type() == ARGUMENT_TYPE_TEXT) && defstr.length() >= 2 && defstr[0] == '\"' && defstr[defstr.length() - 1] == '\"') {
			defstr = defstr.mid(1, defstr.length() - 2);
		}
		fd->label[i] = new QLabel(tr("%1:").arg(argstr));
		fd->label[i]->setAlignment(Qt::AlignRight);
		QWidget *entry = NULL;
		if(arg) {
			if(arg->type() == ARGUMENT_TYPE_INTEGER) {
				IntegerArgument *iarg = (IntegerArgument*) arg;
				int min = INT_MIN, max = INT_MAX;
				if(iarg->min()) {
					min = iarg->min()->intValue();
				}
				if(iarg->max()) {
					max = iarg->max()->intValue();
				}
				MathSpinBox *spin = new MathSpinBox();
				entry = spin;
				fd->entry[i] = spin->entry();
				spin->setRange(min, max);
				if(!defstr.isEmpty()) {
					spin->setValue(defstr.toInt());
				} else if(!arg->zeroForbidden() && min <= 0 && max >= 0) {
					spin->setValue(0);
				} else {
					if(max < 0) {
						spin->setValue(max);
					} else if(min <= 1) {
						spin->setValue(1);
					} else {
						spin->setValue(min);
					}
				}
				spin->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
				connect(spin, SIGNAL(textChanged(const QString&)), this, SLOT(onInsertFunctionChanged()));
				connect(spin, SIGNAL(valueChanged(int)), this, SLOT(onInsertFunctionChanged()));
				connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
			} else if(arg->type() == ARGUMENT_TYPE_DATE) {
				MathDateTimeEdit *dateEdit = new MathDateTimeEdit();
				entry = dateEdit;
				if(defstr == "now") {
					dateEdit->setDateTime(QDateTime::currentDateTime());
					dateEdit->setDisplayFormat("yyyy-MM-ddTHH:mm:ss");
				} else {
					dateEdit->setDate(QDate::currentDate());
					dateEdit->setDisplayFormat("yyyy-MM-dd");
					dateEdit->setCalendarPopup(true);
				}
				dateEdit->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
				fd->entry[i] = dateEdit->entry();
				connect(dateEdit, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(onInsertFunctionChanged()));
				connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
			} else if(arg->type() == ARGUMENT_TYPE_BOOLEAN) {
				fd->boolean_index[i] = bindex;
				bindex += 2;
				fd->entry[i] = new QWidget();
				hbox = new QHBoxLayout(fd->entry[i]);
				QButtonGroup *group = new QButtonGroup(this); group->setExclusive(true);
				QRadioButton *w = new QRadioButton(tr("True")); group->addButton(w); hbox->addWidget(w);
				w->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
				fd->boolean_buttons.push_back(w);
				w = new QRadioButton(tr("False")); group->addButton(w); hbox->addWidget(w);
				w->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
				fd->boolean_buttons.push_back(w); w->setChecked(true);
				hbox->addStretch(1);
				connect(fd->boolean_buttons[fd->boolean_buttons.size() - 2], SIGNAL(toggled(bool)), this, SLOT(onInsertFunctionChanged()));
				connect(fd->boolean_buttons[fd->boolean_buttons.size() - 1], SIGNAL(toggled(bool)), this, SLOT(onInsertFunctionChanged()));
			} else if(arg->type() == ARGUMENT_TYPE_DATA_PROPERTY && f->subtype() == SUBTYPE_DATA_SET) {
					QComboBox *w = new QComboBox();
					fd->entry[i] = w;
					DataPropertyIter it;
					DataSet *ds = (DataSet*) f;
					DataProperty *dp = ds->getFirstProperty(&it);
					if(fd->rpn && (size_t) i < CALCULATOR->RPNStackSize()) {
						QTableWidgetItem *item = rpnView->item(i, 0);
						if(item) defstr = item->text();
					}
					int active_index = -1;
					for(int i2 = 0; dp; i2++) {
						if(!dp->isHidden()) {
							w->addItem(QString::fromStdString(dp->title()), QVariant::fromValue((void*) dp));
							if(active_index < 0 && defstr.toStdString() == dp->getName()) {
								active_index = i2;
							}
						}
						dp = ds->getNextProperty(&it);
					}
					w->addItem(tr("Info"), QVariant::fromValue((void*) NULL));
					if(active_index < 0) active_index = w->count() - 1;
					w->setCurrentIndex(active_index);
					connect(w, SIGNAL(currentTextChanged(const QString&)), this, SLOT(onInsertFunctionChanged()));
			} else {
				typestr = QString::fromStdString(arg->printlong());
				if(typestr == freetype) typestr = "";
				if(arg->type() == ARGUMENT_TYPE_DATA_OBJECT && f->subtype() == SUBTYPE_DATA_SET && ((DataSet*) f)->getPrimaryKeyProperty()) {
					QComboBox *combo = new QComboBox();
					combo->setEditable(true);
					DataObjectIter it;
					DataSet *ds = (DataSet*) f;
					DataObject *obj = ds->getFirstObject(&it);
					DataProperty *dp = ds->getProperty("name");
					if(!dp || !dp->isKey()) dp = ds->getPrimaryKeyProperty();
					while(obj) {
						combo->addItem(QString::fromStdString(obj->getPropertyInputString(dp)));
						obj = ds->getNextObject(&it);
					}
					combo->setCurrentText(QString());
					fd->entry[i] = combo->lineEdit();
					entry = combo;
				} else if(i == 1 && f->id() == FUNCTION_ID_ASCII && arg->type() == ARGUMENT_TYPE_TEXT) {
					QComboBox *combo = new QComboBox();
					combo->setEditable(true);
					combo->addItem("UTF-8");
					combo->addItem("UTF-16");
					combo->addItem("UTF-32");
					fd->entry[i] = combo->lineEdit();
					entry = combo;
				} else if(i == 3 && f->id() == FUNCTION_ID_DATE && arg->type() == ARGUMENT_TYPE_TEXT) {
					QComboBox *combo = new QComboBox();
					combo->setEditable(true);
					combo->addItem("chinese");
					combo->addItem("coptic");
					combo->addItem("egyptian");
					combo->addItem("ethiopian");
					combo->addItem("gregorian");
					combo->addItem("hebrew");
					combo->addItem("indian");
					combo->addItem("islamic");
					combo->addItem("julian");
					combo->addItem("milankovic");
					combo->addItem("persian");
					fd->entry[i] = combo->lineEdit();
					entry = combo;
				} else if(USE_QUOTES(arg, f)) {
					fd->entry[i] = new QLineEdit();
				} else {
					fd->entry[i] = new MathLineEdit();
				}
				if(i >= f->minargs() && !has_vector) {
					((QLineEdit*) fd->entry[i])->setPlaceholderText(tr("optional", "optional argument"));
				}
				connect(fd->entry[i], SIGNAL(textEdited(const QString&)), this, SLOT(onInsertFunctionChanged()));
				connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
			}
		} else {
			fd->entry[i] = new MathLineEdit();
			if(i >= f->minargs() && !has_vector) {
				((QLineEdit*) fd->entry[i])->setPlaceholderText(tr("optional", "optional argument"));
			}
			connect(fd->entry[i], SIGNAL(textEdited(const QString&)), this, SLOT(onInsertFunctionChanged()));
			connect(fd->entry[i], SIGNAL(returnPressed()), this, SLOT(onInsertFunctionEntryActivated()));
		}
		if(arg && arg->type() == ARGUMENT_TYPE_FILE) {
			QAction *action = ((QLineEdit*) fd->entry[i])->addAction(LOAD_ICON("document-open"), QLineEdit::TrailingPosition);
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
			((QLineEdit*) fd->entry[i])->setTextMargins(0, 0, 22, 0);
#	endif
#endif
			action->setProperty("QALCULATE ENTRY", QVariant::fromValue((void*) fd->entry[i]));
			typestr = "";
			connect(action, SIGNAL(triggered()), this, SLOT(onEntrySelectFile()));
		} else if(arg && arg->type() == ARGUMENT_TYPE_MATRIX) {
			QAction *action = ((QLineEdit*) fd->entry[i])->addAction(LOAD_ICON("table"), QLineEdit::TrailingPosition);
#ifdef _WIN32
#	if (QT_VERSION < QT_VERSION_CHECK(6, 2, 0))
			((QLineEdit*) fd->entry[i])->setTextMargins(0, 0, 22, 0);
#	endif
#endif
			action->setProperty("QALCULATE ENTRY", QVariant::fromValue((void*) fd->entry[i]));
			typestr = "";
			connect(action, SIGNAL(triggered()), this, SLOT(onEntryEditMatrix()));
		}
		if(fd->rpn && (size_t) i < CALCULATOR->RPNStackSize()) {
			QTableWidgetItem *item = rpnView->item(i, 0);
			if(item) {
				if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
					if(item->text() == "1") {
						fd->boolean_buttons[fd->boolean_buttons.size() - 2]->blockSignals(true);
						fd->boolean_buttons[fd->boolean_buttons.size() - 1]->setChecked(false);
						fd->boolean_buttons[fd->boolean_buttons.size() - 2]->setChecked(true);
						fd->boolean_buttons[fd->boolean_buttons.size() - 2]->blockSignals(false);
					}
				} else if(!arg || arg->type() != ARGUMENT_TYPE_DATA_PROPERTY || f->subtype() != SUBTYPE_DATA_SET) {
					fd->entry[i]->blockSignals(true);
					if(i == 0 && args == 1 && (has_vector || arg->type() == ARGUMENT_TYPE_VECTOR)) {
						QString rpn_vector = item->text();
						for(int i2 = i + 1; i2 < rpnView->rowCount(); i2++) {
							item = rpnView->item(i, 0);
							if(item) {
								rpn_vector += QString::fromStdString(CALCULATOR->getComma());
								rpn_vector += " ";
								rpn_vector += item->text();
							}
						}
						((QLineEdit*) fd->entry[i])->setText(rpn_vector);
					} else {
						((QLineEdit*) fd->entry[i])->setText(item->text());
					}
					fd->entry[i]->blockSignals(false);
				}
			}
		} else if(arg && arg->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(defstr == "1") {
				fd->boolean_buttons[fd->boolean_buttons.size() - 2]->blockSignals(true);
				fd->boolean_buttons[fd->boolean_buttons.size() - 1]->setChecked(false);
				fd->boolean_buttons[fd->boolean_buttons.size() - 2]->setChecked(true);
				fd->boolean_buttons[fd->boolean_buttons.size() - 2]->blockSignals(false);
			}
		} else if(!arg || arg->type() != ARGUMENT_TYPE_DATA_PROPERTY || f->subtype() != SUBTYPE_DATA_SET) {
			fd->entry[i]->blockSignals(true);
			if(!defstr.isEmpty() && (!arg || (arg->type() != ARGUMENT_TYPE_DATE && arg->type() != ARGUMENT_TYPE_INTEGER)) && (i < f->minargs() || has_vector || (defstr != "undefined" && defstr != "\"\""))) {
				((QLineEdit*) fd->entry[i])->setText(defstr);
			}
			if(i == 0) {
				std::string seltext, str2;
				if(expressionEdit->textCursor().hasSelection()) seltext = expressionEdit->textCursor().selectedText().toStdString();
				else seltext = expressionEdit->toPlainText().toStdString();
				bool use_current_result = (!auto_result.empty() || (!expressionEdit->expressionHasChanged() && !settings->history_answer.empty() && settings->current_result)) && (!expressionEdit->textCursor().hasSelection() || seltext == expressionEdit->toPlainText().toStdString());
				CALCULATOR->separateToExpression(seltext, str2, settings->evalops, true);
				remove_blank_ends(seltext);
				if(!seltext.empty()) {
					if(arg && arg->type() == ARGUMENT_TYPE_INTEGER) {
						MathStructure m;
						CALCULATOR->beginTemporaryStopMessages();
						if(use_current_result) {
							m = auto_result.empty() ? *settings->current_result : mauto;
						} else {
							CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(seltext, settings->evalops.parse_options), 200, settings->evalops);
						}
						if(!CALCULATOR->endTemporaryStopMessages() && m.isInteger()) {
							bool overflow = false;
							int v = m.number().intValue(&overflow);
							QSpinBox *spin = (QSpinBox*) entry;
							if(!overflow && v >= spin->minimum() && v <= spin->maximum()) spin->setValue(v);
						}
					} else if(arg && arg->type() == ARGUMENT_TYPE_DATE) {
						MathStructure m;
						CALCULATOR->beginTemporaryStopMessages();
						if(use_current_result) {
							m = auto_result.empty() ? *settings->current_result : mauto;
						} else {
							CALCULATOR->calculate(&m, CALCULATOR->unlocalizeExpression(seltext, settings->evalops.parse_options), 200, settings->evalops);
						}
						if(!CALCULATOR->endTemporaryStopMessages() && m.isDateTime()) {
							QDateTime d;
							d.setDate(QDate(m.datetime()->year(), m.datetime()->month(), m.datetime()->day()));
							Number nr_sec = m.datetime()->second();
							Number nr_msec(nr_sec); nr_msec.frac(); nr_msec *= 1000; nr_msec.round();
							nr_sec.trunc();
							d.setTime(QTime(m.datetime()->hour(), m.datetime()->minute(), nr_sec.intValue(), nr_msec.intValue()));
							((QDateTimeEdit*) entry)->setDateTime(d);
						}
					} else {
						if(use_current_result) {
							if(exact_text.empty()) {
								Number nr(settings->history_answer.size(), 1, 0);
								((QLineEdit*) fd->entry[i])->setText(QString("%1(%2)").arg(QString::fromStdString(settings->f_answer->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) fd->entry[i]).formattedName(TYPE_FUNCTION, true))).arg(QString::fromStdString(print_with_evalops(nr))));
							} else {
								((QLineEdit*) fd->entry[i])->setText(QString::fromStdString(exact_text));
							}
						} else {
							((QLineEdit*) fd->entry[i])->setText(QString::fromStdString(seltext));
						}
					}
				}
			}
			fd->entry[i]->blockSignals(false);
		}
		table->addWidget(fd->label[i], r, 0, 1, 1);
		if(entry) table->addWidget(entry, r, 1, 1, 1);
		else table->addWidget(fd->entry[i], r, 1, 1, 1);
		r++;
		fd->entry[i]->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
		if(i == 0) fd->entry[i]->setFocus();
		if(!typestr.isEmpty()) {
			typestr.replace(">=", SIGN_GREATER_OR_EQUAL);
			typestr.replace("<=", SIGN_LESS_OR_EQUAL);
			typestr.replace("!=", SIGN_NOT_EQUAL);
			QLabel *w = new QLabel("<i><small>" + typestr.toHtmlEscaped() + THIN_SPACE "</small></i>");
			w->setWordWrap(true);
			w->setAlignment(Qt::AlignRight | Qt::AlignTop);
			table->addWidget(w, r, 1, 1, 1);
			r++;
		}
	}
	table->setColumnStretch(1, 1);

	if(!fd->keep_open) fd->w_scrollresult->hide();
	fd->b_exec->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_cancel->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_insert->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_keepopen->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	fd->b_exec->setDefault(false);
	fd->b_cancel->setDefault(false);
	fd->b_insert->setDefault(false);
	fd->dialog->setProperty("QALCULATE FD", QVariant::fromValue((void*) fd));
	connect(fd->b_exec, SIGNAL(clicked()), this, SLOT(onInsertFunctionExec()));
	if(fd->rpn) connect(fd->b_insert, SIGNAL(clicked()), this, SLOT(onInsertFunctionRPN()));
	else connect(fd->b_insert, SIGNAL(clicked()), this, SLOT(onInsertFunctionInsert()));
	connect(fd->b_cancel, SIGNAL(clicked()), fd->dialog, SLOT(reject()));
	connect(fd->b_keepopen, SIGNAL(toggled(bool)), this, SLOT(onInsertFunctionKeepOpen(bool)));
	connect(fd->dialog, SIGNAL(rejected()), this, SLOT(onInsertFunctionClosed()));

	box->setSizeConstraint(QLayout::SetFixedSize);
	fd->dialog->show();

}
void QalculateWindow::onEntrySelectFile() {
	QLineEdit *w = (QLineEdit*) sender()->property("QALCULATE ENTRY").value<void*>();
	FunctionDialog *fd = (FunctionDialog*) w->property("QALCULATE FD").value<void*>();
	QString str = QFileDialog::getOpenFileName(fd->dialog, QString(), w->text());
	if(!str.isEmpty()) w->setText(str);
}
void QalculateWindow::onEntryEditMatrix() {
	QLineEdit *entry = (QLineEdit*) sender()->property("QALCULATE ENTRY").value<void*>();
	FunctionDialog *fd = (FunctionDialog*) entry->property("QALCULATE FD").value<void*>();
	QDialog *dialog = new QDialog(fd->dialog);
	if(settings->always_on_top) dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
	dialog->setWindowTitle(tr("Matrix"));
	QVBoxLayout *box = new QVBoxLayout(dialog);
	MatrixWidget *w = new MatrixWidget(dialog);
	w->setMatrixString(entry->text());
	box->addWidget(w);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, Qt::Horizontal, dialog);
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
	buttonBox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	box->addWidget(buttonBox);
	w->setFocus();
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
	connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));
	if(dialog->exec() == QDialog::Accepted) {
		if(!w->isEmpty()) {
			entry->setText(w->getMatrixString());
		}
	}
	dialog->deleteLater();
}
void QalculateWindow::insertFunctionDo(FunctionDialog *fd) {
	MathFunction *f = fd->f;
	std::string str = f->preferredInputName(settings->printops.abbreviate_names, settings->printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressionEdit).formattedName(TYPE_FUNCTION, true) + "(", str2;
	int argcount = fd->args;
	if((f->maxargs() < 0 || f->minargs() < f->maxargs()) && argcount > f->minargs()) {
		while(true) {
			ParseOptions pa = settings->evalops.parse_options; pa.base = 10;
			std::string defstr = CALCULATOR->localizeExpression(f->getDefaultValue(argcount), pa);
			remove_blank_ends(defstr);
			if(f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_BOOLEAN) {
				if(fd->boolean_buttons[fd->boolean_index[argcount - 1]]->isChecked()) str2 = "1";
				else str2 = "0";
			} else if(settings->evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_INTEGER) {
				Number nr(((QLineEdit*) fd->entry[argcount - 1])->text().toStdString());
				str2 = print_with_evalops(nr);
			} else if(f->getArgumentDefinition(argcount) && f->getArgumentDefinition(argcount)->type() == ARGUMENT_TYPE_DATA_PROPERTY && f->subtype() == SUBTYPE_DATA_SET) {
				DataProperty *dp = (DataProperty*) ((QComboBox*) fd->entry[argcount - 1])->currentData().value<void*>();
				if(dp) {
					str2 = dp->getName();
				} else {
					str2 = "info";
				}
			} else {
				str2 = ((QLineEdit*) fd->entry[argcount - 1])->text().toStdString();
				remove_blank_ends(str2);
			}
			if(!str2.empty() && USE_QUOTES(f->getArgumentDefinition(argcount), f) && (unicode_length(str2) <= 2 || str2.find_first_of("\"\'") == std::string::npos)) {
				if(str2.find("\"") != std::string::npos) {
					str2.insert(0, "\'");
					str2 += "\'";
				} else {
					str2.insert(0, "\"");
					str2 += "\"";
				}
			}
			if(str2.empty() || str2 == defstr) argcount--;
			else break;
			if(argcount == 0 || argcount == f->minargs()) break;
		}
	}

	int i_vector = f->maxargs() > 0 ? f->maxargs() : argcount;
	for(int i = 0; i < argcount; i++) {
		if(f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_BOOLEAN) {
			if(fd->boolean_buttons[fd->boolean_index[i]]->isChecked()) str2 = "1";
			else str2 = "0";
		} else if((i != (f->maxargs() > 0 ? f->maxargs() : argcount) - 1 || i_vector == i - 1) && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_VECTOR) {
			i_vector = i;
			str2 = ((QLineEdit*) fd->entry[i])->text().toStdString();
			remove_blank_ends(str2);
			if(str2.find_first_of(PARENTHESISS VECTOR_WRAPS) == std::string::npos && str2.find_first_of(CALCULATOR->getComma() == COMMA ? COMMAS : CALCULATOR->getComma()) != std::string::npos) {
				str2.insert(0, 1, '[');
				str2 += ']';
			}
		} else if(settings->evalops.parse_options.base != BASE_DECIMAL && f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_INTEGER) {
			Number nr(((QLineEdit*) fd->entry[i])->text().toStdString());
			str2 = print_with_evalops(nr);
		} else if(f->getArgumentDefinition(i + 1) && f->getArgumentDefinition(i + 1)->type() == ARGUMENT_TYPE_DATA_PROPERTY && f->subtype() == SUBTYPE_DATA_SET) {
			DataProperty *dp = (DataProperty*) ((QComboBox*) fd->entry[i])->currentData().value<void*>();
			if(dp) {
				str2 = dp->getName();
			} else {
				str2 = "info";
			}
		} else {
			str2 = ((QLineEdit*) fd->entry[i])->text().toStdString();
			remove_blank_ends(str2);
		}
		if((i < f->minargs() || !str2.empty()) && USE_QUOTES(f->getArgumentDefinition(i + 1), f) && (unicode_length(str2) <= 2 || str2.find_first_of("\"\'") == std::string::npos)) {
			if(str2.find("\"") != std::string::npos) {
				str2.insert(0, "\'");
				str2 += "\'";
			} else {
				str2.insert(0, "\"");
				str2 += "\"";
			}
		}
		if(i > 0) {
			str += CALCULATOR->getComma();
			str += " ";
		}
		str += str2;
	}
	str += ")";
	expressionEdit->blockCompletion(true);
	expressionEdit->insertPlainText(QString::fromStdString(str));
	expressionEdit->blockCompletion(false);
	//if(fd->add_to_menu) function_inserted(f);
}

void QalculateWindow::onInsertFunctionChanged() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	fd->w_result->clear();
}
void QalculateWindow::onInsertFunctionEntryActivated() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	for(int i = 0; i < fd->args; i++) {
		if(fd->entry[i] == sender()) {
			if(i == fd->args - 1) {
				if(fd->rpn) onInsertFunctionRPN();
				else if(fd->keep_open || settings->rpn_mode) onInsertFunctionExec();
				else onInsertFunctionInsert();
			} else {
				if(fd->f->getArgumentDefinition(i + 2) && fd->f->getArgumentDefinition(i + 2)->type() == ARGUMENT_TYPE_BOOLEAN) {
					fd->boolean_buttons[fd->boolean_index[i + 1]]->setFocus();
				} else {
					fd->entry[i + 1]->setFocus();
				}
			}
			break;
		}
	}
}
void QalculateWindow::onInsertFunctionExec() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	expressionEdit->blockUndo(true);
	expressionEdit->clear();
	expressionEdit->blockUndo(false);
	if(!fd->keep_open) fd->dialog->hide();
	insertFunctionDo(fd);
	calculateExpression();
	if(fd->keep_open) {
		QString str;
		bool b_approx = *settings->printops.is_approximate || (mstruct && mstruct->isApproximate());
		str = "<span font-weight=\"bold\">";
		if(!b_approx) str += "= ";
		else str += SIGN_ALMOST_EQUAL " ";
		if(result_text.length() > 100000) str += QString::fromStdString(ellipsize_result(result_text, 20000));
		else str += QString::fromStdString(result_text);
		str += "</span>";
		fd->w_scrollresult->show();
		fd->w_result->setText(str);
		fd->entry[0]->setFocus();
		expressionEdit->selectAll();
	} else {
		fd->dialog->deleteLater();
		delete fd;
	}
}
void QalculateWindow::onInsertFunctionRPN() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	if(!fd->keep_open) fd->dialog->hide();
	calculateRPN(fd->f);
	//if(fd->add_to_menu) function_inserted(f);
	if(fd->keep_open) {
		fd->entry[0]->setFocus();
		expressionEdit->selectAll();
	} else {
		fd->dialog->deleteLater();
		delete fd;
	}
}
void QalculateWindow::onInsertFunctionInsert() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	if(!fd->keep_open) fd->dialog->hide();
	insertFunctionDo(fd);
	if(fd->keep_open) {
		fd->entry[0]->setFocus();
		expressionEdit->selectAll();
	} else {
		fd->dialog->deleteLater();
		delete fd;
	}
}
void QalculateWindow::onInsertFunctionKeepOpen(bool b) {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	fd->keep_open = b;
	settings->keep_function_dialog_open = b;
	if(!b) fd->w_scrollresult->hide();
}
void QalculateWindow::onInsertFunctionClosed() {
	FunctionDialog *fd = (FunctionDialog*) sender()->property("QALCULATE FD").value<void*>();
	fd->dialog->deleteLater();
	delete fd;
}
void QalculateWindow::executeFromFile(const QString &file) {
	QFile qfile(file);
	if(!qfile.open(QIODevice::ReadOnly)) {
		qDebug() << tr("Failed to open %1.\n%2").arg(file).arg(qfile.errorString());
		return;
	}
	char buffer[10000];
	std::string str, scom;
	size_t ispace;
	bool rpn_save = settings->rpn_mode;
	settings->rpn_mode = false;
	previous_expression = "";
	expressionEdit->blockUndo(true);
	expressionEdit->blockCompletion(true);
	expressionEdit->blockParseStatus(true);
	block_expression_history = true;
	while(qfile.readLine(buffer, 10000)) {
		str = buffer;
		remove_blank_ends(str);
		ispace = str.find_first_of(SPACES);
		if(ispace == std::string::npos) scom = "";
		else scom = str.substr(0, ispace);
		if(equalsIgnoreCase(str, "exrates") || equalsIgnoreCase(str, "stack") || equalsIgnoreCase(str, "swap") || equalsIgnoreCase(str, "rotate") || equalsIgnoreCase(str, "copy") || equalsIgnoreCase(str, "clear stack") || equalsIgnoreCase(str, "exact") || equalsIgnoreCase(str, "approximate") || equalsIgnoreCase(str, "approx") || equalsIgnoreCase(str, "factor") || equalsIgnoreCase(str, "partial fraction") || equalsIgnoreCase(str, "simplify") || equalsIgnoreCase(str, "expand") || equalsIgnoreCase(str, "mode") || equalsIgnoreCase(str, "help") || equalsIgnoreCase(str, "?") || equalsIgnoreCase(str, "list") || equalsIgnoreCase(str, "exit") || equalsIgnoreCase(str, "quit") || equalsIgnoreCase(str, "clear") || equalsIgnoreCase(str, "clear history") || equalsIgnoreCase(scom, "variable") || equalsIgnoreCase(scom, "function") || equalsIgnoreCase(scom, "set") || equalsIgnoreCase(scom, "save") || equalsIgnoreCase(scom, "store") || equalsIgnoreCase(scom, "swap") || equalsIgnoreCase(scom, "delete") || equalsIgnoreCase(scom, "keep") || equalsIgnoreCase(scom, "assume") || equalsIgnoreCase(scom, "base") || equalsIgnoreCase(scom, "rpn") || equalsIgnoreCase(scom, "move") || equalsIgnoreCase(scom, "rotate") || equalsIgnoreCase(scom, "copy") || equalsIgnoreCase(scom, "pop") || equalsIgnoreCase(scom, "convert") || (equalsIgnoreCase(scom, "to") && scom != "to") || equalsIgnoreCase(scom, "list") || equalsIgnoreCase(scom, "find") || equalsIgnoreCase(scom, "info") || equalsIgnoreCase(scom, "help")) str.insert(0, 1, '/');
		if(!str.empty()) calculateExpression(true, false, OPERATION_ADD, NULL, false, 0, "", str, false);
	}
	expressionEdit->clear();
	expressionEdit->setExpressionHasChanged(true);
	if(parsed_mstruct) parsed_mstruct->clear();
	if(parsed_tostruct) parsed_tostruct->setUndefined();
	matrix_mstruct.clear();
	expressionEdit->blockUndo(false);
	expressionEdit->blockCompletion(false);
	expressionEdit->blockParseStatus(false);
	block_expression_history = false;
	settings->rpn_mode = rpn_save;
	previous_expression = "";
	if(mstruct) {
		if(settings->rpn_mode) {
			mstruct->unref();
			mstruct = CALCULATOR->getRPNRegister(1);
			if(!mstruct) mstruct = new MathStructure();
			else mstruct->ref();
		} else {
			mstruct->clear();
		}
	}
	qfile.close();
}
void QalculateWindow::convertToUnit(Unit *u) {
	executeCommand(COMMAND_CONVERT_UNIT, true, "", u);
}
void QalculateWindow::normalModeActivated() {
	settings->rpn_mode = false;
	settings->chain_mode = false;
	rpnDock->hide();
	CALCULATOR->clearRPNStack();
	rpnView->clear();
	rpnView->setRowCount(0);
}
void QalculateWindow::onRPNVisibilityChanged(bool b) {
	if(settings->rpn_mode != b) {
		if(b) {
			settings->rpn_mode = true;
			settings->chain_mode = false;
			if(!settings->rpn_shown) {
				rpnDock->blockSignals(true);
				rpnDock->hide();
				rpnDock->setFloating(true);
				settings->rpn_shown = true;
				rpnDock->resize(rpnDock->sizeHint());
				rpnDock->show();
				rpnDock->blockSignals(false);
			}
			QAction *w = findChild<QAction*>("action_rpnmode");
			if(w) w->setChecked(true);
			toAction_t->setEnabled(false);
		}
	}
}
 void QalculateWindow::onRPNClosed() {
	normalModeActivated();
	QAction *w = findChild<QAction*>("action_normalmode");
	if(w) w->setChecked(true);
}
void QalculateWindow::rpnModeActivated() {
	if(settings->rpn_mode) {
		normalModeActivated();
		QAction *w = findChild<QAction*>("action_normalmode");
		if(w) w->setChecked(true);
	} else {
		settings->rpn_mode = true;
		settings->chain_mode = false;
		if(!settings->rpn_shown) {rpnDock->setFloating(true); settings->rpn_shown = true;}
		rpnDock->show();
		rpnDock->raise();
		toAction_t->setEnabled(false);
	}
}
void QalculateWindow::chainModeActivated() {
	if(settings->chain_mode) {
		normalModeActivated();
		QAction *w = findChild<QAction*>("action_normalmode");
		if(w) w->setChecked(true);
	} else {
		settings->rpn_mode = false;
		settings->chain_mode = true;
		rpnDock->hide();
		CALCULATOR->clearRPNStack();
		rpnView->clear();
		rpnView->setRowCount(0);
	}
}
void QalculateWindow::checkVersion() {
	settings->checkVersion(true, this);
}
void QalculateWindow::reportBug() {
	QDesktopServices::openUrl(QUrl("https://github.com/Qalculate/qalculate-qt/issues"));
}
void QalculateWindow::help() {
	QDesktopServices::openUrl(QUrl("https://qalculate.github.io/manual/index.html"));
}
void QalculateWindow::loadInitialHistory() {
	historyView->loadInitial();
}

void QalculateWindow::loadWorkspace(const QString &filename) {
	bool rpn_mode_prev = settings->rpn_mode;
	bool chain_mode_prev = settings->chain_mode;
	if(settings->loadWorkspace(filename.toLocal8Bit().data())) {
		settings->preferences_version[0] = 4;
		settings->preferences_version[1] = 1;
		mstruct->unref();
		mstruct = new MathStructure();
		mstruct_exact.setUndefined();
		parsed_mstruct->clear();
		expressionEdit->clear();
		historyView->loadInitial();
		if(expressionEdit->completionInitialized()) expressionEdit->updateCompletion();
		if(functionsDialog) functionsDialog->updateFunctions();
		if(unitsDialog) unitsDialog->updateUnits();
		if(variablesDialog) variablesDialog->updateVariables();
		if(datasetsDialog) datasetsDialog->updateDatasets();
		functionsMenu->clear();
		unitsMenu->clear();
		variablesMenu->clear();
		keypad->updateBase();
		keypad->updateSymbols();
		if(settings->show_keypad >= 0 && settings->show_keypad != keypadDock->isVisible()) keypadAction->trigger();
		onBasesActivated(settings->show_bases > 0);
		QAction *action = find_child_data(this, "group_keypad", settings->show_keypad == 0 ? -1 : settings->keypad_type);
		if(action) action->setChecked(true);
		showNumpadAction->setChecked(!settings->hide_numpad);
		keypad->setKeypadType(settings->keypad_type);
		updateKeypadTitle();
		keypad->hideNumpad(settings->hide_numpad);
		nKeypadAction->setEnabled(settings->hide_numpad);
		if(preferencesDialog) {
			preferencesDialog->hide();
			preferencesDialog->deleteLater();
			preferencesDialog = NULL;
		}
		if(settings->rpn_mode != rpn_mode_prev || settings->chain_mode != chain_mode_prev) {
			if(settings->rpn_mode) action = findChild<QAction*>("action_rpnmode");
			else if(settings->chain_mode) action = findChild<QAction*>("action_chainmode");
			else action = findChild<QAction*>("action_normalmode");
			if(action) action->setChecked(true);
			if(settings->rpn_mode) rpnModeActivated();
			else if(settings->chain_mode) chainModeActivated();
			else normalModeActivated();
		}
		action = find_child_data(this, "group_outbase", settings->printops.base);
		if(!action) action = customOutputBaseAction;
		if(action) action->setChecked(true);
		if(action == customOutputBaseAction) customOutputBaseEdit->setValue(settings->printops.base == BASE_CUSTOM ? CALCULATOR->customOutputBase().intValue() : settings->printops.base);
		action = find_child_data(this, "group_inbase", settings->evalops.parse_options.base);
		if(!action) action = customInputBaseAction;
		if(action) action->setChecked(true);
		if(action == customInputBaseAction) customInputBaseEdit->setValue(settings->evalops.parse_options.base == BASE_CUSTOM ? CALCULATOR->customInputBase().intValue() : settings->evalops.parse_options.base);
		action = find_child_data(this, "group_general", settings->printops.min_exp);
		if(action) action->setChecked(true);
		QSpinBox *w = findChild<QSpinBox*>("spinbox_precision");
		if(w) {
			w->blockSignals(true);
			w->setValue(CALCULATOR->getPrecision());
			w->blockSignals(false);
		}
		settings->setCustomAngleUnit();
		action = find_child_data(this, "group_type", CALCULATOR->defaultAssumptions()->type());
		if(action) action->setChecked(true);
		action = find_child_data(this, "group_sign", CALCULATOR->defaultAssumptions()->sign());
		if(action) action->setChecked(true);
		if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM && !CALCULATOR->customAngleUnit()) settings->evalops.parse_options.angle_unit = ANGLE_UNIT_NONE;
		if(settings->evalops.parse_options.angle_unit == ANGLE_UNIT_CUSTOM) {
			action = findChild<QAction*>("action_angle_unit" + QString::fromStdString(CALCULATOR->customAngleUnit()->referenceName()));
		} else {
			action = find_child_data(this, "group_angleunit", settings->evalops.parse_options.angle_unit);
		}
		if(action) action->setChecked(true);
		action = NULL;
		if(settings->dual_approximation < 0) action = findChild<QAction*>("action_autoappr");
		else if(settings->dual_approximation > 0) action = findChild<QAction*>("action_dualappr");
		else if(settings->evalops.approximation == APPROXIMATION_EXACT) action = findChild<QAction*>("action_exact");
		else if(settings->evalops.approximation == APPROXIMATION_TRY_EXACT) action = findChild<QAction*>("action_approximate");
		else if(settings->evalops.approximation == APPROXIMATION_APPROXIMATE) action = findChild<QAction*>("action_approximate");
		if(action) {
			action->setChecked(true);
		}
		w = findChild<QSpinBox*>("spinbox_maxdecimals");
		if(w) {
			w->blockSignals(true);
			w->setValue(!settings->printops.use_max_decimals || settings->printops.max_decimals < 0 ? -1 : settings->printops.max_decimals);
			w->blockSignals(false);
		}
		w = findChild<QSpinBox*>("spinbox_mindecimals");
		if(w) {
			w->blockSignals(true);
			w->setValue(!settings->printops.use_min_decimals || settings->printops.min_decimals < 0 ? 0 : settings->printops.min_decimals);
			w->blockSignals(false);
		}
		updateWSActions();
		updateWindowTitle();
		workspace_changed = false;
	} else {
		QMessageBox::critical(this, tr("Error"), tr("Failed to open workspace"), QMessageBox::Ok);
	}
}
void QalculateWindow::saveWorkspaceAs() {
	while(true) {
		QString str = QFileDialog::getSaveFileName(this);
		if(str.isEmpty()) break;
		settings->show_bases = basesDock->isVisible();
		if(settings->saveWorkspace(str.toLocal8Bit().data())) {
			workspace_changed = false;
			updateWSActions();
			break;
		} else {
			QMessageBox::critical(this, tr("Error"), tr("Couldn't save workspace"), QMessageBox::Ok);
		}
	}
}
void QalculateWindow::saveWorkspace() {
	settings->show_bases = basesDock->isVisible();
	if(settings->saveWorkspace(settings->current_workspace.c_str())) {
		workspace_changed = false;
		updateWSActions();
	} else {
		QMessageBox::critical(this, tr("Error"), tr("Couldn't save workspace"), QMessageBox::Ok);
	}
}
int QalculateWindow::askSaveWorkspace() {
	if(!workspace_changed) return true;
	bool b_noask = settings->save_workspace >= 0;
	int b = 0;
	if(b_noask) {
		if(settings->save_workspace > 0) b = QMessageBox::Yes;
		else b = QMessageBox::No;
	} else {
		QMessageBox *dialog = new QMessageBox(QMessageBox::Question, tr("Save file?"), tr("Do you want to save the current workspace?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
		dialog->setCheckBox(new QCheckBox(tr("Do not ask again")));
		b = dialog->exec();
		b_noask = dialog->checkBox()->isChecked();
		dialog->deleteLater();
	}
	if(b == QMessageBox::Yes) {
		settings->show_bases = basesDock->isVisible();
		while(!settings->saveWorkspace(settings->current_workspace.c_str())) {
			int answer = QMessageBox::critical(this, tr("Error"), tr("Couldn't save workspace"), QMessageBox::Retry | QMessageBox::Ignore | QMessageBox::Cancel);
			if(answer == QMessageBox::Ignore) return 0;
			else if(answer == QMessageBox::Cancel) return -1;
		}
		workspace_changed = false;
		if(b_noask) settings->save_workspace = 1;
		return 1;
	} else if(b == QMessageBox::Cancel) {
		return -1;
	}
	if(b_noask) settings->save_workspace = 0;
	return 0;
}
void QalculateWindow::openRecentWorkspace() {
	settings->show_bases = basesDock->isVisible();
	if(settings->current_workspace.empty()) {
		settings->window_state = saveState();
		if(height() != DEFAULT_HEIGHT || width() != DEFAULT_WIDTH) settings->window_geometry = saveGeometry();
		else settings->window_geometry = QByteArray();
		settings->splitter_state = ehSplitter->saveState();
		if(settings->savePreferences() < 0) return;
	} else {
		if(askSaveWorkspace() < 0) return;
	}
	loadWorkspace(qobject_cast<QAction*>(sender())->data().toString());
	updateWSActions();
}
void QalculateWindow::openWorkspace() {
	settings->show_bases = basesDock->isVisible();
	if(settings->current_workspace.empty()) {
		settings->window_state = saveState();
		if(height() != DEFAULT_HEIGHT || width() != DEFAULT_WIDTH) settings->window_geometry = saveGeometry();
		else settings->window_geometry = QByteArray();
		settings->splitter_state = ehSplitter->saveState();
		if(settings->savePreferences() < 0) return;
	} else {
		if(askSaveWorkspace() < 0) return;
	}
	QString str = QFileDialog::getOpenFileName(this);
	if(!str.isEmpty()) {
		loadWorkspace(str);
	}
	updateWSActions();
}
void QalculateWindow::openDefaultWorkspace() {
	if(askSaveWorkspace() < 0) return;
	loadWorkspace(QString());
	workspace_changed = false;
	updateWSActions();
}
void QalculateWindow::updateWSActions() {
	saveWSAction->setEnabled(!settings->current_workspace.empty());
	defaultWSAction->setEnabled(!settings->current_workspace.empty());
	if(settings->recent_workspaces.empty() && recentWSSeparator) {
		recentWSMenu->removeAction(recentWSSeparator);
		recentWSSeparator = NULL;
	}
	for(int i = 0; i < recentWSAction.count(); i++) {
		recentWSMenu->removeAction(recentWSAction.at(i));
	}
	recentWSAction.clear();
	if(!settings->recent_workspaces.empty() && !recentWSSeparator) {
		recentWSSeparator = recentWSMenu->addSeparator();
	}
	for(size_t i = settings->recent_workspaces.size(); i > 0; i--) {
		std::string str = (settings->recent_workspaces[i - 1]);
#ifdef _WIN32
		size_t index = str.rfind('\\');
#else
		size_t index = str.rfind('/');
#endif
		if(index != std::string::npos) str = str.substr(index + 1);
		recentWSAction << recentWSMenu->addAction(QString::fromStdString(str), this, SLOT(openRecentWorkspace()));
		recentWSAction.last()->setData(QString::fromStdString(settings->recent_workspaces[i - 1]));
	}
}

