QT += core
QT += network
QT -= gui
CONFIG += c++11

#QMAKE_CXXFLAGS += -std=c++11
#QMAKE_LFLAGS +=  -std=c++11

!include( $$PWD/../../lib/archiver/archiver.pri ){
    error( "Couldn't find the archiver.pri file!" )
}

!include( $$PWD/../../lib/networking/networking.pri ){
    error( "Couldn't find the networking.pri file!" )
}

!include( $$PWD/../../lib/consoleRW/consolerw.pri ){
    error( "Couldn't find the consolerw.pri file!" )
}

#system($$PWD/../protocol/gen.sh)

TARGET = client
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle


TEMPLATE = app

INCLUDEPATH += \
               $$PWD/../protocol/ \
               $$PWD/../protocol/gen

SOURCES += $$PWD/src/main.cpp \
           $$PWD/src/clientsession.cpp \
           $$PWD/../protocol/gen/networkMsgStructs.pb.cc

HEADERS += \
    $$PWD/src/clientsession.h \
    $$PWD/src/client_utils.h \
    $$PWD/../protocol/protocol.h \
    $$PWD/../protocol/gen/networkMsgStructs.pb.h
    
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles

