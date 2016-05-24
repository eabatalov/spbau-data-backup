QT += core
QT += network
QT -= gui
CONFIG += c++11

#QMAKE_CXXFLAGS += -std=c++11
#QMAKE_LFLAGS +=  -std=c++11

!include( $$PWD/../archiver/archiver.pri ){
    error( "Couldn't find the archiver.pri file!" )
}

#system($$PWD/../common/gen.sh)

TARGET = client
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle


TEMPLATE = app

INCLUDEPATH += $$PWD/../echoclient/src \
               $$PWD/../common/ \
               $$PWD/../common/gen

SOURCES += $$PWD/src/main.cpp \
           $$PWD/../echoclient/src/consolestream.cpp \
           $$PWD/../echoclient/src/networkstream.cpp \
           $$PWD/src/clientsession.cpp \
           $$PWD/../common/gen/networkMsgStructs.pb.cc

HEADERS += \
    $$PWD/../echoclient/src/consolestream.h \
    $$PWD/../echoclient/src/networkstream.h \
    $$PWD/src/clientsession.h \
    $$PWD/src/client_utils.h \
    $$PWD/../common/protocol.h \
    $$PWD/../common/gen/networkMsgStructs.pb.h
    
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles

