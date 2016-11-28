#include "clientsessiononserver.h"
#include "serverclientmanager.h"
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
#include <QThread>

#define DEBUG_FLAG true
#define LOG(format, var) do{ \
    if (DEBUG_FLAG) fprintf(stderr, "line: %d.  " format, __LINE__ ,var); \
    }while(0)

ClientSessionOnServer::ClientSessionOnServer(ServerClientManager* serverClientManager, size_t clientNumber, QTcpSocket* clientSocket, QObject *parent)
    :QObject(parent)
    ,mSessionNumber(clientNumber)
    ,serverClientManager(serverClientManager)
    ,mPerClientState(NOT_AUTORIZATED){
    NetworkStream* networkStream = new NetworkStream(clientSocket, this);
    connect(networkStream, &NetworkStream::newNetMessage, this, &ClientSessionOnServer::onNetworkInput);
    connect(this, &ClientSessionOnServer::sigSendClientMessage, networkStream, &NetworkStream::sendNetMessage);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ClientSessionOnServer::disconnectSocket);
}


void ClientSessionOnServer::onNetworkInput(const QByteArray &message) {
    utils::commandType cmd = *((utils::commandType*)message.data());
    switch (cmd) {
    case utils::clientLogin:
        procLoginRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ls:
        procLsRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::restore:
        procRestoreRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::replyAfterRestore:
        procReplyAfterRestore(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::backup:
        procBackupRequest(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::clientExit:
        procClientExit();
        break;
    default:
        std::cerr << "Unsupported command from client." << std::endl;
    }

}

void ClientSessionOnServer::procLoginRequest(const char* buffer, std::uint64_t bufferSize) {
    if (mPerClientState != NOT_AUTORIZATED) {
        std::cerr << "Error with curstate. Unexpected LoginRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected LoginRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::LoginClientRequest loginRequest;
    loginRequest.ParseFromArray(buffer, bufferSize);

    AuthStruct authStruct;
    authStruct.login = loginRequest.login();
    authStruct.sessionId = mSessionNumber;

    userDataHolder = serverClientManager->tryAuth(authStruct);
    if (userDataHolder == NULL) {
        sendAnsToClientLogin(false);
    } else {
        mPerClientState = NORMAL;

        std::string mLoginStdString = userDataHolder->getLogin().toStdString();
        std::cout << "connected user: " << mLoginStdString  << std::endl;

        sendAnsToClientLogin(true);
    }

}

void ClientSessionOnServer::sendAnsToClientLogin(bool result) {
    networkUtils::protobufStructs::LoginServerAnswer serverAnswer;
    serverAnswer.set_issuccess(result);

    std::string serializated;
    if (!serverAnswer.SerializeToString(&serializated)) {
        std::cerr << "Error with serialization LSDetailed." << std::endl;
        sendServerError("Ooops... Error with serialization LoginServerAnswer.");
        return;
    }
    sendSerializatedMessage(serializated, utils::ansToClientLogin, serverAnswer.ByteSize());
}

void ClientSessionOnServer::procLsRequest(const char *buffer, uint64_t bufferSize) {
    if (mPerClientState != NORMAL) {
        std::cerr << "Error with curstate. Unexpected LSRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected LSRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::LsClientRequest lsRequest;
    lsRequest.ParseFromArray(buffer, bufferSize);

    if (lsRequest.has_backupid()) {
        if (userDataHolder->isValidBackupId(lsRequest.backupid())) {
            networkUtils::protobufStructs::LsDetailedServerAnswer lsDetailed;

            QByteArray metaInQByteArray;
            try {
                userDataHolder->setArchiveWithoutContent((std::uint64_t)(lsRequest.backupid()), metaInQByteArray);
            } catch (Archiver::ArchiverException e) {
                qCritical() << e.whatQMsg() << '\n';
            }

            if (metaInQByteArray.size() == 0) {
                sendServerError("Ooops...");
            } else {
                lsDetailed.set_meta(metaInQByteArray.data(), metaInQByteArray.size());
                networkUtils::protobufStructs::ShortBackupInfo* backupInfo = new networkUtils::protobufStructs::ShortBackupInfo();
                backupInfo->set_backupid(lsRequest.backupid());
                backupInfo->set_path(userDataHolder->getOriginalPath(lsRequest.backupid()));
                backupInfo->set_time(userDataHolder->getCreationTime(lsRequest.backupid()));
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
        std::uint64_t backupsNumber = userDataHolder->getBackupsNumber();
        for (uint64_t i = 0; i < backupsNumber; ++i) {
            backupInfo = lsSummary.add_shortbackupinfo();
            backupInfo->set_backupid(i);
            backupInfo->set_path(userDataHolder->getOriginalPath(i));
            backupInfo->set_time(userDataHolder->getCreationTime(i));
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

void ClientSessionOnServer::procRestoreRequest(const char *bufferbuffer, uint64_t bufferSize) {
    if (mPerClientState != NORMAL) {
        std::cerr << "Error with curstate. Unexpected RestoreRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected RestoreRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    LOG("Receive restore request. Size = %ld\n", bufferSize);
    networkUtils::protobufStructs::RestoreClientRequest restoreRequest;
    restoreRequest.ParseFromArray(bufferbuffer, bufferSize);

    if (userDataHolder->isValidBackupId(restoreRequest.backupid())) {
        mRestoreBackupId = restoreRequest.backupid();

        QByteArray archive;
        userDataHolder->setArchive(mRestoreBackupId, archive);

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

void ClientSessionOnServer::procReplyAfterRestore(const char *bufferbuffer, uint64_t bufferSize) {
    if (mPerClientState != WAIT_RESTORE_RESULT) {
        std::cerr << "Error with curstate. Unexpected ReplyAfterRestore" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected ReplyAfterRestore.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }

    networkUtils::protobufStructs::ClientReplyAfterRestore replyAfterRestore;
    replyAfterRestore.ParseFromArray(bufferbuffer, bufferSize);

    if (replyAfterRestore.issuccess()) {
        userDataHolder->incSuccRestoreCount(mRestoreBackupId);
    } else {
        userDataHolder->incFailRestoreCount(mRestoreBackupId);
    }
    mPerClientState = NORMAL;
}

void ClientSessionOnServer::procBackupRequest(const char *bufferbuffer, uint64_t bufferSize) {
    if (mPerClientState != NORMAL) {
        std::cerr << "Error with curstate. Unexpected BackupRequest" << std::endl;
        sendServerExit("Oooops. Server exit because of error with curstate. Unexpected BackupRequest.");
        mPerClientState = ABORTED;
        disconnectSocket();
    }
    networkUtils::protobufStructs::ClientBackupRequest backupRequest;
    backupRequest.ParseFromArray(bufferbuffer, bufferSize);

    bool isSuc;
    std::uint64_t backupid;

    userDataHolder->addNewBackup(backupRequest.archive().c_str(), backupRequest.archive().size(), backupRequest.path(), isSuc, backupid);

    networkUtils::protobufStructs::ServerBackupResult backupResult;
    backupResult.set_backupid(backupid);
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

void ClientSessionOnServer::procClientExit() {
    LOG("onclientExit = %ld\n", mSessionNumber);
}

void ClientSessionOnServer::disconnectSocket() {
    emit releaseClientPlace(mSessionNumber);
    this->deleteLater();
}
