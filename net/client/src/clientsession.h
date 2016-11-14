#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include <QObject>
#include "protocol.h"
#include "networkMsgStructs.pb.h"
#include "networkstream.h"
#include "consolestream.h"

class ClientSession : public QObject {
    Q_OBJECT
public:
    explicit ClientSession(NetworkStream* networkStream, ConsoleStream* consoleStream, QObject *parent = 0);
    ~ClientSession() {
        // TODO move protobuf lib deinitialization to the end of main
        google::protobuf::ShutdownProtobufLibrary();
    }

signals:
    void sigWriteToConsole(const std::string& message);
    void sigWriteToNetwork(const QByteArray & message);

private:
    // XXX We don't need those pointers because we use signals
    NetworkStream* mNetworkStream;
    ConsoleStream* mConsoleStream;
    enum ClientState {
        NOT_STARTED,
        WAIT_LOGIN_INPUT,
        WAIT_LOGIN_STATUS,
        WAIT_USER_INPUT,
        WAIT_LS_RESULT,
        WAIT_RESTORE_RESULT,
        WAIT_BACKUP_RESULT,
        ABORTED,
        FINISHED
    } mClientState;

    //SendNetMessage:
    void sendLoginRequest(const std::string & login);
    void askLs();
    void askLs(const std::string& command);
    void makeRestoreRequest(const std::string& command);
    void makeBackup(const std::string& command);
    void sendLsFromClient(networkUtils::protobufStructs::LsClientRequest ls);
    void sendRestoreResult(bool restoreResult);
    void sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize);

    //ReceiveNetMessage:
    // XXX let's rename non-slot functions to somethings like procLoginAns
    void OnLoginAns(const char *buffer, uint64_t bufferSize);
    void OnDetailedLs(const char *buffer, uint64_t bufferSize);
    void OnSummaryLs(const char *buffer, uint64_t bufferSize);
    void OnReceiveArchiveToRestore(const char *buffer, uint64_t bufferSize);
    void OnReceiveBackupResults(const char *buffer, uint64_t bufferSize);
    void OnServerError(const char *buffer, uint64_t bufferSize);
    void OnNotFoundBackupId(const char *buffer, uint64_t bufferSize);
    void OnServerExit(const char *buffer, uint64_t bufferSize);


    std::string restorePath;


private slots:
    void onConsoleInput(const std::string& message);
    void onNetworkInput(const QByteArray & message);
    void onStart();

};

#endif // CLIENTLOGIC_H
