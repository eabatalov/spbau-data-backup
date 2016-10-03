#include <QCoreApplication>
#include <archiver.h>
#include <iostream>
#include <string>
#include <cstring>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QCommandLineParser>

#include "struct_serialization.pb.h"
#include "archiver_utils.h"
#include <fs_tree.h>

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("cmd_archiver");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Command line archiver provide function to pack, unpack and list archive content\n"
                                     "Examples of usage:\n"
                                     "\"pack -i sourcePath -o outputFileArchive\" to pack sourcePath to outputFileArchive\n"
                                     "\"unpack -i inputFileArchive -o outputPath\" to unpack inputFileArchive to outputPath\n"
                                     "\"list -i ArchiveFile\" to check list fs_tree of archive data.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("action", "The action to be performed");

    QCommandLineOption inputOption(QStringList() << "i" << "input", "Input directory or archive (depends from action).",
                                   "input");
    // TODO use c++11 syntax and don't use *Option local variables. Use common option name constants.
    // parser.addOption({ "i", "input" }, "Input directory");
    parser.addOption(inputOption);

    QCommandLineOption outputOption(QStringList() << "o" << "output" , "Output directory or archive (depends from action).",
                                    "output");
    parser.addOption(outputOption);

    parser.process(app);

    if (parser.positionalArguments().at(0) == QString("pack"))
    {
        if (parser.isSet(inputOption) && parser.isSet(outputOption))
            // TODO fix usage of pointer returned from temporary object 
            Archiver::pack(parser.value(inputOption).toStdString().c_str(), parser.value(outputOption).toStdString().c_str());
        else
            std::cerr << "Too few options with pack action." << std::endl;
    } else
    if (parser.positionalArguments().at(0) == QString("unpack"))
    {
        if (parser.isSet(inputOption) && parser.isSet(outputOption))
            Archiver::unpack(parser.value(inputOption).toStdString().c_str(), parser.value(outputOption).toStdString().c_str());
        else
            std::cerr << "Too few options with unpack action." << std::endl;
    } else
    if (parser.positionalArguments().at(0) == QString("list"))
    {
        if (parser.isSet(inputOption) && !parser.isSet(outputOption))
        {
            QTextStream QTextStream(stdout);
            Archiver::printArchiveFsTree(parser.value(inputOption).toStdString().c_str(), QTextStream);
        }
        else
            std::cerr << "Wrong options with list action." << std::endl;
    } else
    {
        std::cerr << "Unexpected action." << std::endl;
    }

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
