QT += core
QT += network
QT -= gui
CONFIG += c++11

#QMAKE_CXXFLAGS += -std=c++11
#QMAKE_LFLAGS +=  -std=c++11

#system($$PWD/../common/gen.sh)

TARGET = server
CONFIG += console
CONFIG += warn_on
CONFIG -= app_bundle

LIBS += -L/usr/local/lib -lprotobuf


TEMPLATE = app

INCLUDEPATH += $$PWD/../echoclient/src \
               $$PWD/../common/ \
               $$PWD/../common/gen \
               $$PWD/../echoserver/src \
               $$PWD/gen

SOURCES += $$PWD/src/main.cpp \
           $$PWD/src/serverclientmanager.cpp \
           $$PWD/../echoclient/src/networkstream.cpp \
           $$PWD/src/clientsessiononserver.cpp \
           $$PWD/../common/gen/networkMsgStructs.pb.cc \
           $$PWD/gen/serverStructs.pb.cc


HEADERS += \
    $$PWD/src/serverclientmanager.h \
    $$PWD/src/clientsessiononserver.h \
    $$PWD/../echoclient/src/networkstream.h \
    $$PWD/../common/protocol.h \
    $$PWD/../common/gen/networkMsgStructs.pb.h \
    $$PWD/gen/serverStructs.pb.h
    
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/objectfiles
MOC_DIR = $$PWD/mocfiles

