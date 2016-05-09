#include "perclient.h"

PerClient::PerClient(size_t clientNumber, QTcpSocket* clientSocket, QObject *parent)
    :NetworkStream(clientSocket, parent)
    ,mClientNumber(clientNumber)
{
    connect(this, &NetworkStream::newNetMessage, this, &PerClient::slotNewNetMessege);
    connect(clientSocket, &QTcpSocket::disconnected, this, &PerClient::disconnectSocket);
}

void PerClient::slotSendClientMessege(size_t clientNumber, const QByteArray &message)
{
   if (clientNumber == mClientNumber)
       this->sendMessage(message);
}

void PerClient::slotNewNetMessege(const QByteArray &message)
{
    emit newClientMessege(mClientNumber, message);
}

void PerClient::disconnectSocket()
{
    emit releaseClientPlace(mClientNumber);
    this->deleteLater();
}

