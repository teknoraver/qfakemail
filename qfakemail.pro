FORMS += qfakemailwidget.ui
HEADERS += qfakemail.h
SOURCES += qfakemail.cpp main.cpp
RESOURCES += qfakemail.qrc
TEMPLATE = app
CONFIG += release warn_on thread qt
TARGET = qfakemail
QT += network widgets
RC_FILE = qfakemail.rc
DEFINES *= QT_USE_QSTRINGBUILDER

win32 {
	LIBS += -static
}