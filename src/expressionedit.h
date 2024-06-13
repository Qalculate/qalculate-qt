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

#include <QPlainTextEdit>
#include <QSortFilterProxyModel>
#include <QStringList>

#include <libqalculate/qalculate.h>

class QCompleter;
class QStandardItemModel;
class QTableView;
class QMenu;
class QAction;
class QTimer;
class ExpressionTipLabel;

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

class ExpressionEdit : public QPlainTextEdit {

	Q_OBJECT

	protected:

		QCompleter *completer;
		ExpressionProxyModel *completionModel;
		QStandardItemModel *sourceModel;
		QTableView *completionView;
		QMenu *cmenu, *fileMenu, *modeMenu;
		QAction *undoAction, *redoAction, *cutAction, *copyAction, *pasteAction, *deleteAction, *selectAllAction, *clearAction, *statusOffAction, *statusDelayAction, *statusNoDelayAction, *statusHistoryAction, *statusExpressionAction, *clearHistoryAction, *fileAction, *fileSeparator, *tbAction;
		QTimer *completionTimer, *toolTipTimer;
		ExpressionTipLabel *tipLabel;
		QWidget *tb;

		QStringList expression_undo_buffer;
		QList<int> expression_undo_pos;
		QString current_history;
		int history_index, undo_index;
		int tabbed_index;
		int previous_pos;
		QString previous_text;
		bool enable_tab;
		
		CompletionData *cdata;
		
		int current_object_start, current_object_end;
		int current_function_index;
		std::string current_object_text;
		int completion_blocked, parse_blocked, block_add_to_undo;
		int block_text_change;
		int do_completion_signal;
		bool disable_history_arrow_keys, dont_change_index, cursor_has_moved;
		int block_display_parse;
		QString prev_parsed_expression, parsed_expression_tooltip, current_status_text;
		int current_status_type;
		bool expression_has_changed, expression_has_changed2;
		bool parsed_had_errors, parsed_had_warnings;
		bool expression_from_history;
		int previous_epos;
		QPoint function_pos;
		bool parentheses_highlighted;
		int default_frame;
		QRect prev_rect;

		void setCurrentObject();
		void setStatusText(const QString &text, int = 0);
		bool displayFunctionHint(MathFunction *f, int arg_index = 1);
		void highlightParentheses();

		void keyReleaseEvent(QKeyEvent*) override;
		void contextMenuEvent(QContextMenuEvent *e) override;
		void insertFromMimeData(const QMimeData*) override;
		bool expressionInQuotes();

	public:

		ExpressionEdit(QWidget *parent = NULL, QWidget *toolbar = NULL);
		virtual ~ExpressionEdit();

		std::string expression() const;
		QSize sizeHint() const override;
		QSize minimumSizeHint() const override;

		void wrapSelection(const QString &text = QString(), bool insert_before = false, bool add_parentheses = false, bool add_comma = false, const QString &add_arg = QString(), bool quote = false);
		bool doChainMode(const QString &op);
		bool expressionHasChanged();
		void setExpressionHasChanged(bool);
		void displayParseStatus(bool = false, bool = true);
		void inputMethodEvent(QInputMethodEvent*) override;
		void keyPressEvent(QKeyEvent*) override;
		QString selectedText(bool = false);
		void onCompleterEvent(QEvent*);
		bool eventFilter(QObject*, QEvent*) override;
		void setMenuAndToolbarItems(QMenu*, QMenu*, QAction*);

		std::vector<CalculatorMessage> status_messages;

	protected slots:

		void onTextChanged();
		void onCursorPositionChanged();
		void onCompletionActivated(const QModelIndex&);
		void onCompletionHighlighted(const QModelIndex&);
		void onCompletionMenuItemActivated();
		void enableIM();
		void enableCompletionDelay();
		void onCompletionModeChanged();
		void onStatusModeChanged();
		void showCurrentStatus();

	public slots:

		void updateCompletion();
		void setExpression(std::string);
		void setExpression(const QString &str);
		void blockCompletion(bool = true, bool = true);
		void blockParseStatus(bool = true);
		void blockUndo(bool = true);
		void hideCompletion();
		void addToHistory();
		void smartParentheses();
		void insertBrackets();
		void selectAll(bool b = true);
		void editUndo();
		void editRedo();
		void editDelete();
		void insertDate();
		void insertMatrix();
		void enableTabCompletion(bool);
		bool completeOrActivateFirst(bool = false);
		bool complete(MathStructure* = NULL, MathStructure* = NULL, QMenu* = NULL, bool = false);
		void clearHistory();

	signals:

		void returnPressed();
		void toConversionRequested(std::string);
		void calculateRPNRequest(int);
		void expressionStatusModeChanged(bool);
		void statusChanged(QString, bool, bool, bool, bool);

};

#endif //EXPRESSION_EDIT_H
