system($$PWD/gen.sh)

!include( $$PWD/../collect_file_tree/collect_file_tree.pri ){
    error( "Couldn't find the collect_file_tree.pri file!" )
}

LIBS += -L/usr/local/lib -lprotobuf

SOURCES += $$PWD/src/archiver.cpp \
           $$PWD/gen/struct_serialization.pb.cc

HEADERS += $$PWD/src/archiver.h \
           $$PWD/gen/struct_serialization.pb.h \
           $$PWD/src/meta_pack.h \
           $$PWD/src/archiver_utils.h

INCLUDEPATH += $$PWD/gen \
               $$PWD/src

DEPENDPATH += $$PWD/gen
