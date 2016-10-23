#ifndef ARCHIVER_STRUCTS
#define ARCHIVER_STRUCTS

#include <QString>
#include <struct_serialization.pb.h>
#include <vector>

struct ArchiveInfo {
    std::uint64_t contentSize;
    ArchiveInfo()
        :contentSize(0) {}
};

struct ArchivePackingState {
    char* filesContent;
    std::uint64_t contentFreePosition;
    QString dirAbsPath;
    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
    ArchivePackingState(char* filesContent, const QString & dirAbsPath)
        :filesContent(filesContent)
        ,contentFreePosition(0)
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

struct ArchiveUnpackingState {
    char* filesContent;
    QString dirAbsPath;
    ArchiverUtils::protobufStructs::PBArchiveMetaData* metaArchive;
    std::vector<DirTimeSetTask> dirsQueue;
    ArchiveUnpackingState(char* filesContent, const QString & dirAbsPath, ArchiverUtils::protobufStructs::PBArchiveMetaData* metaArchive)
        :filesContent(filesContent)
        ,dirAbsPath(dirAbsPath)
        ,metaArchive(metaArchive) {}
};

typedef ArchiveUnpackingState AUS;

#endif // ARCHIVER_STRUCTS

