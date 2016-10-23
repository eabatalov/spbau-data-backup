#ifndef ARCHIVER_UTILS_H
#define ARCHIVER_UTILS_H

namespace ArchiverUtils {
    QString getDirentName(const QString &path);
    QString getDirAbsPath(const QString &path);
    const size_t byteSizeOfNumber = sizeof(std::uint64_t);
}


#endif // ARCHIVER_UTILS_H

