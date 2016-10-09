QT += core
QT -= gui

!include( $$PWD/../lib/archiver/archiver.pri ){
    error( "Couldn't find the archiver.pri file!" )
}

TARGET = cmd_archiver
CONFIG += console
CONFIG -= app_bundle
CONFIG += warn_on
CONFIG += c++11

TEMPLATE = app

SOURCES += $$PWD/src/main.cpp \
    src/commandlinemanager.cpp


DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles

HEADERS += \
    src/commandlinemanager.h
