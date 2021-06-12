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

class HistoryView : public QTextBrowser {

	Q_OBJECT

	public:

		HistoryView(QWidget *parent = NULL);
		virtual ~HistoryView();

		QString s_text;
		int i_pos;
		QImage *paste_image;
		std::vector<std::string> v_text;

		void addResult(std::vector<std::string> values, std::string expression = "", bool exact = true, bool dual_approx = false);
		void addMessages();

	protected:
	
		void mouseReleaseEvent(QMouseEvent *e) override;

	public slots:

	signals:

		void insertTextRequested(std::string);
		void insertValueRequested(int);

};

#endif //HISTORY_VIEW_H
