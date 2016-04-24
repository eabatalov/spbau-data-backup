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
    void signalSendMessege(size_t clientNumber, QString messege);
    void signalReceiveMessege(size_t clientNumber, QString messege);


public slots:
    void slotSendMessege(size_t clientNumber, QString messege);

private slots:
    void slotReceiveMessege(size_t clientNumber, QString messege);
    void slotNewConnection();
    void releaseClientPlace(size_t clientNumber);
};

#endif // SERVERNETWORKSTREAM_H
