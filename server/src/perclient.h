#ifndef PERCLIENT_H
#define PERCLIENT_H

#include "../../echoclient/src/networkstream.h"
#include <networkstream.h>
#include <QTcpSocket>
#include <string>
#include <serverStructs.pb.h>

class PerClient:public QObject
{
    Q_OBJECT
public:
    PerClient(size_t clientNumber, QTcpSocket *clientSocket, QObject* parent = 0);

private:
    size_t mClientNumber;
    std::string mLogin;
    NetworkStream* mNetworkStream;
    serverUtils::protobufStructs::VectorOfServerMetadataForArchive mMetadatas;
    enum PerClientState{
        NOT_AUTORIZATED,
        NORMAL,
        WAIT_RESTORE_RESULT,
        ABORTED
    } mPerClientState;
    void onLsRequest(const char* buffer, std::uint64_t bufferSize);
    void onRestoreRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void onReplyAfterRestore(const char* bufferbuffer, std::uint64_t bufferSize);
    void onBackupRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void onclientExit(const char* bufferbuffer, std::uint64_t bufferSize);

signals:
    void releaseClientPlace(size_t clientNumber);
    void sigSendClientMessage(const QByteArray & message);

private slots:
    void slotNewNetMessege(const QByteArray & message);
    void disconnectSocket();
};

#endif // PERCLIENT_H
