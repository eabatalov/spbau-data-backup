QT += core
QT += network
QT -= gui

TARGET = echoclient
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += $$PWD/src/main.cpp \
    $$PWD/src/consolestream.cpp \
    $$PWD/src/networkstream.cpp \
    $$PWD/src/echoclientlogic.cpp

HEADERS += \
    $$PWD/src/consolestream.h \
    $$PWD/src/networkstream.h \
    $$PWD/src/echoclientlogic.h
    
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles

