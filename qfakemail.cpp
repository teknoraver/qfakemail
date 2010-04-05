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

#include "qfakemail.h"

#include <QApplication>
#include <QFileDialog>
#include <QList>
#include <QTemporaryFile>
#include <QProgressDialog>
#include <QtNetwork/QTcpSocket>
#include <QMessageBox>

#define CHUNK_SIZE	0x10000

QFakeMail::QFakeMail() : QMainWindow(0)
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
}

QFakeMail::~QFakeMail()
{
	foreach(QTemporaryFile *i, encoded)
		delete i;
	encoded.clear();
}

void QFakeMail::about()
{
	QIcon old = windowIcon();
	setWindowIcon(QIcon(":/matteo.png"));
	QMessageBox::about(this, "About QFakeMail", "QFakeMail - a fake mailer<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
	setWindowIcon(old);
}

void QFakeMail::change()
{
	send->setEnabled(
			server->text().length() &&
			(!isfrom->isChecked() || from->text().length()) &&
			to->text().length());
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
	QString path = QFileDialog::getOpenFileName(this);
	if(path.length())
		files->addItem(path.right(path.count() - path.lastIndexOf(QChar('/')) - 1));
	QFile in(path, this);
	QTemporaryFile *out = new QTemporaryFile(QDir::tempPath() + "/QFakeMailB64", this);
	in.open(QIODevice::ReadOnly);
	out->setAutoRemove(true);
	out->open();
	encoded.append(out);
	char inbuf[57], outbuf[78];
	int readed = in.read(inbuf, 57);
	while(readed > 0) {
		int written = base64_encode(inbuf, outbuf, readed);
		out->write(outbuf, written);
		readed = in.read(inbuf, 57);
	}
	out->reset();
	removefile->setEnabled(files->count() && files->currentRow() > 0);
}

void QFakeMail::sendSlot()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	setEnabled(false);
	pd = new QProgressDialog("Sending", "Abort", 0, 100, this, Qt::Dialog);
	connect(pd, SIGNAL(canceled()), SLOT(reEnableAll()));
	qApp->processEvents();
	connect(&sock, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(gotErrorSlot()));
	connect(&sock, SIGNAL(disconnected()), SLOT(disconnected()));
	connect(&sock, SIGNAL(connected()), SLOT(connected()));
	connect(&sock, SIGNAL(readyRead()), SLOT(readed()));
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
			pd->setValue(5);
			data += "HELO QFakeMail\r\n";
			state = MAIL_FROM;
			break;
		case MAIL_FROM:
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(9);
			data += "MAIL FROM:<" + (isfrom->isChecked() ? from->text() : to->text()) + ">\r\n";
			state = RCPT_TO;
			break;
		case RCPT_TO:
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(13);
			data += "RCPT TO:<" + to->text() + ">\r\n";
			state = DATA;
			break;
		case DATA:
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(17);
			data += "DATA\r\n";
			state = BODY;
			break;
		case BODY: {
			if(read.left(3).toUShort() != 354) {
				error("Bad response from server:\n" + read);
				return;
			}
			QString boundary("QFakeMail-Boundary-IL0v3T3chn0");
			data += "From: " + (isfrom->isChecked() ? from->text() : "") + "\r\n"
				"To: " + to->text() + "\r\n"
				"Subject: " + subject->text() + "\r\n"
				"User-Agent: QFakeMail/1.0\r\n";
			if(files->count()) {
				data += "MIME-Version: 1.0\r\n"
					"Content-Type: Multipart/Mixed;\r\n"
					"  boundary=\"" + boundary + "\"\r\n"
					"\r\n"
					"--" + boundary + "\r\n"
					"Content-Type: text/plain;\r\n"
					"  charset=\"us-ascii\"\r\n"
					"Content-Transfer-Encoding: 7bit\r\n"
					"Content-Disposition: inline\r\n";
			}
			data += "\r\n" +
				message->toPlainText().replace('\n', "\r\n.").replace("\r\n.", "\r\n..") + "\r\n"
				"\r\n";
			sock.write(data);
			pd->setValue(20);
			qApp->processEvents();

			for(int i = 0; i < encoded.size(); i++) {
				data = QByteArray();
				data += "--" + boundary + "\r\n"
					"Content-Type: application/octet-stream;\r\n"
					"  name=\"" + files->item(i)->text() + "\"\r\n"
					"Content-Transfer-Encoding: base64\r\n"
					"Content-Disposition: attachment;\r\n"
					"\tfilename=\"" + files->item(i)->text() + "\"\r\n"
					"\r\n";
				sock.write(data);

				char buf[CHUNK_SIZE];
				int readed = encoded[i]->read(buf, CHUNK_SIZE);
				while(readed > 0) {
					sock.write(buf, readed);
					readed = encoded[i]->read(buf, CHUNK_SIZE);
				}

				sock.write("\r\n\r\n");
				pd->setValue(60 / files->count() * i + 30);
				qApp->processEvents();
			}

			data = QByteArray();
			if(files->count())
				data += "--" + boundary + "\r\n";
			data += ".\r\n";
			state = QUIT;
			break;
		}
		case QUIT:
			if(read.left(3).toUShort() != 250) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(99);
			data += "QUIT\r\n";
			state = DISCONNECT;
			break;
		case DISCONNECT:
			if(read.left(3).toUShort() != 221) {
				error("Bad response from server:\n" + read);
				return;
			}
			pd->setValue(100);
			qApp->processEvents();
			sock.flush();
			sock.disconnectFromHost();
			QMessageBox::information(this, "Done", "Message has been sent", QMessageBox::Ok, QMessageBox::NoButton);
			reEnableAll();
			return;
	}
	sock.write(data);
}

void QFakeMail::connected()
{
	pd->setLabelText("Sending");
	pd->setValue(1);
	state = HELO;
}

void QFakeMail::error(const QString &err)
{
	sock.disconnectFromHost();
	QMessageBox::critical(this, "Error", err, QMessageBox::Ok, QMessageBox::NoButton);
	reEnableAll();
}

void QFakeMail::reEnableAll()
{
	pd->deleteLater();
	setEnabled(true);
	QApplication::setOverrideCursor(Qt::ArrowCursor);
}

inline void QFakeMail::encodeblock(const char in[3], char out[4])
{
	const char code[] = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	out[0] = code[(in[0] & 255) >> 2];
	out[1] = code[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
	out[2] = code[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)];
	out[3] = code[in[2] & 0x3f];
}

inline void QFakeMail::encodelastblock(const char in[3], char out[4], int len)
{
	const char code[] = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	out[0] = code[(in[0] & 255) >> 2];
	out[1] = code[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
	out[2] = (len & 2 ? code[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
	out[3] = '=';
}

inline int QFakeMail::base64_encode(const char *in, char *out, int len)
{
	int i = 0, j = 0;
	while(i < len - len % 3) {
		encodeblock(in + i, out + j);
		i += 3;
		j += 4;
	}
	if(len % 3) {
		encodelastblock(in + i, out + j, len % 3);
		j += 4;
	}
	out[j] = '\r';
	out[j + 1] = '\n';
	return j + 2;
}
