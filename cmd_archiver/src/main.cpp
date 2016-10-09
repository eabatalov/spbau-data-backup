#include <QCoreApplication>
#include "commandlinemanager.h"

#include <struct_serialization.pb.h>

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("cmd_archiver");
    QCoreApplication::setApplicationVersion("1.0");

    CommandLineManager commandLineManager(app, &app);
    commandLineManager.process();

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
