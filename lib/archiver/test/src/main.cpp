#include <QCoreApplication>
#include <archiver.h>
#include <iostream>
#include <string>
#include <cstring>
#include <QFile>
#include <QDir>
#include <QByteArray>

#include "struct_serialization.pb.h"
#include "archiver_utils.h"
#include <fs_tree.h>

void checkArchiver(std::string dir1, std::string dir2);

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc != 4)
    {
        std::cerr << "Incorrect number of arguments in cmd!\nPlease print one of:\n\"-p sourcePath outputFileArchive\" to pack sourcePath to outputFileArchive" << std::endl <<
                     "\"-u inputFileArchive outputPath\" to unpack inputFileArchive to outputPath" << std::endl <<
                     "\"-c sourcePath outputPath\" to check archiver's work for sourcePath and outputPath " << std::endl;
        return -1;
    }

    if (!strcmp("-p", argv[1]))
    {
        Archiver::pack(argv[2], argv[3]);
    }
    else if (!strcmp("-u", argv[1]))
    {
        Archiver::unpack(argv[2], argv[3]);
    }
    else if (!strcmp("-c", argv[1]))
    {
        checkArchiver(argv[2], argv[3]);
    }
    else
    {
        std::cerr << "Unknown first argument in cmd!\nPlease print one of:\n\"-p sourcePath outputFileArchive\" to pack sourcePath to outputFileArchive" << std::endl <<
                     "\"-u inputFileArchive outputPath\" to unpack inputFileArchive to outputPath" << std::endl <<
                     "\"-c sourcePath outputPath\" to check archiver's work for sourcePath and outputPath " << std::endl;
        return -1;
    }


//  Example:

//    archiver.pack("/home/kodark/Документы/CSC", "/home/kodark/Видео/test.pck");
//    archiver.unpack("/home/kodark/Видео/test.pck", "/home/kodark/Видео/2");

//    std::cout << "Done." << std::endl;

//    checkArchiver("/home/kodark/Документы/CSC", "/home/kodark/Видео/2/CSC");

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

bool checkInode(inode* first, inode* second, const std::string & dir1, const std::string & dir2)
{
    std::string name1 = first->name;
    std::string name2 = second->name;
    if (first->parent == NULL)
    {
        name1 = ArchiverUtils::getDirentName(first->name);
        name2 = ArchiverUtils::getDirentName(second->name);
    }
    if (name1 != name2)
    {
        std::cerr << dir1 + name1 << "!=" << dir2 + name2 << std::endl;
        return false;
    }

    if (first->type != second->type)
    {
        std::cerr << dir1 + name1 << ": different types" << std::endl;
        return false;
    }
    if (first->attrs.st_atime != second->attrs.st_atime || first->attrs.st_mtime != second->attrs.st_mtime ||
            first->attrs.st_mode != second->attrs.st_mode || first->attrs.st_uid != second->attrs.st_uid ||
            first->attrs.st_gid != second->attrs.st_gid)
    {
        std::cerr << dir1 + name1 << ": different attrs" << std::endl;
        return false;
    }

    if (first->type == INODE_REG_FILE)
    {
        if (first->attrs.st_size != second->attrs.st_size)
        {
            std::cerr << dir1 + name1 << ": different size" << std::endl;
            return false;
        }
        QFile file1((dir1 + name1).c_str());
        file1.open(QIODevice::ReadOnly);
        QFile file2((dir2 + name2).c_str());
        file2.open(QIODevice::ReadOnly);
        QByteArray bytes1 = file1.readAll();
        QByteArray bytes2 = file2.readAll();
        if (strcmp(bytes1.data(), bytes2.data()))
        {
            std::cerr << dir1 + name1 << ": different content" << std::endl;
            return false;
        }
    }

    if (first->type == INODE_DIR)
    {
        dir_inode* first_dir = reinterpret_cast<dir_inode*>(first);
        dir_inode* second_dir = reinterpret_cast<dir_inode*>(second);
        if (first_dir->num_children != second_dir->num_children)
        {
            std::cerr << dir1 + name1 << ": different numchildren" << std::endl;
            return false;
        }

        for (std::uint64_t i = 0; i < first_dir->num_children; ++i)
        {
            if ( !checkInode(first_dir->children[i], second_dir->children[i], dir1 + name1 + "/", dir2 + name2 + "/") )
            {
                return false;
            }
        }

    }

    return true;
}

void checkArchiver(std::string dir1, std::string dir2)
{
    fs_tree* tree1 = fs_tree_collect(dir1.c_str());
    fs_tree* tree2 = fs_tree_collect(dir2.c_str());

    dir1 = ArchiverUtils::getDirAbsPath(dir1);
    dir2 = ArchiverUtils::getDirAbsPath(dir2);

    if (checkInode(tree1->head, tree2->head, dir1, dir2))
        std::cout << "Ok!" << std::endl;
    else
        std::cout << "Bad archiver..." << std::endl;
}
