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
#include <QDebug>
#include <QTranslator>
#include <QDir>
#include <QTextStream>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <locale.h>

#include "libqalculate/qalculate.h"
#include "qalculatewindow.h"
#include "qalculateqtsettings.h"

QalculateQtSettings *settings;

QTranslator translator, translator_qt, translator_qtbase;

int main(int argc, char **argv) {

	QApplication app(argc, argv);
	app.setApplicationName("qalculate-qt");
	app.setApplicationDisplayName("Qalculate!");
	app.setOrganizationName("Qalculate");
	app.setApplicationVersion(VERSION);

	QCommandLineParser *parser = new QCommandLineParser();
	parser->addPositionalArgument("expression", QApplication::tr("Expression to calculate"), "[expression]");
	parser->addHelpOption();
	parser->process(app);

	QString lockpath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	QDir lockdir(lockpath);
	QLockFile lockFile(lockpath + "/qalculate-qt.lock");
	if(lockdir.mkpath(lockpath)) {
		if(!lockFile.tryLock(100)){
			if(lockFile.error() == QLockFile::LockFailedError) {
				QTextStream outStream(stdout);
				outStream << QApplication::tr("%1 is already running.").arg(app.applicationDisplayName()) << '\n';
				QLocalSocket socket;
				socket.connectToServer("qalculate-qt");
				if(socket.waitForConnected()) {
					QString command;
					QStringList args = parser->positionalArguments();
					for(int i = 0; i < args.count(); i++) {
						if(i > 0) command += " ";
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

#ifndef LOAD_EQZICONS_FROM_FILE
	if(QIcon::themeName().isEmpty() || !QIcon::hasThemeIcon("qalculate")) {
		QIcon::setThemeSearchPaths(QStringList(ICON_DIR));
		QIcon::setThemeName("QALCULATE");
	}
#endif
	app.setWindowIcon(LOAD_APP_ICON("qalculate"));

	settings = new QalculateQtSettings();

	new Calculator(settings->ignore_locale);

	CALCULATOR->setExchangeRatesWarningEnabled(false);

	settings->loadPreferences();
	bool canfetch = CALCULATOR->canFetch();

	QalculateWindow *win = new QalculateWindow();
	win->setCommandLineParser(parser);
	win->show();

	app.processEvents();

	if(settings->fetch_exchange_rates_at_startup && canfetch) {
		settings->fetchExchangeRates(5, -1, win);
		app.processEvents();
	}
	CALCULATOR->loadExchangeRates();

	if(!CALCULATOR->loadGlobalDefinitions()) {
		QTextStream outStream(stderr);
		outStream << QApplication::tr("Failed to load global definitions!\n");
		return 1;
	}

	CALCULATOR->loadLocalDefinitions();

	QStringList args = parser->positionalArguments();
	QString expression;
	for(int i = 0; i < args.count(); i++) {
		if(i > 0) expression += " ";
		expression += args.at(i);
	}

	args.clear();

	return app.exec();

}
