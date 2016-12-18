#include "serverclientmanager.h"
#include "clientsessiononserver.h"
#include "clientsessiononservercreator.h"
#include "threaddeleter.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <QThread>
#include <QCoreApplication>


ServerClientManager::ServerClientManager(QCoreApplication *app, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mApp(app){
    mTcpServer = new QTcpServer(this);
    if (!mTcpServer->listen(adress, port)) {
        std::cerr << tr("Unable to start the server: %1.").
                     arg(mTcpServer->errorString()).toStdString() << std::endl;
        return;
    } else
        std::cout << "Start listening" << std::endl;

    connect(mTcpServer, &QTcpServer::newConnection, this, &ServerClientManager::onNewConnection);
    ConsoleStream* consoleStream = new ConsoleStream(this);
    connect(consoleStream, &ConsoleStream::readln, this, &ServerClientManager::onConsoleInput);
    connect(this, &ServerClientManager::sigWriteToConsole, consoleStream, &ConsoleStream::println);

    std::string pathToUserCredentials("userCredentials.secure");
    std::fstream usersCredentialsFile(pathToUserCredentials, std::ios::in | std::ios::binary);

    if (usersCredentialsFile.is_open()) {
        if (!mUsersCredentials.ParseFromIstream(&usersCredentialsFile)) {
              std::cerr << "Failed to read usersCredentials info." << std::endl;
        }
    } else {
        std::fstream newUsersCredentialsFile(pathToUserCredentials, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!mUsersCredentials.SerializeToOstream(&newUsersCredentialsFile)) {
              std::cerr << "Failed to create usersCredentials info file." << std::endl;
        }
    }
}

ServerClientManager::~ServerClientManager() {
    delete mTcpServer;
    google::protobuf::ShutdownProtobufLibrary();
}

bool ServerClientManager::clientExist(size_t sessionId) {
    return (mSessions.at(sessionId).get() != NULL);
}

void ServerClientManager::onNewConnection() {
    QTcpSocket* newSocket = mTcpServer->nextPendingConnection();

    Session* newSession = new Session(newSocket);
    size_t sessionId = 0;

    if (mFreeSessionIdQueue.empty()) {
        sessionId = mSessions.size();
        mSessions.push_back(std::auto_ptr<Session>(newSession));
    } else {
        sessionId = mFreeSessionIdQueue.front();
        mFreeSessionIdQueue.pop();
        mSessions.at(sessionId).reset(newSession);
    }

    ClientSessionOnServerCreator* clientSessionOnServerCreator = new ClientSessionOnServerCreator();
    clientSessionOnServerCreator->init(newSocket, this, sessionId);
    QThread* thread = new QThread();
    clientSessionOnServerCreator->moveToThread(thread);
    newSocket->setParent(NULL);
    newSocket->moveToThread(thread);

    connect(thread, &QThread::started, clientSessionOnServerCreator, &ClientSessionOnServerCreator::process);
    ThreadDeleter* threadDeleter = new ThreadDeleter(thread);
    connect(clientSessionOnServerCreator, &ClientSessionOnServerCreator::destroyed, thread, &QThread::quit);

    thread->start();
}

UserDataHolder *ServerClientManager::tryRegister(const AuthStruct & authStruct) {
    QMutexLocker locker(&mMutexOnAuth);

    if (isUserRegister(authStruct.login)) {
        return NULL;
    }

    serverUtils::protobufStructs::UserCredentials* userCredentials = this->mUsersCredentials.add_usercredentials();
    userCredentials->set_login(authStruct.login);
    userCredentials->set_password(authStruct.password);

    std::string pathToUserCredentials("userCredentials.secure");
    std::fstream usersCredentialsFile(pathToUserCredentials, std::ios::out | std::ios::binary);

    if (usersCredentialsFile.is_open()) {
        if (!mUsersCredentials.SerializeToOstream(&usersCredentialsFile)) {
              std::cerr << "Failed to write usersCredentials info." << std::endl;
        }
    }

    mUsers.emplace(authStruct.login, QString::fromStdString(authStruct.login));
    mUsers.at(authStruct.login).dataHolder.initMutex();
    mUsers.at(authStruct.login).dataHolder.loadMetadatasFromFile();

    ++mUsers.at(authStruct.login).sessionNumber;
    mSessions.at(authStruct.sessionId)->login = authStruct.login;
    return &mUsers.at(authStruct.login).dataHolder;
}

UserDataHolder *ServerClientManager::tryAuth(const AuthStruct &authStruct) {
    QMutexLocker locker(&mMutexOnAuth);

    if (!checkUserCredentials(authStruct.login, authStruct.password)) {
        return NULL;
    }

    if (mUsers.count(authStruct.login) == 0) {
        mUsers.emplace(authStruct.login, QString::fromStdString(authStruct.login));
        mUsers.at(authStruct.login).dataHolder.initMutex();
        mUsers.at(authStruct.login).dataHolder.loadMetadatasFromFile();
    }

    ++mUsers.at(authStruct.login).sessionNumber;
    mSessions.at(authStruct.sessionId)->login = authStruct.login;
    return &mUsers.at(authStruct.login).dataHolder;
}

bool ServerClientManager::isUserRegister(const std::string & login) {
    bool flag = false;
    for (size_t i = 0; i < mUsersCredentials.usercredentials().size(); ++i) {
        if (mUsersCredentials.usercredentials(i).login() == login) {
            flag = true;
            break;
        }
    }
    return flag;
}

bool ServerClientManager::checkUserCredentials(const std::string & login, const std::string & password) {
    bool flag = false;
    for (size_t i = 0; i < mUsersCredentials.usercredentials().size(); ++i) {
        if (mUsersCredentials.usercredentials(i).login() == login && mUsersCredentials.usercredentials(i).password() == password) {
            flag = true;
            break;
        }
    }
    return flag;
}

void ServerClientManager::releaseClientPlace(std::uint64_t sessionId) {
    std::string curLogin = mSessions.at(sessionId)->login;
    mSessions.at(sessionId)->socket = NULL;

    if (curLogin != "" && mUsers.at(curLogin).sessionNumber == 0) {
        mUsers.erase(curLogin);
    }
    mSessions.at(sessionId).reset();
    mFreeSessionIdQueue.push(sessionId);
}

void ServerClientManager::onConsoleInput(const std::string& message) {
    if (message == "exit") {
        //TODO: fix it
        for (size_t i = 0; i < mSessions.size(); ++i) {
            Session* curSession = mSessions.at(i).get();
            if (curSession != NULL) {
                curSession->socket->disconnectFromHost();
            }
        }
        QMetaObject::invokeMethod( mApp, "quit");
        return;
    }

    if (message == "ls") {
        std::stringstream ss;
        ss << "List of session:\n";
        for (size_t i = 0; i < mSessions.size(); ++i) {
            if (mSessions.at(i).get() != NULL) {
                ss << "sessionId = " << i << " : " << "Login = " << mSessions.at(i)->login << std::endl;
            }
        }
        emit sigWriteToConsole(ss.str()); //tempory object?
        return;
    }

    if (message.length() > 6 && message.substr(0, 4) == "kill") {
        killAllSessionByLogin(message.substr(5));
    }
}

void ServerClientManager::killAllSessionByLogin(const std::string& login) {
    for (size_t i = 0; i < mSessions.size(); ++i) {
        Session* curSession = mSessions.at(i).get();
        if (curSession != NULL && curSession->login == login) {
            curSession->socket->disconnectFromHost();
        }
    }
}
