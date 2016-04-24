#ifndef ECHOCLIENTLOGIC_H
#define ECHOCLIENTLOGIC_H

#include <QObject>
#include "networkstream.h"
#include "consolestream.h"
#include <string>

class EchoClientLogic : public QObject
{
    Q_OBJECT
public:
    explicit EchoClientLogic(NetworkStream* networkStream, ConsoleStream* consoleStream, QObject *parent = 0);

private:
    NetworkStream* mNetworkStream;
    ConsoleStream* mConsoleStream;
    enum EchoClientState{
        NOT_STARTED,
        WAIT_USER_INPUT,
        WAIT_SERVER_ECHO,
        ABORTED
    } mEchoClientState;
    QString mLastSentMessage;

signals:
    void writeToConsole(const std::string& message);
    void writeToNetwork(const QString & message);

private slots:
    void readFromConsole(const std::string& message);
    void readFromNetwork(const QString & message);
    void starting();

};

#endif // ECHOCLIENTLOGIC_H
