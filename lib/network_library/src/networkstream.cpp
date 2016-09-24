#include "networkstream.h"
#include <iostream>
#include <QString>
#include <QHostAddress>
#include <cassert>
#include <QByteArray>


NetworkStream::NetworkStream(const QHostAddress &address, quint16 port, QObject *parent)
    : QObject(parent)
    , mNextBlockSize(0)
{
    mTcpSocket = new QTcpSocket(this);

    connect(mTcpSocket, &QTcpSocket::readyRead, this, &NetworkStream::receiveMessage);
    connect(mTcpSocket, static_cast<void (QTcpSocket::*) (QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &NetworkStream::displayError);
    connect(mTcpSocket, &QTcpSocket::disconnected, this, &NetworkStream::socketDisconnected);
    connect(mTcpSocket, &QTcpSocket::connected, this, &NetworkStream::socketConnected);

    mTcpSocket->connectToHost(address, port);
}

NetworkStream::NetworkStream(QTcpSocket* tcpSocket, QObject *parent)
    : QObject(parent)
    , mTcpSocket(tcpSocket)
    , mNextBlockSize(0)
{
    connect(mTcpSocket, &QTcpSocket::readyRead, this, &NetworkStream::receiveMessage);
    connect(mTcpSocket, static_cast<void (QTcpSocket::*) (QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &NetworkStream::displayError);
    connect(mTcpSocket, &QTcpSocket::disconnected, this, &NetworkStream::socketDisconnected);
    connect(mTcpSocket, &QTcpSocket::connected, this, &NetworkStream::socketConnected);
}

void NetworkStream::sendMessage(const QByteArray &message)
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::ReadWrite);
    out.setVersion(QDataStream::Qt_4_0);

    out << quint64(0) << message;
    out.device()->seek(0);
    out << quint64(arrBlock.size() - sizeof(quint64));
    mTcpSocket->write(arrBlock);
}

void NetworkStream::receiveMessage()
{
    QByteArray message;
    QDataStream in(mTcpSocket);
    in.setVersion(QDataStream::Qt_4_0);

    while(1)
    {
        if (!mNextBlockSize)
        {
            if ((size_t)mTcpSocket->bytesAvailable() < sizeof(quint64)) {
                break;
            }
            in >> mNextBlockSize;
        }

        if (mTcpSocket->bytesAvailable() < mNextBlockSize) {
            break;
        }
        in >> message;
        mNextBlockSize = 0;

        emit newNetMessage(message);
    }
}

void NetworkStream::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            std::cerr << tr("The host was not found. Please check the host name and port settings.").toStdString() << std::endl;
            break;
        case QAbstractSocket::ConnectionRefusedError:
            std::cerr << tr("The connection was refused by the peer. "
                                        "Make sure the fortune server is running, "
                                        "and check that the host name and port "
                                        "settings are correct.").toStdString() << std::endl;
            break;
        default:
            std::cerr << tr("The following error occurred: %1.")
                    .arg(mTcpSocket->errorString()).toStdString() << std::endl;
        }

}

void NetworkStream::sendNetMessage(const QByteArray &message)
{
    sendMessage(message);
}

void NetworkStream::socketDisconnected(){
    std::cout << "NetworkStream: " "Socket disconnected" << std::endl;
}

void NetworkStream::socketConnected(){
    std::cout << "NetworkStream: " "Socket connected" << std::endl;
    emit connected();
}
