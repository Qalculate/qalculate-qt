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

#include <libqalculate/qalculate.h>

class QCompleter;
class QStandardItemModel;
class QTableView;
struct CompletionData;

class ExpressionProxyModel : public QSortFilterProxyModel {

	Q_OBJECT

	public:

		ExpressionProxyModel(CompletionData*, QObject *parent = NULL);
		~ExpressionProxyModel();

		void setFilter(std::string);

		CompletionData *cdata;

	protected:

		std::string str;

		bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
		bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

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
		
		CompletionData *cdata;
		
		int current_object_start, current_object_end;
		int current_function_index;
		std::string current_object_text;
		int completion_blocked;
		int block_text_change;
		bool disable_history_arrow_keys, dont_change_index, cursor_has_moved;
		bool display_expression_status;
		int block_display_parse;
		QString prev_parsed_expression, parsed_expression_tooltip;
		bool expression_has_changed2;
		bool parsed_had_errors, parsed_had_warnings;
		int previous_epos;
		bool parentheses_highlighted;

		void setCurrentObject();
		void setStatusText(QString text);
		bool displayFunctionHint(MathFunction *f, int arg_index = 1);
		void highlightParentheses();

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
		void complete();
		void displayParseStatus();

	signals:

		void returnPressed();

};

#endif //EXPRESSION_EDIT_H
