#ifndef USERDATAHOLDER_H
#define USERDATAHOLDER_H

#include <QMutex>
#include <QString>

#include <string>

#include <serverStructs.pb.h>

class UserDataHolder
{
public:
    UserDataHolder(QString login);
    QString getLogin();

    void loadMetadatasFromFile();
    bool isValidBackupId(std::uint64_t backupId);
    std::uint64_t getBackupsNumber();
    void incSuccRestoreCount(std::uint64_t backupId);
    void incFailRestoreCount(std::uint64_t backupId);
    void addNewBackup(const char* backup, std::uint64_t backupSize, std::string originalPath, bool & isSuc, std::uint64_t & backupId);

    std::string getOriginalPath(std::uint64_t backupId);
    std::uint64_t getCreationTime(std::uint64_t backupId);
    std::uint64_t getFailRestoreCount(std::uint64_t backupId);
    std::uint64_t getSuccRestoreCount(std::uint64_t backupId);
    void setArchiveWithoutContent(std::uint64_t backupId, QByteArray& archiveWithoutContent);
    void setArchive(std::uint64_t backupId, QByteArray& archive);
    void initMutex();
    void deleteMutex();

private:
    QMutex* mutexOnState;
    QString login;
    serverUtils::protobufStructs::VectorOfServerMetadataForArchive mMetadatas;
    std::uint64_t newBackupId;

    std::string getPathToUsermetas();
    QString getPathByBackupId(std::uint64_t backupId);
    bool saveStateMetadatas();
};

#endif // USERDATAHOLDER_H
