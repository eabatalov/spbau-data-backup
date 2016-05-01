QT += core
QT += network
QT -= gui

TARGET = echoserver
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += $$PWD/src/main.cpp \
    $$PWD/src/server.cpp \
    $$PWD/src/servernetworkstream.cpp \
    $$PWD/../echoclient/src/networkstream.cpp \
    $$PWD/src/perclient.cpp

HEADERS += \
    $$PWD/src/server.h \
    $$PWD/src/servernetworkstream.h \
    $$PWD/src/perclient.h \
    $$PWD/../echoclient/src/networkstream.h

DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles
