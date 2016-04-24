#include <QCoreApplication>
#include "echoclientlogic.h"
#include "consolestream.h"
#include "networkstream.h"
#include <iostream>
#include <string>
#include <QHostAddress>

#define HOST_ADDRESS "0.0.0.0"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    NetworkStream* networkStream = new NetworkStream(QHostAddress(HOST_ADDRESS), PORT_NUMBER, &app);
    ConsoleStream* consoleStream = new ConsoleStream(&app);
    EchoClientLogic* echoClientLogic = new EchoClientLogic(networkStream, consoleStream, &app);
    return app.exec();
}
