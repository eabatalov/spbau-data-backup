#include "server.h"
#include <iostream>
#include <QString>

Server::Server(ServerNetworkStream* serverNetworkStream, QObject *parent)
    : QObject(parent)
{
    connect(this, &Server::sendMessege, serverNetworkStream, &ServerNetworkStream::slotSendMessege);
    connect(serverNetworkStream, &ServerNetworkStream::signalReceiveMessege, this, &Server::receiveMessege);
}

void Server::receiveMessege(size_t clientNumber, QString messege)
{
    std::cout << clientNumber << ": " << messege.toStdString() << std::endl;
    emit sendMessege(clientNumber, messege);
}
