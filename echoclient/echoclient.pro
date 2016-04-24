QT += core
QT += network
QT -= gui

TARGET = echoclient
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += src/main.cpp \
    src/consolestream.cpp \
    src/networkstream.cpp \
    src/echoclientlogic.cpp

HEADERS += \
    src/consolestream.h \
    src/networkstream.h \
    src/echoclientlogic.h
    
DESTDIR = ./bin
OBJECTS_DIR = ./objectfiles
MOC_DIR = ./mocfiles

