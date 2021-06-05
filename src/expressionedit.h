/*
    Qalculate (QT UI)

    Copyright (C) 2021  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPRESSION_EDIT_H
#define EXPRESSION_EDIT_H

#include <QTextEdit>
#include <QSortFilterProxyModel>
#include <QStringList>

class QCompleter;
class QStandardItemModel;
class QTableView;

class ExpressionProxyModel : public QSortFilterProxyModel {

	Q_OBJECT

	public:

		ExpressionProxyModel(QObject *parent = NULL);
		~ExpressionProxyModel();

		void setFilter(std::string str);

	protected:

		std::string s_filter;

		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

};

class ExpressionEdit : public QTextEdit {

	Q_OBJECT

	protected:

		QCompleter *completer;
		ExpressionProxyModel *completionModel;
		QStandardItemModel *sourceModel;
		QTableView *completionView;

		QStringList history;
		QString current_history;
		int history_index;
		int current_object_start, current_object_end;
		std::string current_object_text;
		int completion_blocked;
		bool editing_to_expression, editing_to_expression1;
		bool disable_history_arrow_keys, dont_change_index, cursor_has_moved;

	public:

		ExpressionEdit(QWidget *parent = NULL);
		virtual ~ExpressionEdit();

		std::string expression() const;
		QSize sizeHint() const;

		void updateCompletion();

	protected slots:

		void keyPressEvent(QKeyEvent*) override;
		void keyReleaseEvent(QKeyEvent*) override;
		void onTextChanged();
		void onCursorPositionChanged();
		void onCompletionActivated(const QModelIndex&);

	public slots:

		void setExpression(std::string);
		void blockCompletion();
		void unblockCompletion();
		void hideCompletion();
		void addToHistory();

	signals:

		void returnPressed();

};

#endif //EXPRESSION_EDIT_H
