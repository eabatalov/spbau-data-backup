#ifndef CLIENTLOGIC_H
#define CLIENTLOGIC_H

#include <QObject>
#include "protocol.h"
#include "networkMsgStructs.pb.h"
#include <networkstream.h>
#include <consolestream.h>

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
    void sendRegistrationRequest(const std::string & loginAndPassword);
    void sendLoginRequest(const std::string & loginAndPassword);
    void askLs();
    void askLs(const std::string& command);
    void makeRestoreRequest(const std::string& command);
    void makeBackup(const std::string& command);
    void sendLsFromClient(networkUtils::protobufStructs::LsClientRequest ls);
    void sendRestoreResult(bool restoreResult);
    void sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize);

    //ReceiveNetMessage:
    void procRegistrationAns(const char *buffer, uint64_t bufferSize);
    void procLoginAns(const char *buffer, uint64_t bufferSize);
    void procDetailedLs(const char *buffer, uint64_t bufferSize);
    void procSummaryLs(const char *buffer, uint64_t bufferSize);
    void procReceiveArchiveToRestore(const char *buffer, uint64_t bufferSize);
    void procReceiveBackupResults(const char *buffer, uint64_t bufferSize);
    void procServerError(const char *buffer, uint64_t bufferSize);
    void procNotFoundBackupId(const char *buffer, uint64_t bufferSize);
    void procServerExit(const char *buffer, uint64_t bufferSize);


    std::string restorePath;


private slots:
    void onConsoleInput(const std::string& message);
    void onNetworkInput(const QByteArray & message);
    void onStart();

};

#endif // CLIENTLOGIC_H
