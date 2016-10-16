#include "serverclientmanager.h"
#include "clientsessiononserver.h"
#include <iostream>

ServerClientManager::ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mMaxClientNumber(maxClientNumber)
{
    mClients = new ClientSessionOnServer*[mMaxClientNumber];
    used = new bool[mMaxClientNumber];
    for (size_t i = 0; i < mMaxClientNumber; ++i)
    {
        mClients[i] = NULL;
        used[i] = false;
    }
    mTcpServer = new QTcpServer(this);
    if (!mTcpServer->listen(adress, port)) {
        std::cerr << tr("Unable to start the server: %1.").
                     arg(mTcpServer->errorString()).toStdString() << std::endl;
        return;
    }
    else
        std::cout << "Start listening" << std::endl;

    connect(mTcpServer, &QTcpServer::newConnection, this, &ServerClientManager::onNewConnection);
}

ServerClientManager::~ServerClientManager()
{
    delete[] mClients;
    delete[] used;
    mTcpServer->deleteLater();
    mClients = NULL;
    used = NULL;
    google::protobuf::ShutdownProtobufLibrary();
}

bool ServerClientManager::clientExist(size_t clientNumber)
{
    return (clientNumber < mMaxClientNumber && used[clientNumber]);
}

void ServerClientManager::onNewConnection()
{
    QTcpSocket* newClient = mTcpServer->nextPendingConnection();
    bool canAdd = false;
    size_t clientNumber = 0;
    for (size_t i = 0; i < mMaxClientNumber && !canAdd; ++i)
    {
        if (!used[i])
        {
            clientNumber = i;
            canAdd = true;
        }
    }
    if (!canAdd)
    {
        std::cerr << "Too much clients..." << std::endl;
        newClient->deleteLater();
        return;
    }

    used[clientNumber] = true;
    ClientSessionOnServer* perClient = new ClientSessionOnServer(clientNumber, newClient, this);
    connect(perClient, &ClientSessionOnServer::releaseClientPlace, this, &ServerClientManager::releaseClientPlace);
    mClients[clientNumber] = perClient;
}

void ServerClientManager::releaseClientPlace(size_t clientNumber)
{
    mClients[clientNumber] = NULL;
    used[clientNumber] = false;
}

