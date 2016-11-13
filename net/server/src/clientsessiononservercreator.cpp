#include "clientsessiononservercreator.h"



ClientSessionOnServerCreator::ClientSessionOnServerCreator(QObject *parent)
    : QObject(parent)
    , mSocket(NULL)
    , clientSession(NULL)
    , serverClientManager(NULL){

}

void ClientSessionOnServerCreator::init(QTcpSocket * socket, ServerClientManager * serverClientManager, int clientId) {
    mSocket = socket;
    this->serverClientManager = serverClientManager;
    this->clientId = clientId;
}

ClientSessionOnServerCreator::~ClientSessionOnServerCreator() {
    delete clientSession;
    delete mSocket;
}

void ClientSessionOnServerCreator::process() {
    clientSession = new ClientSessionOnServer(clientId, mSocket, 0);
    qRegisterMetaType<std::uint64_t>("std::uint64_t");
    connect(clientSession, SIGNAL(releaseClientPlace(std::uint64_t)), serverClientManager, SLOT(releaseClientPlace(std::uint64_t)), Qt::QueuedConnection);

    //TODO: understand why uncommenting this line leads to segfault
//    connect(clientSession, &ClientSessionOnServer::destroyed, this, &ClientSessionOnServerCreator::deleteLater);
}

