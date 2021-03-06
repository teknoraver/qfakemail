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

#include <QApplication>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QCompleter>
#include <QSettings>

#include "qfakemail.h"

#define CHUNK_SIZE	0x10000

QFakeMail::QFakeMail() : QMainWindow(0), email("([a-z0-9._%+-]+@[a-z0-9-]+\\.[a-z0-9.-]+)", Qt::CaseInsensitive)
{
	setupUi(this);
	connect(server, SIGNAL(textEdited(const QString&)), SLOT(change()));
	connect(isfrom, SIGNAL(toggled(bool)), SLOT(change()));
	connect(from, SIGNAL(textEdited(const QString&)), SLOT(change()));
	connect(to, SIGNAL(textEdited(const QString&)), SLOT(change()));
	connect(files, SIGNAL(itemSelectionChanged()), SLOT(filesSelected()));
	connect(removefile, SIGNAL(clicked()), SLOT(removeFile()));
	connect(addfile, SIGNAL(clicked()), SLOT(addFile()));
	connect(send, SIGNAL(clicked()), SLOT(sendSlot()));
	connect(actionAbout, SIGNAL(triggered()), SLOT(about()));

	connect(&sock, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(gotErrorSlot()));
	connect(&sock, SIGNAL(disconnected()), SLOT(disconnected()));
	connect(&sock, SIGNAL(connected()), SLOT(connected()));
	connect(&sock, SIGNAL(readyRead()), SLOT(readed()));

	QSettings settings;
	int size = settings.beginReadArray("recents");
	for (int i = 0; i < size; i++) {
		settings.setArrayIndex(i);
		recents << settings.value("server").toString();
	}
	settings.endArray();
	server->setCompleter(new QCompleter(recents, server));
}

QFakeMail::~QFakeMail()
{
	qDeleteAll(encoded);
	encoded.clear();
}

void QFakeMail::about()
{
	QIcon old = windowIcon();
	setWindowIcon(QIcon(":/icons/matteo.png"));
	QMessageBox::about(this, "About QFakeMail", "QFakeMail - a fake mailer<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
	setWindowIcon(old);
}

void QFakeMail::change()
{
	send->setEnabled(
		server->text().length() &&
		(!isfrom->isChecked() || from->text().contains(email)) &&
		to->text().contains(email));
}

void QFakeMail::filesSelected()
{
	removefile->setEnabled(true);
}

void QFakeMail::removeFile()
{
	delete encoded.takeAt(files->currentRow());
	files->takeItem(files->currentRow());
	removefile->setEnabled(files->count());
}

void QFakeMail::addFile()
{
	foreach(QString path, QFileDialog::getOpenFileNames(this)) {
		QFileInfo info(path);
		if(!info.exists())
			continue;

		files->addItem(info.fileName());
		QFile in(path, this);
		QTemporaryFile *out = new QTemporaryFile(QDir::tempPath() + "/QFakeMailB64", this);
		in.open(QIODevice::ReadOnly);
		out->setAutoRemove(true);
		out->open();
		out->write(mimedb.mimeTypeForFile(path).name().toLocal8Bit() + "\r\n");
		encoded.append(out);
		QByteArray inbuf;
		inbuf = in.read(57);
		while(inbuf.size() > 0) {
			out->write(inbuf.toBase64() + "\r\n");
			inbuf = in.read(57);
		}
		out->flush();
	}
	removefile->setEnabled(files->count() && files->currentRow() > 0);
}

void QFakeMail::sendSlot()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	setEnabled(false);
	pd = new QProgressDialog("Sending", "Abort", 0, 7 + files->count(), this, Qt::Dialog);
	connect(pd, SIGNAL(canceled()), SLOT(reEnableAll()));
	qApp->processEvents();
	sock.connectToHost(server->text(), port->value(), QIODevice::ReadWrite);
}

void QFakeMail::disconnected()
{
	if(state == DISCONNECT)
		return;
	error("Connessione chiusa dal server");
}

void QFakeMail::gotErrorSlot()
{
	if(state == DISCONNECT && sock.error() == QAbstractSocket::RemoteHostClosedError)
		return;
	error(sock.errorString());
}

void QFakeMail::readed()
{
	if(!sock.canReadLine())
		return;
	QString read(sock.readLine());
	QByteArray data;
	switch(state) {
		case HELO:
			if(read.left(3).toUShort() != 220) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(2);
			data += "HELO QFakeMail\r\n";
			state = MAIL_FROM;
			break;
		case MAIL_FROM: {
			QString f = from->text();
			if(isfrom->isChecked() && email.indexIn(f) != -1) {
				QStringList list = email.capturedTexts();
				if(list.size() > 1)
					f = list[1];
			}
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(3);
			data += "MAIL FROM:<" + (isfrom->isChecked() ? f : to->text()) + ">\r\n";
			state = RCPT_TO;
			break;
		}
		case RCPT_TO: {
			QString t = to->text();
			if(email.indexIn(t) != -1) {
				QStringList list = email.capturedTexts();
				if(list.size() > 1)
					t = list[1];
			}
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(4);
			data += "RCPT TO:<" + t + ">\r\n";
			state = DATA;
			break;
		}
		case DATA:
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(5);
			data += "DATA\r\n";
			state = BODY;
			break;
		case BODY: {
			if(read.left(3).toUShort() != 354) {
				error("Bad response from server:\n" + read);
				return;
			}
			QString boundary("QFakeMail-Boundary-teknoraver");
			data += "From: " + (isfrom->isChecked() ? from->text() : "") + "\r\n"
				"To: " + to->text() + "\r\n"
				"Subject: " + encode(subject->text()) + "\r\n"
				"User-Agent: QFakeMail/1.1\r\n"
				"Content-Type: text/plain; charset=UTF-8\r\n";
			if(files->count()) {
				data += "MIME-Version: 1.0\r\n"
					"Content-Type: Multipart/Mixed;\r\n"
					"  boundary=\"" + boundary + "\"\r\n"
					"\r\n"
					"--" + boundary + "\r\n"
					"Content-Disposition: inline\r\n";
			}
			QString body = message->toPlainText();
			if(isUtf8(body)) {
				data += "Content-Transfer-Encoding: base64\r\n\r\n";
				QByteArray b64 = body.toUtf8().toBase64();
				for(int i = 0; i < b64.length(); i += 76)
					data += b64.mid(i, 76) + "\r\n";
			} else {
				data += "Content-Transfer-Encoding: 7bit\r\n\r\n";
				data += body.replace("\r\n.\r\n", "\r\n..\r\n");
			}
			data += "\r\n\r\n";
			sock.write(data);
			pd->setValue(6);
			qApp->processEvents();

			for(int i = 0; i < encoded.size(); i++) {
				encoded[i]->reset();
				data.clear();
				data += "--" + boundary + "\r\n"
					"Content-Type: " + encoded[i]->readLine().trimmed() + ";\r\n"
					"  name=\"" + files->item(i)->text() + "\"\r\n"
					"Content-Transfer-Encoding: base64\r\n"
					"Content-Disposition: attachment;\r\n"
					"  filename=\"" + files->item(i)->text() + "\"\r\n"
					"\r\n";
				sock.write(data);

				char buf[CHUNK_SIZE];
				int readed = encoded[i]->read(buf, CHUNK_SIZE);
				while(readed > 0) {
					sock.write(buf, readed);
					readed = encoded[i]->read(buf, CHUNK_SIZE);
				}

				sock.write("\r\n\r\n");
				pd->setValue(7 + i);
				qApp->processEvents();
			}

			data = QByteArray();
			if(files->count())
				data += "--" + boundary + "--\r\n";
			data += ".\r\n";
			state = QUIT;
			break;
		}
		case QUIT:
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(pd->maximum() - 1);
			data += "QUIT\r\n";
			state = DISCONNECT;
			break;
		case DISCONNECT:
			if(read.left(3).toUShort() != 221) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(pd->maximum());
			qApp->processEvents();
			sock.flush();
			sock.disconnectFromHost();
			QMessageBox::information(this, "Done", "Message has been sent");
			reEnableAll();
			return;
	}
	sock.write(data);
}

bool QFakeMail::isUtf8(QString orig)
{
	for(const QChar *data = orig.constData(); !data->isNull(); data++)
		if(data->unicode() > 127)
			return true;
	return false;
}

QString QFakeMail::encode(QString orig)
{
	if(isUtf8(orig))
		return "=?UTF-8?B?" + QString(orig.toUtf8().toBase64()) + "?=";
	return orig;
}

void QFakeMail::connected()
{
	pd->setLabelText("Sending");
	pd->setValue(1);
	state = HELO;
	if(!recents.contains(server->text(), Qt::CaseInsensitive)) {
		recents << server->text();
		if(recents.size() > 4)
			recents.removeFirst();

		QSettings settings;
		settings.beginWriteArray("recents");
		for(int i = 0; i < recents.size(); i++) {
			settings.setArrayIndex(i);
			settings.setValue("server", recents[i]);
		}
		settings.endArray();
		server->setCompleter(new QCompleter(recents, server));
	}
}

void QFakeMail::error(const QString &err)
{
	sock.disconnectFromHost();
	QMessageBox::critical(this, "Error", err);
	reEnableAll();
}

void QFakeMail::reEnableAll()
{
	pd->deleteLater();
	setEnabled(true);
	QApplication::setOverrideCursor(Qt::ArrowCursor);
}
