#ifndef SERVERNETWORKSTREAM_H
#define SERVERNETWORKSTREAM_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QHostAddress>
#include <QMutex>

#include <unordered_map>
#include <vector>

#include "authentication.h"
#include <consolestream.h>
#include "userdataholder.h"
#include "serverStructs.pb.h"


class ServerClientManager : public QObject {
    Q_OBJECT
public:
    explicit ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port,QObject *parent = 0);
    ~ServerClientManager();
    UserDataHolder* tryAuth(const AuthStruct & authStruct);
    UserDataHolder* tryRegister(const AuthStruct & authStruct);

    struct User {
        UserDataHolder dataHolder;
        std::uint64_t sessionNumber;

        User(QString login)
            :dataHolder(login)
            ,sessionNumber(0) { }
    };

public slots:
    void releaseClientPlace(std::uint64_t clientNumber);

private:
    QTcpServer* mTcpServer;
    QMutex mMutexOnAuth;
    size_t mMaxClientNumber;
    std::unordered_map<std::string, User> mUsers;
    std::vector<bool> mUsed;
    std::vector<QTcpSocket*> mSockets;
    std::vector<std::string> mLoginsBySessionId; //fix it to map?
    serverUtils::protobufStructs::VectorOfUserCredentials mUsersCredentials;

    bool clientExist(size_t clientNumber);
    void killAllSessionByLogin(const std::string& login);
    bool isUserRegister(const std::string login);
    bool checkUserCredentials(const std::string & login, const std::string & password);

signals:
    void sigWriteToConsole(const std::string& message);

private slots:
    void onNewConnection();
    void onConsoleInput(const std::string& message);

};

#endif // SERVERNETWORKSTREAM_H
