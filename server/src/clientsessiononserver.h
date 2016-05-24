#ifndef PERCLIENT_H
#define PERCLIENT_H

#include "../../echoclient/src/networkstream.h"
#include <networkstream.h>
#include <QTcpSocket>
#include <string>
#include <serverStructs.pb.h>

class ClientSessionOnServer:public QObject
{
    Q_OBJECT
public:
    ClientSessionOnServer(size_t clientNumber, QTcpSocket *clientSocket, QObject* parent = 0);

private:
    size_t mClientNumber;
    std::string mLogin;
    NetworkStream* mNetworkStream;
    serverUtils::protobufStructs::VectorOfServerMetadataForArchive mMetadatas;
    std::uint64_t newBackupId;
    std::uint64_t mRestoreBackupId;
    enum PerClientState{
        NOT_AUTORIZATED,
        NORMAL,
        WAIT_RESTORE_RESULT,
        ABORTED
    } mPerClientState;

    bool isValidBackupId(std::uint64_t backupId);
    void onLsRequest(const char* buffer, std::uint64_t bufferSize);
    void onRestoreRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void onReplyAfterRestore(const char* bufferbuffer, std::uint64_t bufferSize);
    void onBackupRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void onclientExit(const char* bufferbuffer, std::uint64_t bufferSize);
    void sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize);
    bool saveStateMetadatas();

signals:
    void releaseClientPlace(size_t clientNumber);
    void sigSendClientMessage(const QByteArray & message);

private slots:
    void onNetworkInput(const QByteArray & message);
    void disconnectSocket();
};

#endif // PERCLIENT_H
