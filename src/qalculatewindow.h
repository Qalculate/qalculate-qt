/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef QALCULATE_WINDOW_H
#define QALCULATE_WINDOW_H

#include <QMainWindow>
#include <libqalculate/qalculate.h>

class QLocalSocket;
class QLocalServer;
class QCommandLineParser;
class ExpressionEdit;
class HistoryView;
class QSplitter;

#ifdef LOAD_EQZICONS_FROM_FILE
	#ifdef RESOURCES_COMPILED
		#define LOAD_APP_ICON(x) QIcon(ICON_DIR "/EQZ/apps/64x64/" x ".png")
		#define LOAD_ICON(x) QIcon(ICON_DIR "/EQZ/actions/64x64/" x ".png")
		#define LOAD_ICON_APP(x) QIcon(ICON_DIR "/EQZ/apps/64x64/" x ".png")
		#define LOAD_ICON_STATUS(x) QIcon(ICON_DIR "/EQZ/status/64x64/" x ".png")
		#define LOAD_ICON2(x, y) QIcon(ICON_DIR "/EQZ/actions/64x64/" y ".png")
	#else
		#define LOAD_APP_ICON(x) QIcon(ICON_DIR "/hicolor/64x64/apps/" x ".png")
		#define LOAD_ICON(x) (QString(x).startsWith("eqz") ? QIcon(ICON_DIR "/hicolor/64x64/actions/" x ".png") : QIcon::fromTheme(x))
		#define LOAD_ICON_APP(x) QIcon::fromTheme(x)
		#define LOAD_ICON_STATUS(x) QIcon::fromTheme(x)
		#define LOAD_ICON2(x, y) (QString(x).startsWith("eqz") ? QIcon(ICON_DIR "/hicolor/64x64/actions/" x ".png") : QIcon::fromTheme(x, QIcon::fromTheme(y)))
	#endif
#else
	#define LOAD_APP_ICON(x) QIcon::fromTheme(x)
	#define LOAD_ICON(x) QIcon::fromTheme(x)
	#define LOAD_ICON_APP(x) QIcon::fromTheme(x)
	#define LOAD_ICON_STATUS(x) QIcon::fromTheme(x)
	#define LOAD_ICON2(x, y) QIcon::fromTheme(x, QIcon::fromTheme(y))
#endif

class QalculateWindow : public QMainWindow {

	Q_OBJECT

	public:

		QalculateWindow();
		virtual ~QalculateWindow();

		void setCommandLineParser(QCommandLineParser*);

	protected:

		QLocalSocket *socket;
		QLocalServer *server;
		QCommandLineParser *parser;

		ExpressionEdit *expressionEdit;
		HistoryView *historyView;
		QSplitter *ehSplitter;

		void calculateExpression(bool do_mathoperation = false, MathOperation op = OPERATION_ADD, MathFunction *f = NULL, bool do_stack = false, size_t stack_index = 0, bool check_exrates = true);
		void setResult(Prefix *prefix = NULL, bool update_parse = false, size_t stack_index = 0, bool register_moved = false, bool noprint = false);

	public slots:

		void serverNewConnection();
		void socketReadyRead();
		void calculate();

	signals:

};

#endif //QALCULATE_WINDOW_H
