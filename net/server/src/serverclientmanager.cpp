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


ServerClientManager::ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mMaxClientNumber(maxClientNumber) {
    mUsed = std::vector<bool>(mMaxClientNumber, false);
    mLoginsBySessionId = std::vector<std::string>(mMaxClientNumber);
    mSockets = std::vector<QTcpSocket*>(mMaxClientNumber);

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
    for (size_t i = 0; i < mMaxClientNumber; ++i) {
        if (mSockets[i]) {
            mSockets[i]->close();
        }
    }
    while (true) {
        if (mUsers.empty())
            break;
        else
            QThread::sleep(1);
    }
    delete mTcpServer;
    google::protobuf::ShutdownProtobufLibrary();
}

bool ServerClientManager::clientExist(size_t clientNumber) {
    return (clientNumber < mMaxClientNumber && mUsed[clientNumber]);
}

void ServerClientManager::onNewConnection() {
    //TODO: fix it
    QTcpSocket* newClient = mTcpServer->nextPendingConnection();
    bool canAdd = false;
    size_t clientNumber = 0;
    for (size_t i = 0; i < mMaxClientNumber && !canAdd; ++i) {
        if (!mUsed[i]) {
            clientNumber = i;
            canAdd = true;
        }
    }
    if (!canAdd) {
        std::cerr << "Too much clients..." << std::endl;
        newClient->deleteLater();
        return;
    }

    mUsed[clientNumber] = true;
    mSockets[clientNumber] = newClient;

    ClientSessionOnServerCreator* clientSessionOnServerCreator = new ClientSessionOnServerCreator();
    clientSessionOnServerCreator->init(newClient, this, clientNumber);
    QThread* thread = new QThread();
    clientSessionOnServerCreator->moveToThread(thread);
    newClient->setParent(NULL);
    newClient->moveToThread(thread);

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
    mLoginsBySessionId[authStruct.sessionId] = authStruct.login;
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
    mLoginsBySessionId[authStruct.sessionId] = authStruct.login;
    return &mUsers.at(authStruct.login).dataHolder;
}

bool ServerClientManager::isUserRegister(const std::string login) {
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

void ServerClientManager::releaseClientPlace(std::uint64_t clientNumber) {
    std::string curLogin = mLoginsBySessionId[clientNumber];
    mSockets[clientNumber] = NULL;

    if (curLogin != "" && mUsers.at(curLogin).sessionNumber == 0) {
        mUsers.at(curLogin).dataHolder.deleteMutex();
        mUsers.erase(curLogin);
    }
    mUsed[clientNumber] = false;
}

void ServerClientManager::onConsoleInput(const std::string& message) {
    if (message == "exit") {
        //TODO: fix it
        QCoreApplication* ca;
        QMetaObject::invokeMethod( ca, "quit");
        return;
    }

    if (message == "ls") {
        std::stringstream ss;
        ss << "List of session:\n";
        for (size_t i = 0; i < this->mMaxClientNumber; ++i) {
            if (mUsed[i]) {
                ss << "sessionId = " << i << " : " << "Login = " << mLoginsBySessionId[i] << std::endl;
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
    for (size_t i = 0; i < this->mMaxClientNumber; ++i) {
        if (mUsed[i] && mLoginsBySessionId[i] == login) {
            mSockets[i]->disconnectFromHost();
        }
    }
}
