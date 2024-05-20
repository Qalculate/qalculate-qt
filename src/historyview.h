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

		void addResult(std::vector<std::string> values, std::string expression = "", bool pexact = true, std::string parse = "", int exact = 1, bool dual_approx = false, const QString &image = QString(), bool *implicit_warning = NULL, int initial_load = 0, size_t index = 0, bool temporary = false);
		void clearTemporary();
		void addMessages();
		void loadInitial(bool reload = false);
		void indexAtPos(const QPoint &pos, int *expression_index, int *result_index, int *value_index = NULL, QString *anchorstr = NULL);
		void replaceColors(QString&, QColor prev_text_color = QColor());

	protected:

		QString s_text, previous_html, previous_html2, temporary_error;
		int i_pos, i_pos2, i_pos_p, i_pos_p2, previous_cursor, previous_cursor2, previous_temporary;
		int has_lock_symbol;
		QMenu *cmenu;
		QAction *insertTextAction, *insertValueAction, *copyAction, *copyFormattedAction, *copyAsciiAction, *selectAllAction, *delAction, *clearAction, *protectAction, *movetotopAction;
		QColor text_color;
		QRect prev_fonti;
		QPoint context_pos;
		QLineEdit *searchEdit;
		size_t last_ans, last_ans2;
		QString last_ref, last_ref2;
		bool initial_loaded;

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
		void reloadHistory();

	signals:

		void insertTextRequested(std::string);
		void insertValueRequested(int);
		void historyReloaded();

};

#endif //HISTORY_VIEW_H
