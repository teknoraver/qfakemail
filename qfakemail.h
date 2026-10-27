// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2005-2007 Matteo Croce <rootkit85@yahoo.it>

#ifndef QFAKEMAIL_H
#define QFAKEMAIL_H

#include <QMainWindow>
#include <QTemporaryFile>
#include <QProgressDialog>
#include <QTcpSocket>
#include <QMimeDatabase>
#include <QStringList>
#include <QRegularExpression>

#include "ui_qfakemailwidget.h"

class QFakeMail: public QMainWindow, private Ui::MainWindow
{
Q_OBJECT

public:
	QFakeMail();
	~QFakeMail() override;

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
	QMimeDatabase mimedb;
	QProgressDialog *pd;
	QTcpSocket sock;
	QRegularExpression email;
	enum {HELO, MAIL_FROM, RCPT_TO, DATA, BODY, QUIT, DISCONNECT} state;
	bool isUtf8(QString);
	QString encode(QString);
	QStringList recents;
};

#endif
