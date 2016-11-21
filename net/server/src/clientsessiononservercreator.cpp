#include "clientsessiononservercreator.h"
#include "authentication.h"



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
    //delete clientSession;
    //delete mSocket;
}

void ClientSessionOnServerCreator::process() {
    clientSession = new ClientSessionOnServer(serverClientManager, clientId, mSocket, 0);
    qRegisterMetaType<std::uint64_t>("std::uint64_t");
    qRegisterMetaType<AuthStruct>("AuthStruct");
    connect(clientSession, &ClientSessionOnServer::releaseClientPlace, serverClientManager, &ServerClientManager::releaseClientPlace);
    connect(clientSession, &ClientSessionOnServer::destroyed, this, &ClientSessionOnServerCreator::deleteLater);
}

