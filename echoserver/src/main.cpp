#include <QCoreApplication>
#include "server.h"
#include "../../common/protocol.h"
#include <iostream>
#include "servernetworkstream.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ServerNetworkStream* serverNetworkStream = new ServerNetworkStream(3, QHostAddress::Any, PORT_NUMBER, &app);
    Server* mysrv = new Server(serverNetworkStream, &app);


    return app.exec();
}
