#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include <QObject>
#include "protocol.h"
#include "networkMsgStructs.pb.h"
#include "networkstream.h"
#include "consolestream.h"

class ClientLogic : public QObject
{
    Q_OBJECT
public:
    explicit ClientLogic(NetworkStream* networkStream, ConsoleStream* consoleStream, QObject *parent = 0);

private:
    NetworkStream* mNetworkStream;
    ConsoleStream* mConsoleStream;
    enum ClientState{
        NOT_STARTED,
        WAIT_USER_INPUT,
        WAIT_LS_RESULT,
        WAIT_RESTORE_RESULT,
        WAIT_BACKUP_RESULT,
        ABORTED
    } mClientState;

    //SendNetMessage:
    void askLs();
    void askLs(const std::string& command);
    void makeRestoreRequest(const std::string& command);
    void makeBackup(const std::string& command);
    void sendLsFromClient(networkUtils::protobufStructs::LsClientRequest ls);
    void sendRestoreResult(bool restoreResult);

    //ReceiveNetMessage:
    void OnDetailedLs(const char *buffer, uint64_t bufferSize);
    void OnSummaryLs(const char *buffer, uint64_t bufferSize);
    void OnReceiveArchiveToRestore(const char *buffer, uint64_t bufferSize);
    void OnReceiveBackupResults(const char *buffer, uint64_t bufferSize);
    void OnServerError(const char *buffer, uint64_t bufferSize);

    std::string restorePath;
signals:
    void sigWriteToConsole(const std::string& message);
    void sigWriteToNetwork(const QByteArray & message);

private slots:
    void slotReadFromConsole(const std::string& message);
    void slotReadFromNetwork(const QByteArray & message);
    void slotStarting();

};

#endif // CLIENTLOGIC_H
