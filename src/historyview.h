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

#include <QTextEdit>

class QImage;
class QMenu;
class QAction;
class ExpressionEdit;
class QColor;
class QLineEdit;
class QDialog;

class HistoryView : public QTextEdit {

	Q_OBJECT

	public:

		HistoryView(QWidget *parent = NULL);
		virtual ~HistoryView();

		ExpressionEdit *expressionEdit;
		QAction *findAction;
		QDialog *searchDialog;

		void addResult(std::vector<std::string> values, std::string expression = "", bool pexact = true, std::string parse = "", int exact = 1, bool dual_approx = false, const QString &image = QString(), bool *implicit_warning = NULL, bool initial_load = false, size_t index = 0);
		void addMessages();
		void loadInitial();
		void indexAtPos(const QPoint &pos, int *expression_index, int *result_index, int *value_index = NULL, QString *anchorstr = NULL);
		void replaceColors(QString&);

	protected:

		QString s_text;
		int i_pos;
		int has_lock_symbol;
		QMenu *cmenu;
		QAction *insertTextAction, *insertValueAction, *copyAction, *copyFormattedAction, *copyAsciiAction, *selectAllAction, *delAction, *clearAction, *protectAction, *movetotopAction;
		QColor prev_color;
		QPoint context_pos;
		QLineEdit *searchEdit;
		size_t last_ans;
		QString last_ref;

		void mouseDoubleClickEvent(QMouseEvent *e) override;
		void mouseReleaseEvent(QMouseEvent *e) override;
		void mouseMoveEvent(QMouseEvent *e) override;
		void contextMenuEvent(QContextMenuEvent *e) override;
		void keyPressEvent(QKeyEvent *e) override;
		void inputMethodEvent(QInputMethodEvent*) override;
		void changeEvent(QEvent*) override;

	protected slots:

		void doFind();

	public slots:

		void editInsertValue();
		void editInsertText();
		void editCopy(int = -1);
		void editCopyFormatted();
		void editCopyAscii();
		void editClear();
		void editRemove();
		void editProtect();
		void editFind();
		void editMoveToTop();

	signals:

		void insertTextRequested(std::string);
		void insertValueRequested(int);

};

#endif //HISTORY_VIEW_H
