#include "echoclientlogic.h"
#include <iostream>

EchoClientLogic::EchoClientLogic(NetworkStream* networkStream, ConsoleStream* consoleStream, QObject *parent)
    : QObject(parent)
    , mNetworkStream(networkStream)
    , mConsoleStream(consoleStream)
    , mEchoClientState(NOT_STARTED)
{
    connect(mConsoleStream, &ConsoleStream::readln, this, &EchoClientLogic::readFromConsole);
    connect(this, &EchoClientLogic::writeToConsole, mConsoleStream, &ConsoleStream::println);
    connect(mNetworkStream,  &NetworkStream::newNetMessage, this, &EchoClientLogic::readFromNetwork);
    connect(this, &EchoClientLogic::writeToNetwork, mNetworkStream, &NetworkStream::sendNetMessage);
    connect(mNetworkStream, &NetworkStream::connected, this, &EchoClientLogic::starting);
}

void EchoClientLogic::readFromConsole(const std::string &message)
{
    if (mEchoClientState == WAIT_USER_INPUT)
    {
        mEchoClientState = WAIT_SERVER_ECHO;
        if (message == "exit")
            exit(0);
        mLastSentMessage = QByteArray::fromStdString(message);
        emit writeToNetwork(mLastSentMessage);
    } else
    {
        mEchoClientState = ABORTED;
        std::cerr << "Something wrong state. Expected WAIT_USER_INPUT" << std::endl;
    }
}

void EchoClientLogic::readFromNetwork(const QByteArray &message)
{
    if (mEchoClientState == WAIT_SERVER_ECHO)
    {
        mEchoClientState = WAIT_USER_INPUT;
        if (message == mLastSentMessage)
        {
            emit writeToConsole("Echo is completed. Print new message or \"exit\" to close app:");
        } else
        {
            mEchoClientState = ABORTED;
            std::cerr << "Echo doesn't work..." << std::endl;
        }
    } else
    {
        mEchoClientState = ABORTED;
        std::cerr << "Something wrong state. Expected WAIT_SERVER_ECHO" << std::endl;
    }
}

void EchoClientLogic::starting()
{
    if (mEchoClientState == NOT_STARTED)
    {
        mEchoClientState = WAIT_USER_INPUT;
        emit writeToConsole("Print message or \"exit\" to close app:");
    } else
    {
        mEchoClientState = ABORTED;
        std::cerr << "Error, connecting after connecting..." << std::endl;
    }
}
