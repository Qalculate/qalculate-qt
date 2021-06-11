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

class HistoryView : public QTextBrowser {

	Q_OBJECT

	public:

		HistoryView(QWidget *parent = NULL);
		virtual ~HistoryView();

		QString s_text;
		int i_pos;

		void addResult(std::vector<std::string> values, std::string expression = "", bool exact = true);
		void addMessages();

	public slots:

	signals:

};

#endif //HISTORY_VIEW_H
