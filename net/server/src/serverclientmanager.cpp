#include "serverclientmanager.h"
#include "clientsessiononserver.h"
#include "clientsessiononservercreator.h"
#include <iostream>
#include <QThread>

ServerClientManager::ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mMaxClientNumber(maxClientNumber) {
    //mClients = new ClientSessionOnServer*[mMaxClientNumber];
    // XXX there is a class called std::vector for this :)
    used = new bool[mMaxClientNumber];
    for (size_t i = 0; i < mMaxClientNumber; ++i) {
    //    mClients[i] = NULL;
        used[i] = false;
    }
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
    //delete[] mClients;
    delete[] used;
    mTcpServer->deleteLater();
    //mClients = NULL;
    used = NULL;
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

    //mClients[clientNumber] = perClient;
}

void ServerClientManager::releaseClientPlace(std::uint64_t clientNumber) {
    //mClients[clientNumber] = NULL;
    used[clientNumber] = false;
}

