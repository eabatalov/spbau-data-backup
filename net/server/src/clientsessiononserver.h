#ifndef PERCLIENT_H
#define PERCLIENT_H

#include <networkstream.h>
#include <serverStructs.pb.h>

#include <QMutex>
#include <QTcpSocket>
#include <QString>

#include "authentication.h"
#include "serverclientmanager.h"
#include "userdataholder.h"

class ClientSessionOnServer:public QObject {
    Q_OBJECT
public:
    ClientSessionOnServer(ServerClientManager *serverClientManager, size_t clientNumber, QTcpSocket *clientSocket, QObject* parent = 0);

public slots:

private:
    std::uint64_t mSessionNumber;
    ServerClientManager* mServerClientManager;
    UserDataHolder* mUserDataHolder;

    std::uint64_t mRestoreBackupId;
    enum PerClientState {
        NOT_AUTORIZATED,
        WAIT_AUTH_RESULT,
        NORMAL,
        WAIT_RESTORE_RESULT,
        ABORTED
    } mPerClientState;

    void procRegistarionRequest(const char* buffer, std::uint64_t bufferSize);
    void procLoginRequest(const char* buffer, std::uint64_t bufferSize);
    void procLsRequest(const char* buffer, std::uint64_t bufferSize);
    void procRestoreRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void procReplyAfterRestore(const char* bufferbuffer, std::uint64_t bufferSize);
    void procBackupRequest(const char* bufferbuffer, std::uint64_t bufferSize);
    void procClientExit();
    void sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize);
    void sendAnsToClietnRegistration(bool result);
    void sendAnsToClientLogin(bool result);
    void sendNotFoundBackupIdToClient(std::uint64_t backupId);
    void sendServerError(QString message);
    void sendServerExit(QString message);

signals:
    void releaseClientPlace(std::uint64_t clientNumber);
    void sigSendClientMessage(const QByteArray & message);
    void sigDisconnectSocket();
    void deleteMe();

private slots:
    void onNetworkInput(const QByteArray & message);
    void disconnectSocket();
};

#endif // PERCLIENT_H
