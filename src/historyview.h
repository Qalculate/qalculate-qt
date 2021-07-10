/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef HISTORY_VIEW_H
#define HISTORY_VIEW_H

#include <QTextBrowser>

class QImage;
class QMenu;
class QAction;
class ExpressionEdit;
class QColor;

class HistoryView : public QTextBrowser {

	Q_OBJECT

	public:

		HistoryView(QWidget *parent = NULL);
		virtual ~HistoryView();

		ExpressionEdit *expressionEdit;

		void addResult(std::vector<std::string> values, std::string expression = "", int exact = 1, bool dual_approx = false, const QString &image = QString(), bool initial_load = false, size_t index = 0);
		void addMessages();
		void loadInitial();

	protected:

		QString s_text;
		int i_pos;
		QImage *paste_image;
		QMenu *cmenu;
		QAction *copyAction, *copyFormattedAction, *selectAllAction, *clearAction;
		QColor prev_color;

		void mouseReleaseEvent(QMouseEvent *e) override;
		void contextMenuEvent(QContextMenuEvent *e) override;
		void keyPressEvent(QKeyEvent *e) override;
		void inputMethodEvent(QInputMethodEvent*) override;
		void changeEvent(QEvent*) override;

	public slots:

		void editCopy();
		void editClear();

	signals:

		void insertTextRequested(std::string);
		void insertValueRequested(int);

};

#endif //HISTORY_VIEW_H
