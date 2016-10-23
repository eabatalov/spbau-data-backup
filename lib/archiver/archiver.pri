system($$PWD/gen.sh)

!include( $$PWD/../fs_tree/fs_tree.pri ){
    error( "Couldn't find the fs_tree.pri file!" )
}

LIBS += -L/usr/local/lib -lprotobuf

SOURCES += $$PWD/src/archiver.cpp \
           $$PWD/gen/struct_serialization.pb.cc

HEADERS += $$PWD/src/archiver.h \
           $$PWD/gen/struct_serialization.pb.h \
           $$PWD/src/meta_pack.h \
           $$PWD/src/archiver_utils.h \
           $$PWD/src/archiver_structs.h

INCLUDEPATH += $$PWD/gen \
               $$PWD/src

DEPENDPATH += $$PWD/gen
