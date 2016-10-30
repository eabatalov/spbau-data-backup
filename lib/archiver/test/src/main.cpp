#include <QCoreApplication>
#include <archiver.h>
#include <iostream>
#include <string>
#include <cstring>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QDebug>

#include "struct_serialization.pb.h"
#include "archiver_utils.h"
#include <fs_tree.h>

void checkArchiver(QString dir1, QString dir2);

int main(int argc, char *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc != 4) {
        std::cerr << "Incorrect number of arguments in cmd!\nPlease print one of:\n\"-p sourcePath outputFileArchive\" to pack sourcePath to outputFileArchive" << std::endl <<
                     "\"-u inputFileArchive outputPath\" to unpack inputFileArchive to outputPath" << std::endl <<
                     "\"-c sourcePath outputPath\" to check archiver's work for sourcePath and outputPath " << std::endl;
        return -1;
    }

    try {
        if (!strcmp("-p", argv[1])) {
            qDebug() << QString("START PACKING: ") + argv[2] + " " + argv[3] + "\n";
            Archiver::pack(argv[2], argv[3]);
        } else
            if (!strcmp("-u", argv[1])) {
                qDebug() << QString("START UNPACKING: ") + argv[2] + " " + argv[3] + "\n";
                Archiver::unpack(argv[2], argv[3]);
            } else
                if (!strcmp("-c", argv[1])) {
                    qDebug() << QString("START CHECKING: ") + argv[2] + " " + argv[3] + "\n";
                    checkArchiver(argv[2], argv[3]);
                } else {
                    std::cerr << "Unknown first argument in cmd!\nPlease print one of:\n\"-p sourcePath outputFileArchive\" to pack sourcePath to outputFileArchive" << std::endl <<
                                 "\"-u inputFileArchive outputPath\" to unpack inputFileArchive to outputPath" << std::endl <<
                                 "\"-c sourcePath outputPath\" to check archiver's work for sourcePath and outputPath " << std::endl;
                    return -1;
                }
    } catch (Archiver::ArchiverException e) {
        qCritical() << e.whatQMsg() << '\n';
    }

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

bool checkInode(inode* first, inode* second, const QString & dir1, const QString & dir2) {
    QString name1 = first->name;
    QString name2 = second->name;
    if (first->parent == NULL) {
        name1 = ArchiverUtils::getDirentName(first->name);
        name2 = ArchiverUtils::getDirentName(second->name);
    }

    if (name1 != name2) {
        qCritical() << dir1 + name1 << "!=" << dir2 + name2 << '\n';
        return false;
    }

    if (first->type != second->type) {
        qCritical() << dir1 + name1 << ": different types" << '\n';
        return false;
    }

    if (first->attrs.st_atime != second->attrs.st_atime || first->attrs.st_mtime != second->attrs.st_mtime ||
            first->attrs.st_mode != second->attrs.st_mode || first->attrs.st_uid != second->attrs.st_uid ||
            first->attrs.st_gid != second->attrs.st_gid) {
        qCritical() << dir1 + name1 << ": different attrs" << '\n';
        return false;
    }

    if (first->type == INODE_REG_FILE) {
        if (first->attrs.st_size != second->attrs.st_size)
        {
            qCritical() << dir1 + name1 << ": different size" << '\n';
            return false;
        }

        QFile file1(dir1 + name1);
        file1.open(QIODevice::ReadOnly);
        QFile file2(dir2 + name2);
        file2.open(QIODevice::ReadOnly);
        QByteArray bytes1 = file1.readAll();
        QByteArray bytes2 = file2.readAll();
        if (memcmp(bytes1.data(), bytes2.data(), first->attrs.st_size)) {
            qCritical() << dir1 + name1 << ": different content" << '\n';
            return false;
        }
    }

    if (first->type == INODE_DIR) {
        dir_inode* first_dir = reinterpret_cast<dir_inode*>(first);
        dir_inode* second_dir = reinterpret_cast<dir_inode*>(second);
        if (first_dir->num_children != second_dir->num_children) {
            qCritical() << dir1 + name1 << ": different numchildren" << '\n';
            return false;
        }

        for (std::uint64_t i = 0; i < first_dir->num_children; ++i) {
            if (!checkInode(first_dir->children[i], second_dir->children[i],
                             dir1 + name1 + QDir::separator(), dir2 + name2 + QDir::separator()) ) {
                return false;
            }
        }

    }

    return true;
}

void checkArchiver(QString dir1, QString dir2) {
    fs_tree* tree1 = fs_tree_collect(dir1.toStdString().c_str());
    fs_tree* tree2 = fs_tree_collect(dir2.toStdString().c_str());

    dir1 = ArchiverUtils::getDirAbsPath(dir1);
    dir2 = ArchiverUtils::getDirAbsPath(dir2);

    if (checkInode(tree1->head, tree2->head, dir1, dir2))
        std::cout << "Ok!" << std::endl;
    else
        std::cout << "Bad archiver..." << std::endl;
}
