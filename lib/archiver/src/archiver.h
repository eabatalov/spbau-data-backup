#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <QTextStream>

class Archiver
{
public:
    static void pack(const char* srcPath, const char* dstArchivePath);
    static void unpack(const char* srcArchivePath, const char* dstPath);
    static void printArchiveFsTree(const char* srcArchivePath, QTextStream & qTextStream);

private:
    Archiver();
};

#endif // ARCHIVER_H
