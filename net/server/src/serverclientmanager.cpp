#include "serverclientmanager.h"
#include "clientsessiononserver.h"
#include "clientsessiononservercreator.h"
#include <iostream>
#include <QThread>

ServerClientManager::ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mMaxClientNumber(maxClientNumber) {
    used = std::vector<bool>(mMaxClientNumber, false);
    loginsBySessionId = std::vector<std::string>(mMaxClientNumber);
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
        if (users.empty())
            break;
        else
            QThread::sleep(1);
    }
    delete mTcpServer;
    google::protobuf::ShutdownProtobufLibrary();
}

bool ServerClientManager::clientExist(size_t clientNumber) {
    return (clientNumber < mMaxClientNumber && used[clientNumber]);
}

void ServerClientManager::onNewConnection() {
    //TODO: fix it
    QTcpSocket* newClient = mTcpServer->nextPendingConnection();
    bool canAdd = false;
    size_t clientNumber = 0;
    for (size_t i = 0; i < mMaxClientNumber && !canAdd; ++i) {
        if (!used[i]) {
            clientNumber = i;
            canAdd = true;
        }
    }
    if (!canAdd) {
        std::cerr << "Too much clients..." << std::endl;
        newClient->deleteLater();
        return;
    }

    used[clientNumber] = true;
    mSockets[clientNumber] = newClient;

    ClientSessionOnServerCreator* clientSessionOnServerCreator = new ClientSessionOnServerCreator();
    clientSessionOnServerCreator->init(newClient, this, clientNumber);
    QThread* thread = new QThread();
    clientSessionOnServerCreator->moveToThread(thread);
    newClient->setParent(NULL);
    newClient->moveToThread(thread);

    connect(thread, &QThread::started, clientSessionOnServerCreator, &ClientSessionOnServerCreator::process);
    connect(clientSessionOnServerCreator, &ClientSessionOnServerCreator::destroyed, thread, &QThread::quit);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    thread->start();
}

UserDataHolder *ServerClientManager::tryAuth(const AuthStruct &authStruct) {
    QMutexLocker locker(&mutexOnAuth);

    //TODO: if (...) {..}

    //success

    if (users.count(authStruct.login) == 0) {
        users.emplace(authStruct.login, QString::fromStdString(authStruct.login));
        users.at(authStruct.login).dataHolder.initMutex();
        users.at(authStruct.login).dataHolder.loadMetadatasFromFile();
    }

    ++users.at(authStruct.login).sessionNumber;
    loginsBySessionId[authStruct.sessionId] = authStruct.login;
    return &users.at(authStruct.login).dataHolder;
}

void ServerClientManager::releaseClientPlace(std::uint64_t clientNumber) {
    std::string curLogin = loginsBySessionId[clientNumber];
    mSockets[clientNumber] = NULL;

    if (users.at(curLogin).sessionNumber == 0) {
        users.at(curLogin).dataHolder.deleteMutex();
        users.erase(curLogin);
    }
    used[clientNumber] = false;
}

