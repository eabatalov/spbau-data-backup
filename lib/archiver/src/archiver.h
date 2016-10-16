#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <QTextStream>
#include <QString>

class Archiver
{
public:
    static void pack(const QString & srcPath, const QString & dstArchivePath);
    static void unpack(const QString & srcArchivePath, const QString & dstPath);
    static void printArchiveFsTree(const QString & srcArchivePath, QTextStream & qTextStream);

private:
    Archiver();
};

#endif // ARCHIVER_H
