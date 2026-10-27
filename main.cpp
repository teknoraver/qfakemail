// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2005-2007 Matteo Croce <rootkit85@yahoo.it>

#include <QApplication>
#include "qfakemail.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setOrganizationName("teknoraver");
	a.setApplicationName("QFakeMail");
	QFakeMail qfm;
	qfm.show();
	return a.exec();
}
