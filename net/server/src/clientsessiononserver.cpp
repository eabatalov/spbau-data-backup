#include "clientsessiononserver.h"
#include <networkMsgStructs.pb.h>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <QFile>
#include <QByteArray>

#define DEBUG_FLAG false
#define LOG(format, var) do{ \
    if (DEBUG_FLAG) fprintf(stderr, "line: %d.  " format, __LINE__ ,var); \
    }while(0)

std::string inttostr(std::uint64_t number)
{
    if (number == 0)
        return "0";
    std::string S = "";
    while (number > 0)
    {
        S = std::string(1, '0'+(char)(number % 10)) + S;
        number /= 10;
    }
    return S;
}

ClientSessionOnServer::ClientSessionOnServer(size_t clientNumber, QTcpSocket* clientSocket, QObject *parent)
    :QObject(parent)
    ,mClientNumber(clientNumber)
    ,newBackupId(0)
    ,mPerClientState(NOT_AUTORIZATED)
{
    mNetworkStream = new NetworkStream(clientSocket, this);
    connect(mNetworkStream, &NetworkStream::newNetMessage, this, &ClientSessionOnServer::onNetworkInput);
    connect(this, &ClientSessionOnServer::sigSendClientMessage, mNetworkStream, &NetworkStream::sendNetMessage);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ClientSessionOnServer::disconnectSocket);


    //TODO: Fix it
    mPerClientState = NORMAL;
    mLogin = "example";


    std::fstream metadatasFile("users/" + mLogin + std::string(".usermetas"), std::ios::in | std::ios::binary);

    if (metadatasFile.is_open())
    {
        if (!mMetadatas.ParseFromIstream(&metadatasFile)) {
              std::cerr << "Failed to read metas info." << std::endl;
        }
        newBackupId = mMetadatas.metadatas_size();
    } else
    {
        std::fstream newMetadatasFile("users/" + mLogin + std::string(".usermetas"), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!mMetadatas.SerializeToOstream(&newMetadatasFile)) {
              std::cerr << "Failed to create metas info file." << std::endl;
        }
    }
}

void ClientSessionOnServer::onNetworkInput(const QByteArray &message)
{
    utils::commandType cmd = *((utils::commandType*)message.data());
    switch (cmd)
    {
    case utils::ls:
        onLsRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::restore:
        onRestoreRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::replyAfterRestore:
        onReplyAfterRestore(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::backup:
        onBackupRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::clientExit:
        onclientExit(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    default:
        std::cerr << "Unsupported command from server." << std::endl;
    }

}

void ClientSessionOnServer::onLsRequest(const char *buffer, uint64_t bufferSize)
{
    networkUtils::protobufStructs::LsClientRequest lsRequest;
    //TODO: manage parsing error
    lsRequest.ParseFromArray(buffer, bufferSize);

    if (lsRequest.has_backupid())
    {
        if (isValidBackupId(lsRequest.backupid()))
        {
            networkUtils::protobufStructs::LsDetailedServerAnswer lsDetailed;
            lsDetailed.set_meta("some meta"); //TODO: fix it
            networkUtils::protobufStructs::ShortBackupInfo* backupInfo = new networkUtils::protobufStructs::ShortBackupInfo;
            backupInfo->set_backupid(mMetadatas.metadatas(lsRequest.backupid()).id());
            backupInfo->set_path(mMetadatas.metadatas(lsRequest.backupid()).origianlpath());
            backupInfo->set_time(mMetadatas.metadatas(lsRequest.backupid()).creationtime());
            lsDetailed.set_allocated_shortbackupinfo(backupInfo);

            std::string serializated;
            if (!lsDetailed.SerializeToString(&serializated))
            {
                std::cerr << "Error with serialization LSDetailed." << std::endl;
                return;
            }
            sendSerializatedMessage(serializated, utils::ansToLsDetailed, lsDetailed.ByteSize());
        }
        else
        {
            //TODO: fix it
            std::cerr << "Unsupported backupid" << std::endl;
        }
    }else
    {
        networkUtils::protobufStructs::LsSummaryServerAnswer lsSummary;
        networkUtils::protobufStructs::ShortBackupInfo* backupInfo = 0;
        for (int i = 0; i < mMetadatas.metadatas_size(); ++i)
        {
            backupInfo = lsSummary.add_shortbackupinfo();
            backupInfo->set_backupid(mMetadatas.metadatas(i).id());
            backupInfo->set_path(mMetadatas.metadatas(i).origianlpath());
            backupInfo->set_time(mMetadatas.metadatas(i).creationtime());
        }

        std::string serializated;
        if (!lsSummary.SerializeToString(&serializated))
        {
            std::cerr << "Error with serialization LSSummary." << std::endl;
            return;
        }

        sendSerializatedMessage(serializated, utils::ansToLsSummary, lsSummary.ByteSize());
    }

}

void ClientSessionOnServer::sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize)
{
    std::vector<char> message(messageSize + utils::commandSize);
    std::memcpy(message.data(), (char*)&cmdType, utils::commandSize);
    std::memcpy(message.data() + utils::commandSize, binaryMessage.c_str(), messageSize);
    emit sigSendClientMessage(QByteArray(message.data(), messageSize + utils::commandSize));
}

void ClientSessionOnServer::onRestoreRequest(const char *bufferbuffer, uint64_t bufferSize)
{
    LOG("Receive restore request. Size = %ld\n", bufferSize);
    networkUtils::protobufStructs::RestoreClientRequest restoreRequest;
    restoreRequest.ParseFromArray(bufferbuffer, bufferSize);

    if (isValidBackupId(restoreRequest.backupid()))
    {
        std::string path = "backups/" + mLogin + "_" + inttostr((std::uint64_t)(restoreRequest.backupid()));
        mRestoreBackupId = restoreRequest.backupid();

        QFile metaFile((path + "meta").c_str());
        QFile contFile((path + "cont").c_str());
        metaFile.open(QIODevice::ReadOnly);
        contFile.open(QIODevice::ReadOnly);
        QByteArray meta = metaFile.readAll();
        QByteArray content = contFile.readAll();

        networkUtils::protobufStructs::RestoreServerAnswer restoreAnswer;
        restoreAnswer.set_meta(meta.data(), meta.size());
        restoreAnswer.set_archive(content.data(), content.size());
        std::string serializated;
        if (!restoreAnswer.SerializeToString(&serializated))
        {
            std::cerr << "Error with serialization restoreAnswer." << std::endl;
            return;
        }
        sendSerializatedMessage(serializated, utils::ansToRestore, restoreAnswer.ByteSize());
        mPerClientState = WAIT_RESTORE_RESULT;
        LOG("Send archive to restore. Size = %d\n", restoreAnswer.ByteSize());
        LOG("metasize = %ld\n", restoreAnswer.meta().size());
        LOG("archivesize = %ld\n", restoreAnswer.archive().size());
    }
    else
    {
        LOG("Invalid backupid. backupid = %ld\n", restoreRequest.backupid());
        //TODO fix it
    }
}

void ClientSessionOnServer::onReplyAfterRestore(const char *bufferbuffer, uint64_t bufferSize)
{
    if (mPerClientState == WAIT_RESTORE_RESULT)
    {
        networkUtils::protobufStructs::ClientReplyAfterRestore replyAfterRestore;
        replyAfterRestore.ParseFromArray(bufferbuffer, bufferSize);

        if (replyAfterRestore.issuccess())
        {
            mMetadatas.mutable_metadatas(mRestoreBackupId)->set_succrestorescnt(mMetadatas.mutable_metadatas(mRestoreBackupId)->succrestorescnt()+1);
            LOG("Success restores: %ld\n", mMetadatas.mutable_metadatas(mRestoreBackupId)->succrestorescnt());
        } else
        {
            mMetadatas.mutable_metadatas(mRestoreBackupId)->set_failrestorescnt(mMetadatas.mutable_metadatas(mRestoreBackupId)->failrestorescnt()+1);
            LOG("Failed restores: %ld\n", mMetadatas.mutable_metadatas(mRestoreBackupId)->failrestorescnt());
        }
        saveStateMetadatas();
        mPerClientState = NORMAL;
    }
    else
    {
        std::cerr << "Unexpected restore result" << std::endl;
    }
}

void initServerMetadata(serverUtils::protobufStructs::ServerMetadataForArchive* newServerMetadata,
                        std::uint64_t backupId, const std::string& originalPath, time_t creationTime)
{
    newServerMetadata->set_id(backupId++);
    newServerMetadata->set_origianlpath(originalPath);
    newServerMetadata->set_creationtime(creationTime);
    newServerMetadata->set_succrestorescnt(0);
    newServerMetadata->set_failrestorescnt(0);
}

void ClientSessionOnServer::onBackupRequest(const char *bufferbuffer, uint64_t bufferSize)
{
    networkUtils::protobufStructs::ClientBackupRequest backupRequest;
    backupRequest.ParseFromArray(bufferbuffer, bufferSize);
    serverUtils::protobufStructs::ServerMetadataForArchive* newServerMetadata = mMetadatas.add_metadatas();
    initServerMetadata(newServerMetadata, newBackupId++, backupRequest.path(), std::time(0));

    bool isSuc = true;

    std::string path = "backups/" + mLogin + "_" + inttostr((std::uint64_t)(newServerMetadata->id()));
    std::fstream archiveMeta(path + std::string("meta"), std::ios::out | std::ios::trunc | std::ios::binary);
    std::fstream archiveContent(path + std::string("cont"), std::ios::out | std::ios::trunc | std::ios::binary);
    if (archiveMeta.is_open() && archiveContent.is_open())
    {
        archiveContent.write(backupRequest.archive().c_str(), backupRequest.archive().size());
        archiveMeta.write(backupRequest.meta().c_str(), backupRequest.meta().size());
    }
    else
    {
        isSuc = false;
        std::cerr << "Error with creation archive files." << std::endl;
    }

    if (isSuc)
    {
        isSuc = saveStateMetadatas();
    }
    else
    {
        std::fstream metadatasFile("users/" + mLogin + std::string(".usermetas"), std::ios::in | std::ios::binary);
        if (!mMetadatas.ParseFromIstream(&metadatasFile)) {
              std::cerr << "Failed to read metas info." << std::endl;
        }
    }

    networkUtils::protobufStructs::ServerBackupResult backupResult;
    backupResult.set_backupid(newServerMetadata->id());
    backupResult.set_issuccess(isSuc);

    std::string serializated;
    if (!backupResult.SerializeToString(&serializated))
    {
        std::cerr << "Error with serialization backupResult." << std::endl;
        return;
    }
    sendSerializatedMessage(serializated, utils::ansToBackup, backupResult.ByteSize());

    LOG("metasize = %ld\n", backupRequest.meta().size());
    LOG("archivesize = %ld\n", backupRequest.archive().size());
}

void ClientSessionOnServer::onclientExit(const char *bufferbuffer, uint64_t bufferSize)
{
    //TODO
}

void ClientSessionOnServer::disconnectSocket()
{
    emit releaseClientPlace(mClientNumber);
    this->deleteLater();
}

bool ClientSessionOnServer::isValidBackupId(std::uint64_t backupId)
{
    LOG("backupId = %ld \t", backupId);
    LOG("newBackupId = %ld\n", newBackupId);
    if (backupId < newBackupId)
        return true;
    else
        return false;
}

bool ClientSessionOnServer::saveStateMetadatas()
{
    bool isSuc = true;
    std::fstream metadatasFile("users/" + mLogin + std::string(".usermetas"), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!mMetadatas.SerializeToOstream(&metadatasFile)) {
          std::cerr << "Failed to rewrite metas info file." << std::endl;
          isSuc = false;
    }
    return isSuc;
}

