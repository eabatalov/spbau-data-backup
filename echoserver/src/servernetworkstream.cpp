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

void ServerNetworkStream::slotSendMessege(size_t clientNumber, QString messege){
    if (clientExist(clientNumber))
        emit signalSendMessege(clientNumber, messege);
    else
        std::cerr << "Unknown client number (slotSendMessege): " << clientNumber << std::endl;
}

void ServerNetworkStream::slotReceiveMessege(size_t clientNumber, QString messege){
    if (clientExist(clientNumber))
        emit signalReceiveMessege(clientNumber, messege);
    else
        std::cerr << "Unknown client number (slotReceiveMessege): " << clientNumber << std::endl;
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
    connect(this, &ServerNetworkStream::signalSendMessege, perClient, &PerClient::slotSendClientMessege);
    connect(perClient, &PerClient::newClientMessege, this, &ServerNetworkStream::slotReceiveMessege);
    connect(perClient, &PerClient::releaseClientPlace, this, &ServerNetworkStream::releaseClientPlace);
}

void ServerNetworkStream::releaseClientPlace(size_t clientNumber)
{
    mClients[clientNumber] = NULL;
    used[clientNumber] = false;
}

