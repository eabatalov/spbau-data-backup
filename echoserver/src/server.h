#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include "servernetworkstream.h"

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(ServerNetworkStream* serverNetworkStream, QObject *parent = 0);

private:


signals:
    void sendMessege(size_t clientNumber, QByteArray messege);

public slots:


private slots:
    void receiveMessege(size_t clientNumber, QByteArray messege);

};

#endif // SERVER_H
