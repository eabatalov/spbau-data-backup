#include "userdataholder.h"

#include <archiver.h>

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>

#include <QFile>
#include <QFileInfo>
#include <QReadLocker>
#include <QWriteLocker>

UserDataHolder::UserDataHolder(QString login)
    :mLogin(login)
    ,mNewBackupId(0) { }

QString UserDataHolder::getLogin() {
    return mLogin;
}


//public (Almost all with mutex. Don't call another public methods in it)

void UserDataHolder::loadMetadatasFromFile() {
    QWriteLocker writeLocker(mReadWriteLock);
    std::fstream metadatasFile(getPathToUsermetas(), std::ios::in | std::ios::binary);

    if (metadatasFile.is_open()) {
        if (!mMetadatas.ParseFromIstream(&metadatasFile)) {
              std::cerr << "Failed to read metas info." << std::endl;
        }
        mNewBackupId = mMetadatas.metadatas_size();
    } else {
        std::fstream newMetadatasFile(getPathToUsermetas(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!mMetadatas.SerializeToOstream(&newMetadatasFile)) {
              std::cerr << "Failed to create metas info file." << std::endl;
        }
    }
}

std::uint64_t UserDataHolder::getBackupsNumber() {
    QReadLocker readLocker(mReadWriteLock);
    return mNewBackupId;
}

bool UserDataHolder::isValidBackupId(std::uint64_t backupId) {
    QReadLocker readLocker(mReadWriteLock);
    if (backupId < mNewBackupId)
        return true;
    else
        return false;
}

void UserDataHolder::fillArchiveWithoutContent(std::uint64_t backupId, QByteArray& archiveWithoutContent) {
    //nead here to lock?
    QReadLocker readLocker(mReadWriteLock);
    QString pathToArchive = getPathByBackupId(backupId);
    archiveWithoutContent = Archiver::getArchiveWithoutContent(pathToArchive);
}

void UserDataHolder::fillArchive(std::uint64_t backupId, QByteArray& archive) {
    //nead here to lock?
    QReadLocker readLocker(mReadWriteLock);
    QString pathToArchive = getPathByBackupId(backupId);
    QFile metaFile(pathToArchive);
    metaFile.open(QIODevice::ReadOnly);
    archive = metaFile.readAll();
}

std::string UserDataHolder::getOriginalPath(std::uint64_t backupId) {
    QReadLocker readLocker(mReadWriteLock);
    return mMetadatas.metadatas(backupId).origianlpath();
}

std::uint64_t UserDataHolder::getCreationTime(std::uint64_t backupId) {
    QReadLocker readLocker(mReadWriteLock);
    return mMetadatas.metadatas(backupId).creationtime();
}

std::uint64_t UserDataHolder::getFailRestoreCount(std::uint64_t backupId) {
    QReadLocker readLocker(mReadWriteLock);
    return mMetadatas.metadatas(backupId).failrestorescnt();
}

std::uint64_t UserDataHolder::getSuccRestoreCount(std::uint64_t backupId) {
    QReadLocker readLocker(mReadWriteLock);
    return mMetadatas.metadatas(backupId).succrestorescnt();
}


void UserDataHolder::incSuccRestoreCount(std::uint64_t backupId) {
    QWriteLocker writeLocker(mReadWriteLock);
    mMetadatas.mutable_metadatas(backupId)->set_succrestorescnt(mMetadatas.mutable_metadatas(backupId)->succrestorescnt()+1);
    if (!saveStateMetadatas()) {
        std::cerr << "Cannot save metadatas for " << mLogin.toStdString() << std::endl;
    }
}

void UserDataHolder::incFailRestoreCount(std::uint64_t backupId) {
    QWriteLocker writeLocker(mReadWriteLock);
    mMetadatas.mutable_metadatas(backupId)->set_failrestorescnt(mMetadatas.mutable_metadatas(backupId)->failrestorescnt()+1);
    if (!saveStateMetadatas()) {
        std::cerr << "Cannot save metadatas for " << mLogin.toStdString() << std::endl;
    }
}


void initServerMetadata(serverUtils::protobufStructs::ServerMetadataForArchive* newServerMetadata,
                        std::uint64_t backupId, const std::string& originalPath, time_t creationTime) {
    newServerMetadata->set_id(backupId++);
    newServerMetadata->set_origianlpath(originalPath);
    newServerMetadata->set_creationtime(creationTime);
    newServerMetadata->set_succrestorescnt(0);
    newServerMetadata->set_failrestorescnt(0);
}

void UserDataHolder::addNewBackup(const char* backup, std::uint64_t backupSize, std::string originalPath, bool & isSuc, std::uint64_t & backupId) {
    QWriteLocker writeLocker(mReadWriteLock);

    serverUtils::protobufStructs::ServerMetadataForArchive* newServerMetadata = mMetadatas.add_metadatas();
    isSuc = true;
    backupId = this->mNewBackupId++;
    initServerMetadata(newServerMetadata, backupId, originalPath, std::time(0));

    std::string path = getPathByBackupId(backupId).toStdString();

    std::fstream archive(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (archive.is_open()) {
        archive.write(backup, backupSize);
    } else {
        isSuc = false;
        std::cerr << "Error with creation archive file." << std::endl;
    }

    if (!saveStateMetadatas()) {
        std::cerr << "Cannot save metadatas for " << mLogin.toStdString() << std::endl;
    }
}

void UserDataHolder::initMutex() {
    mReadWriteLock = new QReadWriteLock();
}

void UserDataHolder::deleteMutex() {
    delete mReadWriteLock;
}



//private (without mutex)

bool UserDataHolder::saveStateMetadatas() {
    bool isSuc = true;
    std::fstream metadatasFile(getPathToUsermetas(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!mMetadatas.SerializeToOstream(&metadatasFile)) {
          std::cerr << "Failed to rewrite metas info file." << std::endl;
          isSuc = false;
    }
    return isSuc;
}

std::string UserDataHolder::getPathToUsermetas() {
    return "users/" + mLogin.toStdString() + std::string(".usermetas");
}

QString UserDataHolder::getPathByBackupId(std::uint64_t backupId) {
    return QString("backups/") + mLogin + "_" + QString::number(backupId);
}


