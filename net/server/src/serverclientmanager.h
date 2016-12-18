#ifndef SERVERNETWORKSTREAM_H
#define SERVERNETWORKSTREAM_H

#include <QCoreApplication>
#include <QDataStream>
#include <QHostAddress>
#include <QMutex>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <queue>
#include <memory>
#include <unordered_map>
#include <vector>

#include "authentication.h"
#include <consolestream.h>
#include "userdataholder.h"
#include "serverStructs.pb.h"


class ServerClientManager : public QObject {
    Q_OBJECT
public:
    explicit ServerClientManager(QCoreApplication* app, QHostAddress adress, quint16 port, QObject *parent = 0);
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

    struct Session {
        QTcpSocket* socket;
        std::string login;
        Session(QTcpSocket* socket = NULL)
            :socket(socket) { }
    };

public slots:
    void releaseClientPlace(std::uint64_t sessionId);

private:
    QCoreApplication* mApp;
    QTcpServer* mTcpServer;
    QMutex mMutexOnAuth;
    std::unordered_map<std::string, User> mUsers;
    std::vector< std::auto_ptr<Session> > mSessions;
    std::queue<size_t> mFreeSessionIdQueue;
    serverUtils::protobufStructs::VectorOfUserCredentials mUsersCredentials;

    bool clientExist(size_t sessionId);
    void killAllSessionByLogin(const std::string& login);
    bool isUserRegister(const std::string & login);
    bool checkUserCredentials(const std::string & login, const std::string & password);

signals:
    void sigWriteToConsole(const std::string& message);

private slots:
    void onNewConnection();
    void onConsoleInput(const std::string& message);

};

#endif // SERVERNETWORKSTREAM_H
