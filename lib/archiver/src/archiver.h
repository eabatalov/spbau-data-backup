#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <QTextStream>

class Archiver
{
public:
    static void pack(const std::string srcPath, const std::string dstArchivePath);
    static void unpack(const std::string srcArchivePath, const std::string dstPath);
    static void printArchiveFsTree(const std::string srcArchivePath, QTextStream & qTextStream);

private:
    Archiver();
};

#endif // ARCHIVER_H
