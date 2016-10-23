#include "commandlinemanager.h"
#include <archiver.h>
#include <iostream>

#include <QTextStream>
#include <QCommandLineParser>
#include <QCommandLineOption>

const QString CommandLineManager::inputOption = QString("input");
const QString CommandLineManager::outputOption = QString("output");

CommandLineManager::CommandLineManager(QCoreApplication &app, QObject *parent) : QObject(parent) {
    parser.setApplicationDescription("Command line archiver provide function to pack, unpack and list archive content\n"
                                     "Examples of usage:\n"
                                     "\"pack -i sourcePath -o outputFileArchive\" to pack sourcePath to outputFileArchive\n"
                                     "\"unpack -i inputFileArchive -o outputPath\" to unpack inputFileArchive to outputPath\n"
                                     "\"list -i ArchiveFile\" to check list fs_tree of archive data.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("action", "The action to be performed");

    parser.addOption(QCommandLineOption({"i", "input"}, "Input directory or archive (depends from action).", "PATH"));
    parser.addOption(QCommandLineOption({"o", "output"}, "Output directory or archive (depends from action).", "PATH"));
    parser.process(app);
}

void CommandLineManager::process() {
    if (parser.positionalArguments().at(0) == QString("pack")) {
        if (parser.isSet(inputOption) && parser.isSet(outputOption))
            Archiver::pack(parser.value(inputOption), parser.value(outputOption));
        else
            std::cerr << "Too few options with pack action." << std::endl;
    } else
        if (parser.positionalArguments().at(0) == QString("unpack")) {
            if (parser.isSet(inputOption) && parser.isSet(outputOption))
                Archiver::unpack(parser.value(inputOption), parser.value(outputOption));
            else
                std::cerr << "Too few options with unpack action." << std::endl;
        } else
            if (parser.positionalArguments().at(0) == QString("list")) {
                if (parser.isSet(inputOption) && !parser.isSet(outputOption)) {
                    QTextStream qTextStream(stdout);
                    Archiver::printArchiveFsTree(parser.value(inputOption), qTextStream);
                } else
                    std::cerr << "Wrong options with list action." << std::endl;
            } else {
                std::cerr << "Unexpected action." << std::endl;
            }
}

