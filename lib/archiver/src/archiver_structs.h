#ifndef ARCHIVER_STRUCTS
#define ARCHIVER_STRUCTS

#include <QString>
#include <struct_serialization.pb.h>
#include <vector>
#include <QFile>

struct ArchiveInfo {
    std::uint64_t contentSize;
    ArchiveInfo()
        :contentSize(0) {}
};

//struct ArchivePackingState {
//    char* filesContent;
//    std::uint64_t contentFreePosition;
//    QString dirAbsPath;
//    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
//    ArchivePackingState(char* filesContent, const QString & dirAbsPath)
//        :filesContent(filesContent)
//        ,contentFreePosition(0)
//        ,dirAbsPath(dirAbsPath) {}
//};
struct ArchivePackingState {
    std::uint64_t contentFreePosition;
    QString dirAbsPath;
    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
    ArchivePackingState(const QString & dirAbsPath)
        :contentFreePosition(0)
        ,dirAbsPath(dirAbsPath) {}
};

typedef ArchivePackingState APS;

struct DirTimeSetTask {
    int fd;
    std::uint64_t atime;
    std::uint64_t mtime;
    DirTimeSetTask(int fd, std::uint64_t atime, std::uint64_t mtime)
        :fd(fd), atime(atime), mtime(mtime) {}
};

//struct ArchiveContentWritingState {
//    QFile *archive;
//    std::uint64_t contentOffset;
//    QString dirAbsPath;
//    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
//    ArchivePackingState(QFile *archive, std::uint64_t contentOffset, const QString & dirAbsPath)
//        :archive(archive)
//        ,contentOffset(contentOffset)
//        ,dirAbsPath(dirAbsPath) {}
//};

struct ArchiveUnpackingState {
    QFile *archive;
    QString dirAbsPath;
    ArchiverUtils::protobufStructs::PBArchiveMetaData* metaArchive;
    std::vector<DirTimeSetTask> dirsQueue;
    ArchiveUnpackingState(QFile *archive, const QString & dirAbsPath, ArchiverUtils::protobufStructs::PBArchiveMetaData* metaArchive)
        :archive(archive)
        ,dirAbsPath(dirAbsPath)
        ,metaArchive(metaArchive) {}
};

typedef ArchiveUnpackingState AUS;

#endif // ARCHIVER_STRUCTS

