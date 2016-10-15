#include "commandlinemanager.h"
#include <archiver.h>
#include <iostream>

#include <QTextStream>
#include <QCommandLineParser>
#include <QCommandLineOption>

const QString CommandLineManager::inputOption = QString("input");
const QString CommandLineManager::outputOption = QString("output");

CommandLineManager::CommandLineManager(QCoreApplication &app, QObject *parent) : QObject(parent)
{
    std::cerr << std::endl << "inputOption.toStdString()" << " : " << inputOption.toStdString() << std::endl;
    std::cerr << std::endl << "outputOption.toStdString()" << " : " << outputOption.toStdString() << std::endl;

    parser.setApplicationDescription("Command line archiver provide function to pack, unpack and list archive content\n"
                                     "Examples of usage:\n"
                                     "\"pack -i sourcePath -o outputFileArchive\" to pack sourcePath to outputFileArchive\n"
                                     "\"unpack -i inputFileArchive -o outputPath\" to unpack inputFileArchive to outputPath\n"
                                     "\"list -i ArchiveFile\" to check list fs_tree of archive data.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("action", "The action to be performed");

    std::cerr << "Add inputOption: "
              << parser.addOption(QCommandLineOption({"i", "input"}, "Input directory or archive (depends from action).", "PATH"))
              << std::endl;
    std::cerr << "Add outputOption: "
              << parser.addOption(QCommandLineOption({"o", "output"}, "Output directory or archive (depends from action).", "PATH"))
              << std::endl;
    parser.process(app);
}

void CommandLineManager::process() {

    std::cerr << std::endl << "inputOption.toStdString()" << " : " << inputOption.toStdString() << std::endl;
    std::cerr << std::endl << "outputOption.toStdString()" << " : " << outputOption.toStdString() << std::endl;

    std::cerr << "parser.optionNames().size() = " << parser.optionNames().size() << std::endl;

    std::cerr << "parser.optionNames().at(0).toStdString() = " << parser.optionNames().at(0).toStdString() << std::endl;

    if (parser.positionalArguments().at(0) == QString("pack"))
    {
        if (parser.isSet(inputOption) && parser.isSet(outputOption))
            Archiver::pack(parser.value(inputOption).toStdString(), parser.value(outputOption).toStdString());
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
            QTextStream qTextStream(stdout);
            Archiver::printArchiveFsTree(parser.value(inputOption).toStdString().c_str(), qTextStream);
        }
        else
            std::cerr << "Wrong options with list action." << std::endl;
    } else
    {
        std::cerr << "Unexpected action." << std::endl;
    }
}

