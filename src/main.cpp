/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <QCommandLineParser>
#include <QApplication>
#include <QObject>
#include <QSettings>
#include <QLockFile>
#include <QStandardPaths>
#include <QtGlobal>
#include <QLocalSocket>
#include <QTranslator>
#include <QDir>
#include <QTextStream>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QDebug>

#include <libqalculate/qalculate.h>
#include "qalculatewindow.h"
#include "qalculateqtsettings.h"

QString custom_title;
QalculateQtSettings *settings;
extern bool title_modified;

QTranslator translator, translator_qt, translator_qtbase;

int main(int argc, char **argv) {

	QApplication app(argc, argv);
	app.setApplicationName("qalculate-qt");
	app.setApplicationDisplayName("Qalculate!");
	app.setOrganizationName("qalculate");
	app.setApplicationVersion(VERSION);

	settings = new QalculateQtSettings();

	QalculateTranslator eqtr;
	app.installTranslator(&eqtr);
	if(!settings->ignore_locale) {
#ifndef TRANSLATIONS_DIR
#	define TRANSLATIONS_DIR ":/translations"
#endif
		if(translator.load(QLocale(), QLatin1String("qalculate-qt"), QLatin1String("_"), QLatin1String(TRANSLATIONS_DIR))) app.installTranslator(&translator);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
		if(translator_qt.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
#else
		if(translator_qt.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qt);
		if(translator_qtbase.load(QLocale(), QLatin1String("qtbase"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) app.installTranslator(&translator_qtbase);
#endif
	}

	QCommandLineParser *parser = new QCommandLineParser();
	QCommandLineOption fOption(QStringList() << "f" << "file", QApplication::tr("Execute expressions and commands from a file"), QApplication::tr("FILE"));
	parser->addOption(fOption);
	QCommandLineOption nOption(QStringList() << "n" << "new-instance", QApplication::tr("Start a new instance of the application"));
	parser->addOption(nOption);
	QCommandLineOption tOption(QStringList() << "title", QApplication::tr("Specify the window title"), QApplication::tr("TITLE"));
	parser->addOption(tOption);
	QCommandLineOption vOption(QStringList() << "v" << "verison", QApplication::tr("Display the application version"));
	parser->addOption(vOption);
	parser->addPositionalArgument("expression", QApplication::tr("Expression to calculate"), QApplication::tr("[EXPRESSION]"));
	parser->addHelpOption();
	parser->process(app);

	if(parser->isSet(vOption)) {
		printf(VERSION "\n");
		return 0;
	}

	std::string homedir = getLocalDir();
	recursiveMakeDir(homedir);
	QLockFile lockFile(QString::fromStdString(buildPath(homedir, "qalculate-qt.lock")));
	if(settings->allow_multiple_instances < 1 && !parser->isSet(nOption) && !lockFile.tryLock(100)) {
		if(lockFile.error() == QLockFile::LockFailedError) {
			bool ami_changed = false;
			if(settings->allow_multiple_instances < 0 && parser->value(fOption).isEmpty() && parser->positionalArguments().isEmpty()) {
				settings->allow_multiple_instances = (QMessageBox::question(NULL, QString("Allow multiple instances?"), QApplication::tr("By default, only one instance (one main window) of %1 is allowed.\n\nIf multiple instances are opened simultaneously, only the definitions (variables, functions, etc.), mode, preferences, and history of the last closed window will be saved.\n\nDo you, despite this, want to change the default behavior and allow multiple simultaneous instances?").arg("Qalculate!"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes);
				ami_changed = true;
				if(settings->allow_multiple_instances) {
					QLocalSocket socket;
					socket.connectToServer("qalculate-qt");
					if(socket.waitForConnected()) {
						socket.write("+");
						socket.waitForBytesWritten(3000);
						socket.disconnectFromServer();
					}
				}
			}
			if(settings->allow_multiple_instances <= 0) {
				QTextStream outStream(stdout);
				outStream << QApplication::tr("%1 is already running.").arg(app.applicationDisplayName()) << '\n';
				QLocalSocket socket;
				socket.connectToServer("qalculate-qt");
				if(socket.waitForConnected()) {
					QString command;
					if(!parser->value(fOption).isEmpty()) {
						command = "f";
						command += parser->value(fOption);
					} else if(ami_changed) {
						command = "-";
					} else {
						command = "0";
					}
					QStringList args = parser->positionalArguments();
					for(int i = 0; i < args.count(); i++) {
						if(i > 0) command += " ";
						else if(command == "f") command += ";";
						command += args.at(i);
					}
					socket.write(command.toUtf8());
					socket.waitForBytesWritten(3000);
					socket.disconnectFromServer();
				}
				return 1;
			}
		}
	}

	app.setWindowIcon(LOAD_APP_ICON("qalculate-qt"));

	new Calculator(settings->ignore_locale);

	CALCULATOR->setExchangeRatesWarningEnabled(false);

	settings->loadPreferences();

	CALCULATOR->loadExchangeRates();

	if(!CALCULATOR->loadGlobalDefinitions()) {
		QTextStream outStream(stderr);
		outStream << QApplication::tr("Failed to load global definitions!\n");
		return 1;
	}

	if(settings->do_imaginary_j && CALCULATOR->getVariableById(VARIABLE_ID_I)->hasName("j") == 0) {
		Variable *v_i = CALCULATOR->getVariableById(VARIABLE_ID_I);
		ExpressionName ename = v_i->getName(1);
		ename.name = "j";
		ename.reference = false;
		v_i->addName(ename, 1, true);
		v_i->setChanged(false);
	}

	settings->f_answer->setCategory(CALCULATOR->getFunctionById(FUNCTION_ID_WARNING)->category());
	settings->f_answer->setChanged(false);

	CALCULATOR->loadLocalDefinitions();

	settings->updateFavourites();

	QalculateWindow *win = new QalculateWindow();
	if(parser->value(tOption).isEmpty()) {
		win->updateWindowTitle();
		title_modified = false;
	} else {
		win->setWindowTitle(parser->value(tOption));
		title_modified = true;
	}
	win->setCommandLineParser(parser);
	win->show();

	if(!parser->value(fOption).isEmpty()) win->executeFromFile(parser->value(fOption));

	QStringList args = parser->positionalArguments();
	QString expression;
	for(int i = 0; i < args.count(); i++) {
		if(i > 0) expression += " ";
		expression += args.at(i);
	}
	expression = expression.trimmed();
	if(!expression.isEmpty()) win->calculate(expression);
	args.clear();

	settings->checkVersion(false, win);

	QColor c = QApplication::palette().base().color();
	if(c.red() + c.green() + c.blue() < 255) settings->color = 2;
	else settings->color = 1;
	win->loadInitialHistory();

	return app.exec();

}

QalculateTranslator::QalculateTranslator() : QTranslator() {}
QString	QalculateTranslator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const {
	if(!translator_qt.translate(context, sourceText, disambiguation, n).isEmpty() || !translator_qtbase.translate(context, sourceText, disambiguation, n).isEmpty()) return QString();
	if(strcmp(context, "QalculateTranslator") == 0) return QString();
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "OK") == 0) return tr("OK");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "Cancel") == 0) return tr("Cancel");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "Close") == 0) return tr("Close");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "&Yes") == 0) return tr("&Yes");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "&No") == 0) return tr("&No");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "&Open") == 0) return tr("&Open");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "&Save") == 0) return tr("&Save");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "&Select All") == 0) return tr("&Select All");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "Look in:") == 0) return tr("Look in:");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "File &name:") == 0) return tr("File &name:");
	//: Only used when Qt translation is missing
	if(strcmp(sourceText, "Files of type:") == 0) return tr("Files of type:");
	return QString();
}

