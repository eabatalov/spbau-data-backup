#ifndef PERCLIENT_H
#define PERCLIENT_H

#include <networkstream.h>
#include <serverStructs.pb.h>

#include <QTcpSocket>
#include <QString>




class ClientSessionOnServer:public QObject {
    Q_OBJECT
public:
    ClientSessionOnServer(size_t clientNumber, QTcpSocket *clientSocket, QObject* parent = 0);

private:
    std::uint64_t mClientNumber;
    QString mLogin;
    NetworkStream* mNetworkStream;
    serverUtils::protobufStructs::VectorOfServerMetadataForArchive mMetadatas;
    std::uint64_t newBackupId;
    std::uint64_t mRestoreBackupId;
    enum PerClientState {
        NOT_AUTORIZATED,
        NORMAL,
        WAIT_RESTORE_RESULT,
        ABORTED
    } mPerClientState;

    bool isValidBackupId(std::uint64_t backupId);
    void procLoginRequest(const char* buffer, std::uint64_t bufferSize);
    void procLsRequest(const char* buffer, std::uint64_t bufferSize);
    void procRestoreRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void procReplyAfterRestore(const char* bufferbuffer, std::uint64_t bufferSize);
    void procBackupRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void procClientExit();
    void sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize);
    bool saveStateMetadatas();
    void sendNotFoundBackupIdToClient(std::uint64_t backupId);
    void sendServerError(QString message);
    void sendServerExit(QString message);

signals:
    void releaseClientPlace(std::uint64_t clientNumber);
    void sigSendClientMessage(const QByteArray & message);

private slots:
    void onNetworkInput(const QByteArray & message);
    void disconnectSocket();
};

#endif // PERCLIENT_H
