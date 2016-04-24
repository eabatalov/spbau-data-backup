#ifndef ARCHIVER_H
#define ARCHIVER_H

class Archiver
{
public:
    static void pack(const char* srcPath, const char* dstArchivePath);
    static void unpack(const char* srcArchivePath, const char* dstPath);

private:
    Archiver();
};

#endif // ARCHIVER_H
