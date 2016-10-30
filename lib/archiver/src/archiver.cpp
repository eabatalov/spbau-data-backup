#include "archiver.h"
#include "meta_pack.h"
#include "archiver_structs.h"
#include "archiver_utils.h"
#include <fs_tree.h>
#include <struct_serialization.pb.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <vector>

#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace apb = ArchiverUtils::protobufStructs;

#define DEBUG_FLAG true
#define LOG(format, var) do{ \
    if (DEBUG_FLAG) fprintf(stderr, "line: %d.  " format, __LINE__ ,var); \
    }while(0)

class fs_treeDeleter {
public:
    void operator()(fs_tree* p) const {
        fs_tree_destroy(p);
    }
};


//all new functions
int addInodeToArchive(struct inode* inode, void* pointerToAps);
void buildFsTree(std::unique_ptr<fs_tree, fs_treeDeleter> & fsTree, apb::PBArchiveMetaData & metaArchive,
                 std::vector<std::uint64_t> & numberDirChildren);
void calcNumberDirChildren(apb::PBArchiveMetaData & metaArchive, std::vector<std::uint64_t> & numberDirChildren);
void checkArchiveSizes(std::uint64_t metaSize, std::uint64_t contentSize, std::uint64_t inputFileSize);
int computeArchiveSize(struct inode* inode, void* pointerToArchiveInfo);
apb::PBArchiveMetaData getMetaDataFromArchive(QFile & input, std::uint64_t metaSize);
QString getPathInArchive(const apb::PBArchiveMetaData & archiveMeta, int index);
QString getPathInArchive(struct inode* inode);
std::uint64_t getSize(QFile & input);
void printInodeToQTextStream(struct inode* inode, QTextStream & qTextStream, std::uint64_t spaceCount);
void restoreDirsTime(const std::vector<DirTimeSetTask> & dirsQueue);
void updateDirentInPBArchiveMetaData(std::uint64_t & direntIndexInPBArchiveMetaData, APS* aps, apb::PBDirEntMetaData& tempMeta);
void unpackDirFromArchive(struct inode * inode, AUS* aus, const apb::PBDirEntMetaData & curDirent, QString & path);
int unpackInodeFromArchive(struct inode* inode, void* pointerToAus);
void unpackRegfileFromArchive(AUS* aus, const apb::PBDirEntMetaData & curDirent, QString & path);
void writeDirentIndexInPBArchiveMetaData(std::uint64_t & direntIndexInPBArchiveMetaData, struct inode* inode, APS* aps);
void writeOneFileToArcive(QString path, std::uint64_t contentOffset, QFile * archive, std::uint64_t size);
void readOneFileFromArcive(QString path, std::uint64_t contentOffset, QFile * archive, std::uint64_t size);
void writeContentToArchive(const QString & srcPath , const apb::PBArchiveMetaData & metaArchive, const std::uint64_t contentOffset, QFile * archive);
void writeEmptyContent(QFile * file, std::uint64_t size);

const static std::uint64_t MMAP_BUFFER_SIZE = 4096;

///////////////////////////////////////////
///////////////// PACK ////////////////////
///////////////////////////////////////////

void Archiver::pack(const QString &srcPath, const QString &dstArchiverPath) {
    QByteArray srcPathByteArray = srcPath.toLatin1();
    std::unique_ptr<fs_tree, fs_treeDeleter> tree(fs_tree_collect(srcPathByteArray.data()));

    ArchiveInfo archiveInfo;
    fs_tree_bfs(tree.get(), computeArchiveSize, static_cast<void*>(&archiveInfo));

    //std::unique_ptr<char[],std::default_delete<char[]> > filesContent(new char [archiveInfo.contentSize]);
    APS aps(ArchiverUtils::getDirAbsPath(srcPath));

    fs_tree_bfs(tree.get(), addInodeToArchive, static_cast<void*>(&aps));

    QFile output(dstArchiverPath);
    std::uint64_t metaSize = aps.metaArchive.ByteSize();
    std::unique_ptr<char[],std::default_delete<char[]> > meta(new char [metaSize]);
    output.open(QIODevice::ReadWrite);

    if (!aps.metaArchive.SerializeToArray(meta.get(), metaSize))
        throw Archiver::ArchiverException("Failed to serialize meta info of" + srcPath);

    if ((size_t)output.write((char*)&metaSize, ArchiverUtils::byteSizeOfNumber) < ArchiverUtils::byteSizeOfNumber)
        throw Archiver::ArchiverException("Failed to write metaSize of" + srcPath);

    if ((size_t)output.write((char*)&archiveInfo.contentSize, ArchiverUtils::byteSizeOfNumber) < ArchiverUtils::byteSizeOfNumber)
        throw Archiver::ArchiverException("Failed to write contentSize of " + srcPath);

    if ((size_t)output.write(meta.get(), metaSize) < metaSize)
        throw Archiver::ArchiverException("Failed to write meta of " + srcPath);

    writeEmptyContent(&output, archiveInfo.contentSize);

    writeContentToArchive(aps.dirAbsPath, aps.metaArchive, ArchiverUtils::byteSizeOfNumber * 2 + metaSize, &output);

//    if ((size_t)output.write(filesContent.get(), archiveInfo.contentSize) < archiveInfo.contentSize)
//        throw Archiver::ArchiverException("Failed to write filesContent of " + srcPath);
}

int computeArchiveSize(struct inode* inode, void* pointerToArchiveInfo) {
    if ((inode->type != INODE_REG_FILE) && (inode->type != INODE_DIR))
        throw Archiver::ArchiverException(QString("Unexpected type of file. File: ") + inode->name);
    LOG("computeArchiveSize: %s\n", inode->name);

    ArchiveInfo* archiveInfo = static_cast<ArchiveInfo*>(pointerToArchiveInfo);
    if (inode->type == INODE_REG_FILE)
        archiveInfo->contentSize += inode->attrs.st_size;
    return 1;
}

int addInodeToArchive(struct inode* inode, void* pointerToAps) {
    APS* aps = static_cast<APS*>(pointerToAps);

    apb::PBDirEntMetaData tempMeta;

    std::uint64_t direntIndexInPBArchiveMetaData = 0;
    writeDirentIndexInPBArchiveMetaData(direntIndexInPBArchiveMetaData, inode, aps);

    switch (inode->type) {
    case INODE_REG_FILE:
        pack_regfile_inode(reinterpret_cast<regular_file_inode*>(inode), &tempMeta, aps->contentFreePosition);
        break;
    case INODE_DIR:
        pack_dir_inode(reinterpret_cast<dir_inode*>(inode), &tempMeta, &(aps->metaArchive), direntIndexInPBArchiveMetaData);
        break;
    default:
        break;
    }

    updateDirentInPBArchiveMetaData(direntIndexInPBArchiveMetaData, aps, tempMeta);

    return 1;
}

void writeDirentIndexInPBArchiveMetaData(std::uint64_t & direntIndexInPBArchiveMetaData, struct inode* inode, APS* aps) {
    if (inode->user_data == NULL) {
        direntIndexInPBArchiveMetaData = aps->metaArchive.pbdirentmetadata_size();
        aps->metaArchive.add_pbdirentmetadata();
    } else {
        direntIndexInPBArchiveMetaData = (std::uint64_t)inode->user_data;
    }
}

void updateDirentInPBArchiveMetaData(std::uint64_t & direntIndexInPBArchiveMetaData, APS* aps, apb::PBDirEntMetaData& tempMeta) {
    if (aps->metaArchive.pbdirentmetadata().Get(direntIndexInPBArchiveMetaData).has_parentix()) {
        tempMeta.set_parentix(aps->metaArchive.pbdirentmetadata().Get(direntIndexInPBArchiveMetaData).parentix());
    } else {
        tempMeta.set_parentix(direntIndexInPBArchiveMetaData);
    }
    aps->metaArchive.mutable_pbdirentmetadata()->Mutable(direntIndexInPBArchiveMetaData)->Swap(&tempMeta);
}

void writeContentToArchive(const QString & srcPath , const apb::PBArchiveMetaData & metaArchive, const std::uint64_t contentOffset, QFile * archive) {
    for (int i = 0; i < metaArchive.pbdirentmetadata_size(); ++i) {
        if (metaArchive.pbdirentmetadata(i).has_pbregfilemetadata()) {
            QString path = srcPath + getPathInArchive(metaArchive, i);
            std::uint64_t offsetToFile = metaArchive.pbdirentmetadata(i).pbregfilemetadata().contentoffset();
            std::uint64_t sizeOfFile = metaArchive.pbdirentmetadata(i).pbregfilemetadata().contentsize();
            writeOneFileToArcive(path, contentOffset + offsetToFile, archive, sizeOfFile);
        }
    }
}

void writeEmptyContent(QFile * file, std::uint64_t size) {
    while (size > 0) {
        int temp = 0;
        std::uint64_t actualSize = std::min(sizeof(temp), size);
        file->write((char*)&temp, actualSize);
        size -= actualSize;
    }
}

void writeOneFileToArcive(QString path, std::uint64_t contentOffset, QFile * archive, std::uint64_t size) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw Archiver::ArchiverException(QString("Cannot open file: ") + path);
    }


    while (size > 0) {
        std::uint64_t actualSize = std::min(MMAP_BUFFER_SIZE, size);

        LOG("actualSize = %lld \n", actualSize);
        LOG("file.size() = %lld \n", file.size());
        LOG("contentOffset = %lld \n", contentOffset);
        LOG("archive.size() = %lld \n", archive->size());


        uchar* mmap = file.map(0, actualSize);
        if (mmap == NULL) {
            throw Archiver::ArchiverException(QString("Cannot mapped file: ") + path);
        }
        uchar* archiveMmap = archive->map(contentOffset, actualSize);
        if (archiveMmap == NULL) {
            throw Archiver::ArchiverException(QString("Cannot mapped archive with file: ") + path);
        }

        memmove(archiveMmap, mmap, actualSize);


        if (!file.unmap(mmap)) {
            throw Archiver::ArchiverException(QString("Cannot unmapped file: ") + path);
        }
        if (!archive->unmap(archiveMmap)) {
            throw Archiver::ArchiverException(QString("Cannot unmapped archive with file: ") + path);
        }
        contentOffset += actualSize;
        size -= actualSize;
    }
}


///////////////////////////////////////////
//////////////// UNPACK ///////////////////
///////////////////////////////////////////

void Archiver::unpack(const QString &srcArchivePath, const QString &dstPath) {
    QFile input(srcArchivePath);
    if (!input.open(QIODevice::ReadOnly))
        throw ArchiverException("Error with opening " + srcArchivePath);

    std::uint64_t metaSize = getSize(input);
    std::uint64_t contentSize = getSize(input);

    checkArchiveSizes(metaSize, contentSize, input.size());

    apb::PBArchiveMetaData metaArchive = getMetaDataFromArchive(input, metaSize);

    std::vector<std::uint64_t> numberDirChildren(metaArchive.pbdirentmetadata_size());
    calcNumberDirChildren(metaArchive, numberDirChildren);

    std::unique_ptr<fs_tree, fs_treeDeleter> fsTree(create_fs_tree());
    buildFsTree(fsTree, metaArchive, numberDirChildren);

    AUS aus(&input, ArchiverUtils::getDirAbsPath(dstPath), &metaArchive);
    fs_tree_bfs(fsTree.get(), unpackInodeFromArchive, static_cast<void*>(&aus));

    restoreDirsTime(aus.dirsQueue);
}

int unpackInodeFromArchive(struct inode* inode, void* pointerToAus) {
    AUS* aus = static_cast<AUS*>(pointerToAus);
    const apb::PBDirEntMetaData& curDirent = aus->metaArchive->pbdirentmetadata((std::uint64_t)inode->user_data);
    QString path = aus->dirAbsPath + getPathInArchive(inode);

    switch (inode->type) {
    case INODE_REG_FILE:
        unpackRegfileFromArchive(aus, curDirent, path);
        break;
    case INODE_DIR:
        unpackDirFromArchive(inode, aus, curDirent, path);
        break;
    default:
        break;
    }

    return 1;
}

void unpackRegfileFromArchive(AUS* aus, const apb::PBDirEntMetaData & curDirent, QString & path) {
    readOneFileFromArcive(path, ArchiverUtils::byteSizeOfNumber * 2 + aus->metaArchive->ByteSize() + curDirent.pbregfilemetadata().contentoffset(), aus->archive, curDirent.pbregfilemetadata().contentsize());

    std::string pathStdString = path.toStdString();
    int fileDescriptor;
    timeval time[2];
    fileDescriptor = open(pathStdString.c_str(), O_WRONLY | O_CREAT, curDirent.mode());
    if (fileDescriptor == -1) {
        qCritical() << "Error in opening " << path << '\n';
        return ;
    }

//    write(fileDescriptor, aus->filesContent + curDirent.pbregfilemetadata().contentoffset(),
//          curDirent.pbregfilemetadata().contentsize());

    if (fchown(fileDescriptor, curDirent.uid(), curDirent.gid()))
        qCritical() << "Error in chowning " << path << '\n';

    time[0].tv_sec = curDirent.atime();
    time[0].tv_usec = 0;
    time[1].tv_sec = curDirent.mtime();
    time[1].tv_usec = 0;
    if (futimes(fileDescriptor, time))
        qCritical() << "Error in changing time " << path << '\n';

    close(fileDescriptor);
}

void unpackDirFromArchive(struct inode * inode, AUS* aus, const apb::PBDirEntMetaData & curDirent, QString & path) {
    std::string pathStdString = path.toStdString();
    int fileDescriptor;
    mkdir(pathStdString.c_str(), inode->attrs.st_mode);
    fileDescriptor = open(pathStdString.c_str(), O_RDONLY | O_DIRECTORY);
    if (fileDescriptor == -1) {
        qCritical() << "Error in opening " << path << '\n';
        return ;
    }
    fchown(fileDescriptor, curDirent.uid(), curDirent.gid());
    aus->dirsQueue.push_back(DirTimeSetTask(fileDescriptor, curDirent.atime(), curDirent.mtime()));
}

void restoreDirsTime(const std::vector<DirTimeSetTask> & dirsQueue) {
    for (int i = dirsQueue.size()-1; i >= 0; --i) {
        timeval time[2];
        time[0].tv_sec = dirsQueue[i].atime;
        time[1].tv_sec = dirsQueue[i].mtime;
        time[0].tv_usec = 0;
        time[1].tv_usec = 0;
        if (futimes(dirsQueue[i].fd, time))
            qCritical() << "Error in changing time." << '\n';
        close(dirsQueue[i].fd);
    }
}

void readOneFileFromArcive(QString path, std::uint64_t contentOffset, QFile * archive, std::uint64_t size) {
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite)) {
        throw Archiver::ArchiverException(QString("Cannot open file: ") + path);
    }

    writeEmptyContent(&file, size);

    while (size > 0) {
        std::uint64_t actualSize = std::min(MMAP_BUFFER_SIZE, size);
        LOG("actualSize = %lld \n", actualSize);
        LOG("file.size() = %lld \n", file.size());
        uchar* mmap = file.map(0, actualSize);
        if (mmap == NULL) {
            throw Archiver::ArchiverException(QString("Cannot mapped file: ") + path);
        }
        uchar* archiveMmap = archive->map(contentOffset, actualSize);
        if (archiveMmap == NULL) {
            throw Archiver::ArchiverException(QString("Cannot mapped archive with file: ") + path);
        }

        memmove(mmap, archiveMmap, actualSize);


        if (!file.unmap(mmap)) {
            throw Archiver::ArchiverException(QString("Cannot unmapped file: ") + path);
        }
        if (!archive->unmap(archiveMmap)) {
            throw Archiver::ArchiverException(QString("Cannot unmapped archive with file: ") + path);
        }
        contentOffset += actualSize;
        size -= actualSize;
    }
}

///////////////////////////////////////////
/////// GET_ARCHIVE_WITHOUT_CONTENT ///////
///////////////////////////////////////////

QByteArray Archiver::getArchiveWithoutContent(const QString & srcArchivePath) {
    QByteArray archiveWithoutContent;

    QFile input(srcArchivePath);
    if (!input.open(QIODevice::ReadOnly)) {
        throw ArchiverException("Error with opening " + srcArchivePath);
    }

    std::uint64_t metaSize = getSize(input);
    std::uint64_t contentSize = getSize(input);

    checkArchiveSizes(metaSize, contentSize, input.size());

    std::unique_ptr<char[],std::default_delete<char[]> > bufferForMeta(new char [metaSize]);
    if ((std::uint64_t)input.read(bufferForMeta.get(), metaSize) < metaSize) {
        throw ArchiverException("Error with reading meta");
    }

    archiveWithoutContent.resize(metaSize + 2 * ArchiverUtils::byteSizeOfNumber);

    contentSize = 0;
    memcpy(archiveWithoutContent.data(), &metaSize, ArchiverUtils::byteSizeOfNumber);
    memcpy(archiveWithoutContent.data() + ArchiverUtils::byteSizeOfNumber,
           &contentSize, ArchiverUtils::byteSizeOfNumber);
    memcpy(archiveWithoutContent.data() + 2 * ArchiverUtils::byteSizeOfNumber,
           bufferForMeta.get(), metaSize);

    return archiveWithoutContent;
}


///////////////////////////////////////////
////////// PRINT_ARCHIVE_FS_TREE //////////
///////////////////////////////////////////

void Archiver::printArchiveFsTree(const QString &srcArchivePath, QTextStream & qTextStream) {
    QFile input(srcArchivePath);
    if (!input.open(QIODevice::ReadOnly)) {
        throw ArchiverException("Error with opening " + srcArchivePath);
    }

    std::uint64_t metaSize = getSize(input);
    std::uint64_t contentSize = getSize(input);

    checkArchiveSizes(metaSize, contentSize, input.size());

    apb::PBArchiveMetaData metaArchive = getMetaDataFromArchive(input, metaSize);

    std::vector<std::uint64_t> numberDirChildren(metaArchive.pbdirentmetadata_size());
    calcNumberDirChildren(metaArchive, numberDirChildren);

    std::unique_ptr<fs_tree, fs_treeDeleter> fsTree(create_fs_tree());
    buildFsTree(fsTree, metaArchive, numberDirChildren);

    printInodeToQTextStream(fsTree.get()->head, qTextStream, 0);
}

void printInodeToQTextStream(struct inode* inode, QTextStream & qTextStream, std::uint64_t spaceCount) {
    LOG("visit %s\n", inode->name);
    for (std::uint64_t i = 0; i < spaceCount; ++i)
        qTextStream << " ";

    qTextStream << inode->name << "\n";

    if (qTextStream.status() == QTextStream::WriteFailed) {
        qCritical() << "Error in writing " << inode->name << " in qTextStream" << '\n';
    }

    if (inode->type == INODE_DIR) {
        dir_inode* dir = (dir_inode*)inode;
        for (size_t i = 0; i < dir->num_children; ++i) {
            printInodeToQTextStream(dir->children[i], qTextStream, spaceCount + 1);
        }
    }
}


///////////////////////////////////////////
/////////// ArchiverUtils /////////////////
///////////////////////////////////////////

QString ArchiverUtils::getDirentName(const QString & path) {
    QDir dir(path);
    return dir.dirName();
}

QString ArchiverUtils::getDirAbsPath(const QString & path) {
    QFileInfo fileInfo(path);
    return fileInfo.absoluteDir().absolutePath() + QDir::separator();
}


///////////////////////////////////////////
///////// Support Functions ///////////////
///////////////////////////////////////////

QString getPathInArchive(const apb::PBArchiveMetaData & archiveMeta, int index) {
    if (archiveMeta.pbdirentmetadata(index).parentix() == index)
        return ArchiverUtils::getDirentName(QString::fromStdString(archiveMeta.pbdirentmetadata(index).name()));
    else
        return getPathInArchive(archiveMeta, archiveMeta.pbdirentmetadata(index).parentix())
                + QDir::separator() + QString::fromStdString(archiveMeta.pbdirentmetadata(index).name());
}

QString getPathInArchive(struct inode* inode) {
    if (inode->parent == NULL)
        return ArchiverUtils::getDirentName(inode->name);
    else
        return getPathInArchive(inode->parent)
                + QDir::separator() + inode->name;
}

void calcNumberDirChildren(apb::PBArchiveMetaData & metaArchive, std::vector<std::uint64_t> & numberDirChildren) {
    for (std::uint64_t i = 0; i < (std::uint64_t)metaArchive.pbdirentmetadata_size(); ++i) {
        if (i == metaArchive.pbdirentmetadata(i).parentix())
            continue;
        ++numberDirChildren[metaArchive.pbdirentmetadata(i).parentix()];
    }
}

void buildFsTree(std::unique_ptr<fs_tree, fs_treeDeleter> & fsTree, apb::PBArchiveMetaData & metaArchive,
                 std::vector<std::uint64_t> & numberDirChildren) {
    std::vector<std::uint64_t> curDirChildren(metaArchive.pbdirentmetadata_size());
    std::vector<inode*> indexToInodePointer(metaArchive.pbdirentmetadata_size());

    for (std::uint64_t i = 0; i < (std::uint64_t)metaArchive.pbdirentmetadata_size(); ++i) {
        const apb::PBDirEntMetaData& curDirent = metaArchive.pbdirentmetadata(i);
        inode* curInode;

        if (S_ISDIR(curDirent.mode())) {
            dir_inode* curDirInode = create_dir_inode();
            unpack_dir_inode(&curDirent, curDirInode, indexToInodePointer, numberDirChildren[i], i);
            curInode = reinterpret_cast<inode*>(curDirInode);
        } else
            if (S_ISREG(curDirent.mode())) {
                regular_file_inode* curRegFileInode = create_regular_file_inode();
                unpack_regfile_inode(&curDirent, curRegFileInode, indexToInodePointer, i);
                curInode = reinterpret_cast<inode*>(curRegFileInode);
            }

        LOG("buildFsTree on %s\n", curInode->name);

        if (curDirent.parentix() != i) {
            reinterpret_cast<dir_inode*>(indexToInodePointer[curDirent.parentix()])->children[curDirChildren[curDirent.parentix()]++] = curInode;
        }

        if (!i) {
            init_fs_tree_from_head(fsTree.get(), curInode);
        }
    }
}

std::uint64_t getSize(QFile & input) {
    std::unique_ptr<char[],std::default_delete<char[]> > bufferForNumber(new char [ArchiverUtils::byteSizeOfNumber]);

    if ((size_t)input.read(bufferForNumber.get(), ArchiverUtils::byteSizeOfNumber) < ArchiverUtils::byteSizeOfNumber) {
        throw Archiver::ArchiverException("Error with reading size.");
    }

    return *((std::uint64_t*)bufferForNumber.get());
}

void checkArchiveSizes(std::uint64_t metaSize, std::uint64_t contentSize, std::uint64_t inputFileSize) {
    if (metaSize + contentSize + 2 * ArchiverUtils::byteSizeOfNumber != inputFileSize) {
        throw Archiver::ArchiverException(QString("Error with size of file.\nExpecting: ")
                                + (metaSize + contentSize + 2 * ArchiverUtils::byteSizeOfNumber)
                                + QString(". Got: ") + inputFileSize);
    }
}

apb::PBArchiveMetaData getMetaDataFromArchive(QFile & input, std::uint64_t metaSize) {
    std::unique_ptr<char[],std::default_delete<char[]> > bufferForMeta(new char [metaSize]);
    if ((std::uint64_t)input.read(bufferForMeta.get(), metaSize) < metaSize) {
        throw Archiver::ArchiverException("Error with reading meta");
    }

    apb::PBArchiveMetaData metaArchive;
    if (!metaArchive.ParseFromArray(bufferForMeta.get(), metaSize)) {
        throw Archiver::ArchiverException("Error with parse meta");
    }

    return metaArchive;
}
