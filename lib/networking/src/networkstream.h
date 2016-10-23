#ifndef NETWORKSTREAM_H
#define NETWORKSTREAM_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QByteArray>
#include <string>
#include "../../protocol/protocol.h"

class NetworkStream : public QObject
{
    Q_OBJECT
public:
    explicit NetworkStream(const QHostAddress & address, quint16 port, QObject *parent = 0);
    NetworkStream(QTcpSocket* tcpSocket, QObject *parent);

public slots:
    void receiveMessage();
    void sendNetMessage(const QByteArray & message);

signals:
    void newNetMessage(const QByteArray & message);
    void connected();

private:
    QTcpSocket* mTcpSocket;
    qint64 mNextBlockSize;

protected:
    void sendMessage(const QByteArray & message);

private slots:
    void displayError(QAbstractSocket::SocketError socketError);
    void socketDisconnected();
    void socketConnected();
};

#endif // NETWORKSTREAM_H
