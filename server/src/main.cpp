#include <QCoreApplication>
#include <iostream>
#include <string>
#include <QHostAddress>
#include <networkMsgStructs.pb.h>
#include <servernetworkstream.h>

#define HOST_ADDRESS "0.0.0.0"

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    QCoreApplication app(argc, argv);

    ServerNetworkStream serverNetworkStream(4, QHostAddress::Any, PORT_NUMBER, &app);

    
    
    //TODO:
    //google::protobuf::ShutdownProtobufLibrary();
    return app.exec();
}
