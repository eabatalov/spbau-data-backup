#include "archiver.h"
#include "meta_pack.h"
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
#include <QDir>
#include <QFile>
#include <QFileInfo>


#define DEBUG_FLAG false
#define LOG(format, var) do{ \
    if (DEBUG_FLAG) fprintf(stderr, "line: %d.  " format, __LINE__ ,var); \
    }while(0)

struct ArchiveInfo{
    std::uint64_t contentSize;
    ArchiveInfo()
        :contentSize(0){}
};

struct ArchivePackingState{
    char* filesContent;
    std::uint64_t contentFreePosition; // TODO => filesContentWritePos and try to remove filesContent ptr
    std::string dirAbsPath;
    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
    ArchivePackingState(char* filesContent, std::string dirAbsPath)
        :filesContent(filesContent)
        ,contentFreePosition(0)
        ,dirAbsPath(dirAbsPath)
    {}
};

typedef ArchivePackingState APS;

struct DirTimeSetTask
{
    int fd;
    std::uint64_t atime;
    std::uint64_t mtime;
    DirTimeSetTask(int fd, std::uint64_t atime, std::uint64_t mtime)
        :fd(fd), atime(atime), mtime(mtime)
    {}
};

struct ArchiveUnpackingState{
    char* filesContent;
    std::string dirAbsPath;
    ArchiverUtils::protobufStructs::PBArchiveMetaData* metaArchive;
    std::vector<DirTimeSetTask> dirsQueue;
    ArchiveUnpackingState(char* filesContent, std::string dirAbsPath, ArchiverUtils::protobufStructs::PBArchiveMetaData* metaArchive)
        :filesContent(filesContent)
        ,dirAbsPath(dirAbsPath)
        ,metaArchive(metaArchive)
    {}
};

typedef ArchiveUnpackingState AUS;


class fs_treeDeleter
{
public:
    void operator()(fs_tree* p) const
    {
        fs_tree_destroy(p);
    }
};

int computeArchiveSize(struct inode* inode, void* pointerToArchiveInfo);
int addInodeToArchive(struct inode* inode, void* pointerToAps);
int unpackInodeFromArchive(struct inode* inode, void* pointerToAus);
std::string getPathInArchive(struct inode* inode);
void recursiveUnpack(const std::string& dstUnpackPath, std::uint64_t curIndex,
                     const ArchiverUtils::protobufStructs::PBArchiveMetaData &metaArchive,
                     const QByteArray& archives, const std::vector<std::vector<uint64_t> > &fsTree);



std::string ArchiverUtils::getDirentName(std::string const & path)
{
    QDir dir(path.c_str());
    return dir.dirName().toStdString();
}

std::string ArchiverUtils::getDirAbsPath(std::string const & path)
{
    QFileInfo fileInfo(path.c_str());
    return fileInfo.absoluteDir().absolutePath().toStdString()
        + std::string(1, QDir::separator().toLatin1());
}

int computeArchiveSize(struct inode* inode, void* pointerToArchiveInfo)
{
	if ((inode->type != INODE_REG_FILE) && (inode->type != INODE_DIR))
	{
        std::cerr << "Unexpected type of file. File: " << inode->name << std::endl;
		return 1;
	}
    LOG("name: %s \n", inode->name);
    ArchiveInfo* archiveInfo = static_cast<ArchiveInfo*>(pointerToArchiveInfo);
    if (inode->type == INODE_REG_FILE)
        archiveInfo->contentSize += inode->attrs.st_size;
    return 1;
}

void writeDirentIndexInPBArchiveMetaData(std::uint64_t & direntIndexInPBArchiveMetaData, struct inode* inode, APS* aps)
{
    if (inode->user_data == NULL)
    {
        direntIndexInPBArchiveMetaData = aps->metaArchive.pbdirentmetadata_size();
        aps->metaArchive.add_pbdirentmetadata();
    }
    else
    {
        direntIndexInPBArchiveMetaData = (std::uint64_t)inode->user_data;
    }
}

void updateDirentInPBArchiveMetaData(std::uint64_t & direntIndexInPBArchiveMetaData, APS* aps, ArchiverUtils::protobufStructs::PBDirEntMetaData& tempMeta)
{
    if (aps->metaArchive.pbdirentmetadata().Get(direntIndexInPBArchiveMetaData).has_parentix())
    {
        tempMeta.set_parentix(aps->metaArchive.pbdirentmetadata().Get(direntIndexInPBArchiveMetaData).parentix());
    }
    else
    {
        tempMeta.set_parentix(direntIndexInPBArchiveMetaData);
    }
    aps->metaArchive.mutable_pbdirentmetadata()->Mutable(direntIndexInPBArchiveMetaData)->Swap(&tempMeta);
}

int addInodeToArchive(struct inode* inode, void* pointerToAps)
{
    APS* aps = static_cast<APS*>(pointerToAps);

    ArchiverUtils::protobufStructs::PBDirEntMetaData tempMeta;

    std::uint64_t direntIndexInPBArchiveMetaData = 0;
    writeDirentIndexInPBArchiveMetaData(direntIndexInPBArchiveMetaData, inode, aps);

    switch (inode->type) {
    case INODE_REG_FILE:
        pack_regfile_inode(reinterpret_cast<regular_file_inode*>(inode), &tempMeta,
            aps->dirAbsPath + getPathInArchive(inode), aps->filesContent, aps->contentFreePosition);
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

void unpackRegfileFromArchive(AUS* aus, const ArchiverUtils::protobufStructs::PBDirEntMetaData & curDirent, std::string & path)
{
    int fileDescriptor;
    timeval time[2];
    fileDescriptor = open(path.c_str(), O_WRONLY | O_CREAT, curDirent.mode());
    if (fileDescriptor == -1)
    {
        std::cerr << "Error in opening " << path << std::endl;
        return ;
    }
    write(fileDescriptor, aus->filesContent + curDirent.pbregfilemetadata().contentoffset(),
          curDirent.pbregfilemetadata().contentsize());
    if (fchown(fileDescriptor, curDirent.uid(), curDirent.gid()))
        std::cerr << "Error in chowning " << path << std::endl;
    time[0].tv_sec = curDirent.atime();
    time[0].tv_usec = 0;
    time[1].tv_sec = curDirent.mtime();
    time[1].tv_usec = 0;
    if (futimes(fileDescriptor, time))
        std::cerr << "Error in changing time " << path << std::endl;
    close(fileDescriptor);
}

void unpackDirFromArchive(struct inode * inode, AUS* aus, const ArchiverUtils::protobufStructs::PBDirEntMetaData & curDirent, std::string & path)
{
    int fileDescriptor;
    mkdir(path.c_str(), inode->attrs.st_mode);
    fileDescriptor = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fileDescriptor == -1)
    {
        std::cerr << "Error in opening " << path << std::endl;
        return ;
    }
    fchown(fileDescriptor, curDirent.uid(), curDirent.gid());
    aus->dirsQueue.push_back(DirTimeSetTask(fileDescriptor, curDirent.atime(), curDirent.mtime()));
}

int unpackInodeFromArchive(struct inode* inode, void* pointerToAus)
{
    AUS* aus = static_cast<AUS*>(pointerToAus);
    const ArchiverUtils::protobufStructs::PBDirEntMetaData& curDirent = aus->metaArchive->pbdirentmetadata((std::uint64_t)inode->user_data);
    std::string path = aus->dirAbsPath + getPathInArchive(inode);

    switch (inode->type)
    {
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

void restoreDirsTime(std::vector<DirTimeSetTask> dirsQueue)
{
    for (int i = dirsQueue.size()-1; i >= 0; --i)
    {
        timeval time[2];
        time[0].tv_sec = dirsQueue[i].atime;
        time[1].tv_sec = dirsQueue[i].mtime;
        time[0].tv_usec = 0;
        time[1].tv_usec = 0;
        if (futimes(dirsQueue[i].fd, time))
            std::cerr << "Error in changing time." << std::endl;
        close(dirsQueue[i].fd);
    }
}

Archiver::Archiver()
{}

std::string getPathInArchive(struct inode* inode)
{
    if (inode->parent == NULL)
        return ArchiverUtils::getDirentName(inode->name);
    else
        return getPathInArchive(inode->parent) + std::string(1, QDir::separator().toLatin1()) + inode->name;
}

void Archiver::pack(const char* srcPath, const char* dstArchiverPath)
{
    std::unique_ptr<fs_tree, fs_treeDeleter> tree(fs_tree_collect(srcPath));

    ArchiveInfo archiveInfo;
    fs_tree_bfs(tree.get(), computeArchiveSize, static_cast<void*>(&archiveInfo));

    std::unique_ptr<char[],std::default_delete<char[]> > filesContent(new char [archiveInfo.contentSize]);
    APS aps(filesContent.get(), ArchiverUtils::getDirAbsPath(srcPath));

    fs_tree_bfs(tree.get(), addInodeToArchive, static_cast<void*>(&aps));

    QFile fileContent(dstArchiverPath + QString("content"));
    fileContent.open(QIODevice::WriteOnly);
    fileContent.write(aps.filesContent, archiveInfo.contentSize);
    fileContent.close();

    // TODO but not now: merge meta and file contents into single archive file
    std::fstream output(dstArchiverPath + std::string("meta"), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!aps.metaArchive.SerializeToOstream(&output)) {
          std::cerr << "Failed to write meta info." << std::endl;
    }
}


void calcNumberDirChildren(ArchiverUtils::protobufStructs::PBArchiveMetaData & metaArchive, std::vector<std::uint64_t> & numberDirChildren)
{
    for (std::uint64_t i = 0; i < metaArchive.pbdirentmetadata_size(); ++i)
    {
        if (i == metaArchive.pbdirentmetadata(i).parentix())
            continue;
        ++numberDirChildren[metaArchive.pbdirentmetadata(i).parentix()];
    }
}

void buildFsTree(std::unique_ptr<fs_tree, fs_treeDeleter> & fsTree, ArchiverUtils::protobufStructs::PBArchiveMetaData & metaArchive, std::vector<std::uint64_t> & numberDirChildren)
{
    std::vector<std::uint64_t> curDirChildren(metaArchive.pbdirentmetadata_size());
    std::vector<inode*> indexToInodePointer(metaArchive.pbdirentmetadata_size());

    for (std::uint64_t i = 0; i < metaArchive.pbdirentmetadata_size(); ++i)
    {
        const ArchiverUtils::protobufStructs::PBDirEntMetaData& curDirent = metaArchive.pbdirentmetadata(i);
        inode* curInode;

        if (S_ISDIR(curDirent.mode()))
        {
            dir_inode* curDirInode = create_dir_inode();
            unpack_dir_inode(&curDirent, curDirInode, indexToInodePointer, numberDirChildren[i], i);
            curInode = reinterpret_cast<inode*>(curDirInode);
        } else
            if (S_ISREG(curDirent.mode()))
            {
                regular_file_inode* curRegFileInode = create_regular_file_inode();
                unpack_regfile_inode(&curDirent, curRegFileInode, indexToInodePointer, i);
                curInode = reinterpret_cast<inode*>(curRegFileInode);
            }

        if (curDirent.parentix() != i)
        {
            reinterpret_cast<dir_inode*>(indexToInodePointer[curDirent.parentix()])->children[curDirChildren[curDirent.parentix()]++] = curInode;
        }

        if (!i)
        {
            init_fs_tree_from_head(fsTree.get(), curInode);

        }
    }
}

void Archiver::unpack(const char* srcArchivePath, const char* dstPath)
{
    std::ifstream inputFileWithMeta(srcArchivePath + std::string("meta"), std::ios::binary);
    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
    metaArchive.ParseFromIstream(&inputFileWithMeta);

    std::vector<std::uint64_t> numberDirChildren(metaArchive.pbdirentmetadata_size());
    calcNumberDirChildren(metaArchive, numberDirChildren);

    std::unique_ptr<fs_tree, fs_treeDeleter> fsTree(create_fs_tree());
    buildFsTree(fsTree, metaArchive, numberDirChildren);

    QFile file((srcArchivePath + std::string("content")).c_str());
    file.open(QIODevice::ReadOnly);
    QByteArray contentFiles = file.readAll();

    AUS aus(contentFiles.data(), ArchiverUtils::getDirAbsPath(dstPath), &metaArchive);
    fs_tree_bfs(fsTree.get(), unpackInodeFromArchive, static_cast<void*>(&aus));

    restoreDirsTime(aus.dirsQueue);
}

void printInodeToQTextStream(struct inode* inode, QTextStream & qTextStream, std::uint64_t spaceCount)
{
    for (std::uint64_t i = 0; i < spaceCount; ++i)
        qTextStream << " ";

    qTextStream << inode->name << "\n";

    if (inode->type == INODE_DIR)
    {
        dir_inode* dir = (dir_inode*)inode;
        for (size_t i = 0; i < dir->num_children; ++i)
        {
            printInodeToQTextStream(dir->children[i], qTextStream, spaceCount + 1);
        }
    }
}

void Archiver::printArchiveFsTree(const char* srcArchivePath, QTextStream & qTextStream)
{
    std::ifstream inputFileWithMeta(srcArchivePath + std::string("meta"), std::ios::binary);
    ArchiverUtils::protobufStructs::PBArchiveMetaData metaArchive;
    metaArchive.ParseFromIstream(&inputFileWithMeta);

    std::vector<std::uint64_t> numberDirChildren(metaArchive.pbdirentmetadata_size());
    calcNumberDirChildren(metaArchive, numberDirChildren);

    std::unique_ptr<fs_tree, fs_treeDeleter> fsTree(create_fs_tree());
    buildFsTree(fsTree, metaArchive, numberDirChildren);

    printInodeToQTextStream(fsTree.get()->head, qTextStream, 0);
}

