#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <QTextStream>
#include <QString>
#include <QException>

class Archiver {
public:
    static void pack(const QString & srcPath, const QString & dstArchivePath);
    static void unpack(const QString & srcArchivePath, const QString & dstPath);
    static void printArchiveFsTree(const QString & srcArchivePath, QTextStream & qTextStream);
    static QByteArray getArchiveWithoutContent(const QString & srcArchivePath);

    class ArchiverException : public QException {
    public:
        void raise() const { throw *this; }
        ArchiverException *clone() const { return new ArchiverException(*this); }
        ArchiverException(QString message) : message(message) {}
        QString whatQMsg() const { return message; }

    private:
        QString message;
        ArchiverException() {}

    };

private:
    Archiver() {}
};

#endif // ARCHIVER_H
