#include "clientsessiononservercreator.h"
#include "authentication.h"



ClientSessionOnServerCreator::ClientSessionOnServerCreator(QObject *parent)
    : QObject(parent)
    , mSocket(NULL)
    , mClientSession(NULL)
    , mServerClientManager(NULL){

}

void ClientSessionOnServerCreator::init(QTcpSocket * socket, ServerClientManager * serverClientManager, int clientId) {
    mSocket = socket;
    this->mServerClientManager = serverClientManager;
    this->mClientId = clientId;
}

ClientSessionOnServerCreator::~ClientSessionOnServerCreator() {
}

void ClientSessionOnServerCreator::process() {
    mClientSession = new ClientSessionOnServer(mServerClientManager, mClientId, mSocket, 0);
    qRegisterMetaType<std::uint64_t>("std::uint64_t");
    qRegisterMetaType<AuthStruct>("AuthStruct");
    connect(mClientSession, &ClientSessionOnServer::releaseClientPlace, mServerClientManager, &ServerClientManager::releaseClientPlace);
    connect(mClientSession, &ClientSessionOnServer::destroyed, this, &ClientSessionOnServerCreator::deleteLater);
}

