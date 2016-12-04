#include "serverclientmanager.h"
#include "clientsessiononserver.h"
#include "clientsessiononservercreator.h"
#include "threaddeleter.h"
#include <iostream>
#include <QThread>

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

UserDataHolder *ServerClientManager::tryAuth(const AuthStruct &authStruct) {
    QMutexLocker locker(&mMutexOnAuth);

    //TODO: if (...) {..}

    //success

    if (mUsers.count(authStruct.login) == 0) {
        mUsers.emplace(authStruct.login, QString::fromStdString(authStruct.login));
        mUsers.at(authStruct.login).dataHolder.initMutex();
        mUsers.at(authStruct.login).dataHolder.loadMetadatasFromFile();
    }

    ++mUsers.at(authStruct.login).sessionNumber;
    mLoginsBySessionId[authStruct.sessionId] = authStruct.login;
    return &mUsers.at(authStruct.login).dataHolder;
}

void ServerClientManager::releaseClientPlace(std::uint64_t clientNumber) {
    std::string curLogin = mLoginsBySessionId[clientNumber];
    mSockets[clientNumber] = NULL;

    if (mUsers.at(curLogin).sessionNumber == 0) {
        mUsers.at(curLogin).dataHolder.deleteMutex();
        mUsers.erase(curLogin);
    }
    mUsed[clientNumber] = false;
}

