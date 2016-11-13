#ifndef SERVERNETWORKSTREAM_H
#define SERVERNETWORKSTREAM_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QHostAddress>
#include "clientsessiononserver.h"

class ServerClientManager : public QObject {
    Q_OBJECT
public:
    explicit ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port,QObject *parent = 0);
    ~ServerClientManager();

public slots:
    void releaseClientPlace(std::uint64_t clientNumber);

private:
    QTcpServer* mTcpServer;
    size_t mMaxClientNumber;
    //ClientSessionOnServer** mClients;
    bool* used;
    bool clientExist(size_t clientNumber);

signals:

private slots:
    void onNewConnection();

};

#endif // SERVERNETWORKSTREAM_H
