#include <QCoreApplication>
#include <iostream>
#include <string>
#include <QHostAddress>
#include <networkMsgStructs.pb.h>
#include "serverclientmanager.h"

#include <protocol.h>
#define HOST_ADDRESS "0.0.0.0"

int main(int argc, char *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    QCoreApplication app(argc, argv);

    ServerClientManager serverClientManager(4, QHostAddress::Any, PORT_NUMBER, &app);

    return app.exec();
}
