QT += core
QT -= gui

!include( $$PWD/../archiver.pri ){
    error( "Couldn't find the archiver.pri file!" )
}

TARGET = test_archiver
CONFIG += console
CONFIG -= app_bundle
CONFIG += warn_on
CONFIG += c++11

TEMPLATE = app

SOURCES += src/main.cpp

INCLUDEPATH += ../src

DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles
