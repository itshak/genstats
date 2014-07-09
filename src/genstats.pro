#-------------------------------------------------
#
# Project created by QtCreator 2014-07-08T16:19:35
#
#-------------------------------------------------

QT       += core
QT       += xlsx

QT       -= gui

TARGET = genstats
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    dataaccess.cpp

HEADERS += \
    dataaccess.h \
    types.h

LIBS += -l sqlite3
