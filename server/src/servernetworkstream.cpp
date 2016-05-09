#include "servernetworkstream.h"
#include "perclient.h"
#include <iostream>

ServerNetworkStream::ServerNetworkStream(size_t maxClientNumber, QHostAddress adress, quint16 port, QObject *parent)
    : QObject(parent)
    , mMaxClientNumber(maxClientNumber)
{
    mClients = new PerClient*[mMaxClientNumber];
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

    connect(mTcpServer, &QTcpServer::newConnection, this, &ServerNetworkStream::slotNewConnection);
}

ServerNetworkStream::~ServerNetworkStream()
{
    delete mClients;
    delete used;
    mTcpServer->deleteLater();
    //delete mTcpServer;
    mClients = NULL;
    used = NULL;
}

bool ServerNetworkStream::clientExist(size_t clientNumber)
{
    return (clientNumber < mMaxClientNumber && used[clientNumber]);
}

void ServerNetworkStream::slotNewConnection()
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
        return;
    }

    used[clientNumber] = true;
    PerClient* perClient = new PerClient(clientNumber, newClient, this);
    mClients[clientNumber] = perClient;
}

void ServerNetworkStream::releaseClientPlace(size_t clientNumber)
{
    mClients[clientNumber] = NULL;
    used[clientNumber] = false;
}

