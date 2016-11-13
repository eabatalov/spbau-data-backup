#include "clientsessiononserver.h"
#include <archiver.h>
#include <networkMsgStructs.pb.h>
#include <struct_serialization.pb.h>

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <QFile>
#include <QByteArray>
#include <QDebug>

#define DEBUG_FLAG true
#define LOG(format, var) do{ \
    if (DEBUG_FLAG) fprintf(stderr, "line: %d.  " format, __LINE__ ,var); \
    }while(0)

ClientSessionOnServer::ClientSessionOnServer(size_t clientNumber, QTcpSocket* clientSocket, QObject *parent)
    :QObject(parent)
    ,mClientNumber(clientNumber)
    ,newBackupId(0)
    ,mPerClientState(NOT_AUTORIZATED){
    mNetworkStream = new NetworkStream(clientSocket, this);
    connect(mNetworkStream, &NetworkStream::newNetMessage, this, &ClientSessionOnServer::onNetworkInput);
    connect(this, &ClientSessionOnServer::sigSendClientMessage, mNetworkStream, &NetworkStream::sendNetMessage);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ClientSessionOnServer::disconnectSocket);
}


void ClientSessionOnServer::onNetworkInput(const QByteArray &message) {
    utils::commandType cmd = *((utils::commandType*)message.data());
    switch (cmd) {
    case utils::clientLogin:
        onLoginRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
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
        onclientExit();
        break;
    default:
        std::cerr << "Unsupported command from client." << std::endl;
    }

}

void ClientSessionOnServer::onLoginRequest(const char* buffer, std::uint64_t bufferSize) {
    if (mPerClientState != NOT_AUTORIZATED) {
        std::cerr << "Error with curstate. Unexpected LoginRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected LoginRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::LoginClientRequest loginRequest;
    loginRequest.ParseFromArray(buffer, bufferSize);
    mPerClientState = NORMAL;
    mLogin = QString::fromStdString(loginRequest.login());

    std::cout << "connected user: " << loginRequest.login() << std::endl;

    std::string mLoginStdString = mLogin.toStdString();
    std::fstream metadatasFile("users/" + mLoginStdString + ".usermetas", std::ios::in | std::ios::binary);

    if (metadatasFile.is_open()) {
        if (!mMetadatas.ParseFromIstream(&metadatasFile)) {
              std::cerr << "Failed to read metas info." << std::endl;
        }
        newBackupId = mMetadatas.metadatas_size();
    } else {
        std::fstream newMetadatasFile("users/" + mLoginStdString + std::string(".usermetas"), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!mMetadatas.SerializeToOstream(&newMetadatasFile)) {
              std::cerr << "Failed to create metas info file." << std::endl;
        }
    }

    networkUtils::protobufStructs::LoginServerAnswer serverAnswer;
    serverAnswer.set_issuccess(true);

    std::string serializated;
    if (!serverAnswer.SerializeToString(&serializated)) {
        std::cerr << "Error with serialization LSDetailed." << std::endl;
        sendServerError("Ooops... Error with serialization LoginServerAnswer.");
        return;
    }
    sendSerializatedMessage(serializated, utils::ansToClientLogin, serverAnswer.ByteSize());
}

void ClientSessionOnServer::onLsRequest(const char *buffer, uint64_t bufferSize) {
    if (mPerClientState != NORMAL) {
        std::cerr << "Error with curstate. Unexpected LSRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected LSRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::LsClientRequest lsRequest;
    lsRequest.ParseFromArray(buffer, bufferSize);

    if (lsRequest.has_backupid()) {
        if (isValidBackupId(lsRequest.backupid())) {
            networkUtils::protobufStructs::LsDetailedServerAnswer lsDetailed;
            QString path = "backups/" + mLogin + "_" + QString::number((std::uint64_t)(lsRequest.backupid()));

            QByteArray metaInQByteArray;
            try {
                metaInQByteArray = Archiver::getArchiveWithoutContent(path);
            } catch (Archiver::ArchiverException e) {
                qCritical() << e.whatQMsg() << '\n';
            }

            if (metaInQByteArray.size() == 0) {
                sendServerError("Ooops...");
            } else {
                lsDetailed.set_meta(metaInQByteArray.data(), metaInQByteArray.size());
                networkUtils::protobufStructs::ShortBackupInfo* backupInfo = new networkUtils::protobufStructs::ShortBackupInfo();
                backupInfo->set_backupid(mMetadatas.metadatas(lsRequest.backupid()).id());
                backupInfo->set_path(mMetadatas.metadatas(lsRequest.backupid()).origianlpath());
                backupInfo->set_time(mMetadatas.metadatas(lsRequest.backupid()).creationtime());
                lsDetailed.set_allocated_shortbackupinfo(backupInfo);

                std::string serializated;
                if (!lsDetailed.SerializeToString(&serializated)) {
                    std::cerr << "Error with serialization LSDetailed." << std::endl;
                    sendServerError("Ooops... Error with serialization LSDetailed.");
                    return;
                }
                sendSerializatedMessage(serializated, utils::ansToLsDetailed, lsDetailed.ByteSize());
            }
        } else {
            sendNotFoundBackupIdToClient(lsRequest.backupid());
            std::cerr << "Unsupported backupid" << std::endl;
        }
    } else {
        networkUtils::protobufStructs::LsSummaryServerAnswer lsSummary;
        networkUtils::protobufStructs::ShortBackupInfo* backupInfo = 0;
        for (int i = 0; i < mMetadatas.metadatas_size(); ++i) {
            backupInfo = lsSummary.add_shortbackupinfo();
            backupInfo->set_backupid(mMetadatas.metadatas(i).id());
            backupInfo->set_path(mMetadatas.metadatas(i).origianlpath());
            backupInfo->set_time(mMetadatas.metadatas(i).creationtime());
        }

        std::string serializated;
        if (!lsSummary.SerializeToString(&serializated)) {
            std::cerr << "Error with serialization LSSummary." << std::endl;
            sendServerError("Ooops... Error with serialization LSSummary.");
            return;
        }

        sendSerializatedMessage(serializated, utils::ansToLsSummary, lsSummary.ByteSize());
    }

}

void ClientSessionOnServer::sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize) {
    std::vector<char> message(messageSize + utils::commandSize);
    std::memcpy(message.data(), (char*)&cmdType, utils::commandSize);
    std::memcpy(message.data() + utils::commandSize, binaryMessage.c_str(), messageSize);
    emit sigSendClientMessage(QByteArray(message.data(), messageSize + utils::commandSize));
}

void ClientSessionOnServer::onRestoreRequest(const char *bufferbuffer, uint64_t bufferSize) {
    if (mPerClientState != NORMAL) {
        std::cerr << "Error with curstate. Unexpected RestoreRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected RestoreRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    LOG("Receive restore request. Size = %ld\n", bufferSize);
    networkUtils::protobufStructs::RestoreClientRequest restoreRequest;
    restoreRequest.ParseFromArray(bufferbuffer, bufferSize);

    if (isValidBackupId(restoreRequest.backupid())) {
        QString path = "backups/" + mLogin + "_" + QString::number((std::uint64_t)(restoreRequest.backupid()));
        mRestoreBackupId = restoreRequest.backupid();

        QFile metaFile(path);
        metaFile.open(QIODevice::ReadOnly);
        QByteArray archive = metaFile.readAll();

        networkUtils::protobufStructs::RestoreServerAnswer restoreAnswer;
        restoreAnswer.set_archive(archive.data(), archive.size());
        std::string serializated;
        if (!restoreAnswer.SerializeToString(&serializated)) {
            std::cerr << "Error with serialization restoreAnswer." << std::endl;
            sendServerError("Ooops... Error with serialization restoreAnswer.");
            return;
        }
        sendSerializatedMessage(serializated, utils::ansToRestore, restoreAnswer.ByteSize());
        mPerClientState = WAIT_RESTORE_RESULT;
        LOG("Send archive to restore. Size = %d\n", restoreAnswer.ByteSize());
        LOG("archivesize = %ld\n", restoreAnswer.archive().size());
    } else {
        LOG("Invalid backupid. backupid = %ld\n", restoreRequest.backupid());
        sendNotFoundBackupIdToClient(restoreRequest.backupid());
    }
}

void ClientSessionOnServer::sendServerError(QString message) {
    networkUtils::protobufStructs::ServerError serverError;
    serverError.set_errormessage(message.toStdString());
    std::string serializated;
    if (!serverError.SerializeToString(&serializated)) {
        std::cerr << "Error with serialization serverError." << std::endl;
        sendServerExit("Oooops. Server exit because of error with serialization serverError.");
        return;
    }
    sendSerializatedMessage(serializated, utils::serverError, serverError.ByteSize());
}

void ClientSessionOnServer::sendServerExit(QString message) {
    networkUtils::protobufStructs::ServerError serverError;
    serverError.set_errormessage(message.toStdString());
    std::string serializated;
    if (!serverError.SerializeToString(&serializated)) {
        std::cerr << "Error with serialization serverExit." << std::endl;
        disconnectSocket();
        return;
    }
    sendSerializatedMessage(serializated, utils::serverExit, serverError.ByteSize());
}

void ClientSessionOnServer::sendNotFoundBackupIdToClient(std::uint64_t backupId) {
    networkUtils::protobufStructs::ServerError serverError;
    serverError.set_errormessage((QString("Not found backupId on server: ") + QString::number(backupId)).toStdString());
    std::string serializated;
    if (!serverError.SerializeToString(&serializated)) {
        std::cerr << "Error with serialization serverError." << std::endl;
        sendServerExit("Oooops. Server exit because of error with serialization serverError.");
        return;
    }
    sendSerializatedMessage(serializated, utils::notFoundBackupByIdOnServer, serverError.ByteSize());
}

void ClientSessionOnServer::onReplyAfterRestore(const char *bufferbuffer, uint64_t bufferSize) {
    if (mPerClientState != WAIT_RESTORE_RESULT) {
        std::cerr << "Error with curstate. Unexpected ReplyAfterRestore" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected ReplyAfterRestore.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::ClientReplyAfterRestore replyAfterRestore;
    replyAfterRestore.ParseFromArray(bufferbuffer, bufferSize);

    if (replyAfterRestore.issuccess()) {
        mMetadatas.mutable_metadatas(mRestoreBackupId)->set_succrestorescnt(mMetadatas.mutable_metadatas(mRestoreBackupId)->succrestorescnt()+1);
        LOG("Success restores: %ld\n", mMetadatas.mutable_metadatas(mRestoreBackupId)->succrestorescnt());
    } else {
        mMetadatas.mutable_metadatas(mRestoreBackupId)->set_failrestorescnt(mMetadatas.mutable_metadatas(mRestoreBackupId)->failrestorescnt()+1);
        LOG("Failed restores: %ld\n", mMetadatas.mutable_metadatas(mRestoreBackupId)->failrestorescnt());
    }
    saveStateMetadatas();
    mPerClientState = NORMAL;
}

void initServerMetadata(serverUtils::protobufStructs::ServerMetadataForArchive* newServerMetadata,
                        std::uint64_t backupId, const std::string& originalPath, time_t creationTime) {
    newServerMetadata->set_id(backupId++);
    newServerMetadata->set_origianlpath(originalPath);
    newServerMetadata->set_creationtime(creationTime);
    newServerMetadata->set_succrestorescnt(0);
    newServerMetadata->set_failrestorescnt(0);
}

void ClientSessionOnServer::onBackupRequest(const char *bufferbuffer, uint64_t bufferSize) {
    if (mPerClientState != NORMAL) {
        std::cerr << "Error with curstate. Unexpected BackupRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected BackupRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::ClientBackupRequest backupRequest;
    backupRequest.ParseFromArray(bufferbuffer, bufferSize);
    serverUtils::protobufStructs::ServerMetadataForArchive* newServerMetadata = mMetadatas.add_metadatas();
    initServerMetadata(newServerMetadata, newBackupId++, backupRequest.path(), std::time(0));

    bool isSuc = true;

    std::string path = (QString("backups/") + mLogin + "_" + QString::number((std::uint64_t)(newServerMetadata->id()))).toStdString();

    std::fstream archive(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (archive.is_open()) {
        archive.write(backupRequest.archive().c_str(), backupRequest.archive().size());
    } else {
        isSuc = false;
        std::cerr << "Error with creation archive file." << std::endl;
    }

    if (isSuc) {
        isSuc = saveStateMetadatas();
    } else {
        std::fstream metadatasFile("users/" + mLogin.toStdString() + std::string(".usermetas"), std::ios::in | std::ios::binary);
        if (!mMetadatas.ParseFromIstream(&metadatasFile)) {
              std::cerr << "Failed to read metas info." << std::endl;
        }
    }

    networkUtils::protobufStructs::ServerBackupResult backupResult;
    backupResult.set_backupid(newServerMetadata->id());
    backupResult.set_issuccess(isSuc);

    std::string serializated;
    if (!backupResult.SerializeToString(&serializated)) {
        std::cerr << "Error with serialization backupResult." << std::endl;
        return;
    }
    sendSerializatedMessage(serializated, utils::ansToBackup, backupResult.ByteSize());

    LOG("archivesize = %ld\n", backupRequest.archive().size());
    mPerClientState = NORMAL;
}

void ClientSessionOnServer::onclientExit() {
    LOG("onclientExit = %ld\n", mClientNumber);
}

void ClientSessionOnServer::disconnectSocket() {
    emit releaseClientPlace(mClientNumber);
    this->deleteLater();
}

bool ClientSessionOnServer::isValidBackupId(std::uint64_t backupId) {
    LOG("Is backupId = %ld < ", backupId);
    LOG("newBackupId = %ld?\n", newBackupId);
    if (backupId < newBackupId)
        return true;
    else
        return false;
}

bool ClientSessionOnServer::saveStateMetadatas() {
    bool isSuc = true;
    std::fstream metadatasFile("users/" + mLogin.toStdString() + std::string(".usermetas"), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!mMetadatas.SerializeToOstream(&metadatasFile)) {
          std::cerr << "Failed to rewrite metas info file." << std::endl;
          isSuc = false;
    }
    return isSuc;
}

