#ifndef SERVERNETWORKSTREAM_H
#define SERVERNETWORKSTREAM_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDataStream>
#include <QHostAddress>
#include <QMutex>

#include <map>
#include <vector>

#include "authentication.h"

class ServerClientManager : public QObject {
    Q_OBJECT
public:
    explicit ServerClientManager(size_t maxClientNumber, QHostAddress adress, quint16 port,QObject *parent = 0);
    ~ServerClientManager();
    QMutex* tryAuth(AuthStruct authStruct);

public slots:
    void releaseClientPlace(std::uint64_t clientNumber);

private:
    QTcpServer* mTcpServer;
    size_t mMaxClientNumber;
    std::map<std::string, std::uint64_t> sessionNumberPerClient;
    std::map<std::string, QMutex*> userDataHolder;
    std::vector<bool> used;
    std::vector<QTcpSocket*> mSockets;
    std::vector<std::string> loginsBySessionId; //fix it to map?
    bool clientExist(size_t clientNumber);

signals:

private slots:
    void onNewConnection();

};

#endif // SERVERNETWORKSTREAM_H
