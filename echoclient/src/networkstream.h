#ifndef NETWORKSTREAM_H
#define NETWORKSTREAM_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <string>
#include "../../common/protocol.h"

class NetworkStream : public QObject
{
    Q_OBJECT
public:
    explicit NetworkStream(const QHostAddress & address, quint16 port, QObject *parent = 0);
    NetworkStream(QTcpSocket* tcpSocket, QObject *parent);

private:
    QTcpSocket* mTcpSocket;
    quint16 mNextBlockSize;

protected:
    void sendMessage(const QString & message);

signals:
    void newNetMessage(const QString & message);
    void connected();

public slots:
    void receiveMessage();
    void sendNetMessage(const QString & message);

private slots:
    void displayError(QAbstractSocket::SocketError socketError);
    void socketDisconnected();
    void socketConnected();
};

#endif // NETWORKSTREAM_H
