#include "perclient.h"
#include <networkMsgStructs.pb.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

PerClient::PerClient(size_t clientNumber, QTcpSocket* clientSocket, QObject *parent)
    :QObject(parent)
    ,mClientNumber(clientNumber)
    ,mPerClientState(NOT_AUTORIZATED)
{
    mNetworkStream = new NetworkStream(clientSocket, this);
    connect(mNetworkStream, &NetworkStream::newNetMessage, this, &PerClient::slotNewNetMessege);
    connect(this, &PerClient::sigSendClientMessage, mNetworkStream, &NetworkStream::sendNetMessage);
    connect(clientSocket, &QTcpSocket::disconnected, this, &PerClient::disconnectSocket);


    //TODO: Fix it
    mPerClientState = NORMAL;
    mLogin = "example";
    serverUtils::protobufStructs::ServerMetadataForArchive* tempMetaData = mMetadatas.add_metadatas();
    tempMetaData->set_id(0);
    tempMetaData->set_origianlpath("/home/firstArchive");
    tempMetaData->set_creationtime(1462810822LL);
    tempMetaData->set_succrestorescnt(0);
    tempMetaData->set_failrestorescnt(0);

    tempMetaData = mMetadatas.add_metadatas();
    tempMetaData->set_id(1);
    tempMetaData->set_origianlpath("/home/secondArchive");
    tempMetaData->set_creationtime(1420070400LL);
    tempMetaData->set_succrestorescnt(10);
    tempMetaData->set_failrestorescnt(10);
}

void PerClient::slotNewNetMessege(const QByteArray &message)
{
    utils::commandType cmd = *((utils::commandType*)message.data());
    std::cerr << "Received msg. Size=" << message.size();
    std::cerr << ", cmd=" << (int)cmd << std::endl;
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

void PerClient::onLsRequest(const char *buffer, uint64_t bufferSize)
{
    networkUtils::protobufStructs::LsClientRequest lsRequest;
    //TODO: manage parsing error
    //lsRequest.ParseFromString(buffer);
    lsRequest.ParseFromArray(buffer, bufferSize);

    if (lsRequest.has_backupid())
    {
        if (lsRequest.backupid() < mMetadatas.metadatas_size())
        {
            networkUtils::protobufStructs::LsDetailedServerAnswer lsDetailed;
            lsDetailed.set_meta("some meta"); //TODO: fix it
            networkUtils::protobufStructs::ShortBackupInfo backupInfo;
            backupInfo.set_backupid(mMetadatas.metadatas(lsRequest.backupid()).id());
            backupInfo.set_path(mMetadatas.metadatas(lsRequest.backupid()).origianlpath());
            backupInfo.set_time(mMetadatas.metadatas(lsRequest.backupid()).creationtime());
            lsDetailed.set_allocated_shortbackupinfo(&backupInfo);

            std::string serializated;
            if (!lsDetailed.SerializeToString(&serializated))
            {
                std::cerr << "Error with serialization LSDetailed." << std::endl;
                return;
            }
            std::vector<char> message(lsDetailed.ByteSize() + utils::commandSize);
            utils::commandType cmd = utils::toFixedType(utils::ansToLsDetailed);
            std::memcpy(message.data(), (char*)&cmd, utils::commandSize);
            std::memcpy(message.data() + utils::commandSize, serializated.c_str(), lsDetailed.ByteSize());
            emit sigSendClientMessage(QByteArray(message.data(),lsDetailed.ByteSize()));

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
        std::vector<char> message(lsSummary.ByteSize() + utils::commandSize);
        utils::commandType cmd = utils::toFixedType(utils::ansToLsSummary);
        std::memcpy(message.data(), (char*)&cmd, utils::commandSize);
        std::memcpy(message.data() + utils::commandSize, serializated.c_str(), lsSummary.ByteSize());
        emit sigSendClientMessage(QByteArray(message.data(), lsSummary.ByteSize()));

    }

}

void PerClient::onRestoreRequest(const char *bufferbuffer, uint64_t bufferSize)
{
    //TODO
}

void PerClient::onReplyAfterRestore(const char *bufferbuffer, uint64_t bufferSize)
{
    //TODO
}

void PerClient::onBackupRequest(const char *bufferbuffer, uint64_t bufferSize)
{
    //TODO
}

void PerClient::onclientExit(const char *bufferbuffer, uint64_t bufferSize)
{
    //TODO
}

void PerClient::disconnectSocket()
{
    emit releaseClientPlace(mClientNumber);
    this->deleteLater();
}

