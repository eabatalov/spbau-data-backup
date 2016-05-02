#include <QCoreApplication>
#include "clientlogic.h"
#include "consolestream.h"
#include "networkstream.h"
#include <iostream>
#include <string>
#include <QHostAddress>

#define HOST_ADDRESS "0.0.0.0"

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    QCoreApplication app(argc, argv);
    NetworkStream* networkStream = new NetworkStream(QHostAddress(HOST_ADDRESS), PORT_NUMBER, &app);
    ConsoleStream* consoleStream = new ConsoleStream(&app);
    ClientLogic* clientLogic = new ClientLogic(networkStream, consoleStream, &app);
    
    //TODO:
    //google::protobuf::ShutdownProtobufLibrary();
    return app.exec();
}
