#include "serverclientmanager.h"
#include "clientsessiononserver.h"
#include "clientsessiononservercreator.h"
#include <iostream>
#include <QThread>

ServerClientManager::ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mMaxClientNumber(maxClientNumber) {
    //mClients = new ClientSessionOnServer*[mMaxClientNumber];
    used = std::vector<bool>(mMaxClientNumber, false);
    for (size_t i = 0; i < mMaxClientNumber; ++i) {
    //    mClients[i] = NULL;
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
    mTcpServer->deleteLater();
    //mClients = NULL;
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

