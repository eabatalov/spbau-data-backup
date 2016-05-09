#ifndef SERVERNETWORKSTREAM_H
#define SERVERNETWORKSTREAM_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QHostAddress>
#include "perclient.h"

class ServerNetworkStream : public QObject
{
    Q_OBJECT
public:
    explicit ServerNetworkStream(size_t maxClientNumber, QHostAddress adress, quint16 port,QObject *parent = 0);
    ~ServerNetworkStream();

private:
    QTcpServer* mTcpServer;
    size_t mMaxClientNumber;
    PerClient** mClients;
    bool* used;
    bool clientExist(size_t clientNumber);

signals:

public slots:

private slots:
    void slotNewConnection();
    void releaseClientPlace(size_t clientNumber);
};

#endif // SERVERNETWORKSTREAM_H
