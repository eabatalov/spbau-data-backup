#ifndef CLIENTSESSIONONSERVERCREATOR_H
#define CLIENTSESSIONONSERVERCREATOR_H

#include <QObject>
#include <QTcpSocket>

#include "clientsessiononserver.h"
#include "serverclientmanager.h"


class ClientSessionOnServerCreator : public QObject
{
    Q_OBJECT
public:
    explicit ClientSessionOnServerCreator(QObject *parent = 0);
    void init(QTcpSocket * socket, ServerClientManager * mServerClientManager, int mClientId);
    ~ClientSessionOnServerCreator();

signals:

public slots:
    void process();

private:
    QTcpSocket * mSocket;
    ClientSessionOnServer * mClientSession;
    ServerClientManager * mServerClientManager;
    int mClientId;
};

#endif // CLIENTSESSIONONSERVERCREATOR_H
