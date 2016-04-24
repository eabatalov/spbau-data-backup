QT += core
QT += network
QT -= gui

TARGET = echoserver
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += src/main.cpp \
    src/server.cpp \
    src/servernetworkstream.cpp \
    ../echoclient/src/networkstream.cpp \
    src/perclient.cpp

HEADERS += \
    src/server.h \
    src/servernetworkstream.h \
    src/perclient.h \
    ../echoclient/src/networkstream.h

DESTDIR = ./bin
OBJECTS_DIR = ./objectfiles
MOC_DIR = ./mocfiles
