/****************************************************************************
 *  QFakeMail - A simple fakemailer Qt based                                *
 *  Copyright (C) 2005-2007  Matteo Croce <rootkit85@yahoo.it>              *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#ifndef QFAKEMAIL_H
#define QFAKEMAIL_H

#include <QMainWindow>
#include <QTemporaryFile>
#include <QProgressDialog>
#include <QtNetwork/QTcpSocket>

#include "ui_qfakemailwidget.h"

class QFakeMail: public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:
	QFakeMail();
	~QFakeMail();

private slots:
	void change();
	void filesSelected();
	void removeFile();
	void addFile();
	void sendSlot();
	void readed();
	void connected();
	void disconnected();
	void about();
	void error(const QString&);
	void reEnableAll();
	void gotErrorSlot();

private:
	QList<QTemporaryFile*> encoded;
	QProgressDialog *pd;
	QTcpSocket sock;
	enum {HELO, MAIL_FROM, RCPT_TO, DATA, BODY, QUIT, DISCONNECT} state;
	inline int base64_encode(const char*, char*, int);
	inline void encodeblock(const char[3], char[4]);
	inline void encodelastblock(const char[3], char[4], int);
};

#endif
