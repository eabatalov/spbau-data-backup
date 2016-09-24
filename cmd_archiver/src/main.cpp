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

    if (argc != 4 && argc != 3)
    {
        std::cerr << "Incorrect number of arguments in cmd!\nPlease print one of:\n\"-p sourcePath outputFileArchive\" to pack sourcePath to outputFileArchive" << std::endl <<
                     "\"-u inputFileArchive outputPath\" to unpack inputFileArchive to outputPath" << std::endl <<
                     "\"-l ArchiveFile\" to check list fs_tree of archive data." << std::endl;
        return -1;
    }

    if (argc == 4 && !strcmp("-p", argv[1]))
    {
        Archiver::pack(argv[2], argv[3]);
    }
    else if (argc == 4 && !strcmp("-u", argv[1]))
    {
        Archiver::unpack(argv[2], argv[3]);
    }
    else if (argc == 3 && !strcmp("-l", argv[1]))
    {
        QTextStream QTextStream(stdout);
        Archiver::printArchiveFsTree(argv[2], QTextStream);
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
