#include "clientlogic.h"
#include <archiver.h>
#include <fstream>

#include <iostream>
#include <cstring>
#include <string>
#include <time.h>
#include <vector>

#include <QFile>
#include <QFileInfo>


ClientLogic::ClientLogic(NetworkStream* networkStream, ConsoleStream* consoleStream, QObject *parent)
    :QObject(parent)
    ,mNetworkStream(networkStream)
    ,mConsoleStream(consoleStream)
    ,mClientState(NOT_STARTED)
{
    connect(mConsoleStream, &ConsoleStream::readln, this, &ClientLogic::slotReadFromConsole);
    connect(this, &ClientLogic::sigWriteToConsole, mConsoleStream, &ConsoleStream::println);
    connect(mNetworkStream,  &NetworkStream::newNetMessage, this, &ClientLogic::slotReadFromNetwork);
    connect(this, &ClientLogic::sigWriteToNetwork, mNetworkStream, &NetworkStream::sendNetMessage);
    connect(mNetworkStream, &NetworkStream::connected, this, &ClientLogic::slotStarting);
}

void ClientLogic::slotReadFromConsole(const std::string& message)
{
    if (mClientState == ABORTED)
    {
        std::cerr << "Current state - ABORTED. Unexpected user input.";
        return;
    }

    if (mClientState == WAIT_USER_INPUT)
    {
        if (message == "exit")
            exit(0);
        if (message == "help")
            emit sigWriteToConsole("Commands:\n"
                                   "ls [backupId] -- list all backups shortly or all info about one backup.\n"
                                   "restore backupId path -- download backup by backupId and restore it in path.\n"
                                   "backup path -- make backup");

        if (message == "ls")
        {
            askLs();
        }
        else if (message.substr(0, 2) == "ls")
        {
            askLs(message.substr(3));
        }
        else if (message.substr(0, 7) == "restore")
        {
            makeRestoreRequest(message.substr(8));
        }
        else if (message.substr(0, 6) == "backup")
        {
            makeBackup(message.substr(7));
        }
        else
        {
            emit sigWriteToConsole("Unknown command: " + message + "\nType \"help\" to show available commands.");
        }
    } else
    {
        emit sigWriteToConsole("Busy. Try later.");
        //should write current state?
    }
}


void ClientLogic::slotReadFromNetwork(const QByteArray &message)
{
    utils::commandType cmd = *((utils::commandType*)message.data());
    switch (cmd)
    {
    case utils::ansToLsSummary:
        OnSummaryLs(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToLsDetailed:
        OnDetailedLs(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToBackup:
        OnReceiveBackupResults(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToRestore:
        OnReceiveArchiveToRestore(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::serverExit:
        OnServerError(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    default:
        std::cerr << "Unsupported command from server." << std::endl;
    }
}


void ClientLogic::slotStarting()
{
    if (mClientState == NOT_STARTED)
    {
        mClientState = WAIT_USER_INPUT;
        emit sigWriteToConsole("Print command or \"exit\" to close app:");
    } else
    {
        mClientState = ABORTED;
        std::cerr << "Error, connecting after connected..." << std::endl;
    }
}

void ClientLogic::sendLsFromClient(networkUtils::protobufStructs::LsClientRequest ls)
{
    std::string serializationLs;
    if (!ls.SerializeToString(&serializationLs))
    {
        std::cerr << "Error with serialization LS cmd." << std::endl;
        return;
    }

    std::vector<char> message(ls.ByteSize() + utils::commandSize);
    utils::commandType cmd = utils::toFixedType(utils::ls);
    std::memcpy(&message.front(), (char*)&cmd, utils::commandSize);
    std::memcpy(&message.front() + utils::commandSize, serializationLs.c_str(), ls.ByteSize());

    std::cout << "Bytes: " << ls.ByteSize() << std::endl;
    std::cout << serializationLs.length() << std::endl;
    std::cout << serializationLs << std::endl;
    std::cout << message.size() << std::endl;

    QByteArray str = QByteArray(message.data(), message.size());
    std::cout << str.size() << std::endl;
    emit sigWriteToNetwork(str);
}

void ClientLogic::askLs()
{
    networkUtils::protobufStructs::LsClientRequest ls;
    ls.set_isls(1);
    sendLsFromClient(ls);
    mClientState = WAIT_LS_RESULT;
}

void ClientLogic::askLs(const std::string& command)
{
    networkUtils::protobufStructs::LsClientRequest ls;
    ls.set_isls(1);
    std::uint64_t backupId = 0;
    for (int i = 0; i < command.size(); ++i)
    {
        if (command[i] >= '0' && command[i] <= '9')
        {
            backupId = 10 * backupId + (command[i]-'0');
        }
        else
            break;
    }
    ls.set_backupid(backupId);
    sendLsFromClient(ls);
    mClientState = WAIT_LS_RESULT;
}

void ClientLogic::makeRestoreRequest(const std::string& command)
{
    networkUtils::protobufStructs::RestoreClientRequest restoreRequest;
    std::uint64_t backupId = 0;
    int i = 0;
    for (i = 0; i < command.size(); ++i)
    {
        if (command[i] >= '0' && command[i] <= '9')
        {
            backupId = 10 * backupId + (command[i]-'0');
        }
        else
            break;
    }
    ++i;
    restoreRequest.set_backupid(backupId);

    if (i >= command.size())
    {
        std::cerr << "Missing path in restore request" << std::endl;
        return;
    }

    restorePath = command.substr(i);

    std::string serializatedRestoreRequest;
    if (!restoreRequest.SerializeToString(&serializatedRestoreRequest))
    {
        std::cerr << "Error with serialization restore request cmd." << std::endl;
        return;
    }
    std::vector<char> message(serializatedRestoreRequest.size() + utils::commandSize);
    utils::commandType cmd = utils::toFixedType(utils::restore);
    std::memcpy(message.data(), (char*)&cmd, utils::commandSize);
    std::memcpy(message.data() + utils::commandSize, serializatedRestoreRequest.c_str(), serializatedRestoreRequest.size());
    emit sigWriteToNetwork(QByteArray(message.data()));

    mClientState = WAIT_RESTORE_RESULT;
}

void ClientLogic::makeBackup(const std::string& command)
{
    networkUtils::protobufStructs::ClientBackupRequest backupRequest;
    Archiver::pack(command.c_str(), "curpack.pck");
    QFile meta("curpack.pckmeta");
    QFile content("curpack.pckcontent");
    meta.open(QIODevice::ReadOnly);
    content.open(QIODevice::ReadOnly);
    backupRequest.set_archive(content.readAll().data());
    backupRequest.set_path(command);
    backupRequest.set_meta(meta.readAll().data());

    std::string serializatedBackupRequest;
    if (!backupRequest.SerializeToString(&serializatedBackupRequest))
    {
        std::cerr << "Error with serialization backup request cmd." << std::endl;
        return;
    }
    std::vector<char> message(serializatedBackupRequest.size() + utils::commandSize);
    utils::commandType cmd = utils::toFixedType(utils::backup);
    std::memcpy(message.data(), (char*)&cmd, utils::commandSize);
    std::memcpy(message.data() + utils::commandSize, serializatedBackupRequest.c_str(), serializatedBackupRequest.size());
    emit sigWriteToNetwork(QByteArray(message.data()));

    mClientState = WAIT_BACKUP_RESULT;
}

void ClientLogic::sendRestoreResult(bool restoreResult)
{
    networkUtils::protobufStructs::ClientReplyAfterRestore reply;
    reply.set_issuccess(restoreResult);

    std::string serializedReply;
    if (!reply.SerializeToString(&serializedReply))
    {
        std::cerr << "Error with serialization backup request cmd." << std::endl;
        return;
    }
    std::vector<char> message(serializedReply.size() + utils::commandSize);
    utils::commandType cmd = utils::toFixedType(utils::replyAfterRestore);
    std::memcpy(message.data(), (char*)&cmd, utils::commandSize);
    std::memcpy(message.data() + utils::commandSize, serializedReply.c_str(), serializedReply.size());
    emit sigWriteToNetwork(QByteArray(message.data()));
}

std::string inttostr(std::uint64_t number)
{
    if (number == 0)
        return "0";
    std::string S = "";
    while (number > 0)
    {
        S = std::string(1, '0'+(char)(number % 10)) + S;
        number /= 10;
    }
    return S;
}

std::string shortBackupInfoToString(const networkUtils::protobufStructs::ShortBackupInfo& backupInfo)
{
    time_t time = (time_t)backupInfo.time();
    return inttostr(backupInfo.backupid()) + " " + backupInfo.path() + " " + ctime(&time);
}

void ClientLogic::OnDetailedLs(const char *buffer, uint64_t bufferSize)
{
    if (mClientState == WAIT_LS_RESULT)
    {
        networkUtils::protobufStructs::LsDetailedServerAnswer lsDetailed;
        lsDetailed.ParseFromArray(buffer, bufferSize);
        //TODO: add fstree
        emit sigWriteToConsole(shortBackupInfoToString(lsDetailed.shortbackupinfo()));
        mClientState = WAIT_USER_INPUT;
    }
    else
    {
        std::cerr << "Unexpected DETAILED_LS_RESULT" << std::endl;
    }
}

void ClientLogic::OnSummaryLs(const char *buffer, uint64_t bufferSize)
{
    if (mClientState == WAIT_LS_RESULT)
    {
        networkUtils::protobufStructs::LsSummaryServerAnswer lsSummary;
        lsSummary.ParseFromArray(buffer, bufferSize);
        std::string ans;
        for (int i = 0; i < lsSummary.shortbackupinfo_size(); ++i)
        {
            ans += shortBackupInfoToString(lsSummary.shortbackupinfo(i)) + "\n";
        }
        ans += lsSummary.shortbackupinfo_size() + " backups.";
        emit sigWriteToConsole(ans);
        mClientState = WAIT_USER_INPUT;
    }
    else
    {
        std::cerr << "Unexpected SUMMURY_LS_RESULT" << std::endl;
    }
}

void ClientLogic::OnReceiveArchiveToRestore(const char *buffer, uint64_t bufferSize)
{
    if (mClientState == WAIT_RESTORE_RESULT)
    {
        networkUtils::protobufStructs::RestoreServerAnswer restoreAnswer;
        restoreAnswer.ParseFromArray(buffer, bufferSize);

        QFile meta("curunpack.pckmeta");
        QFile content("curunpack.pckcontent");
        meta.open(QIODevice::WriteOnly);
        content.open(QIODevice::WriteOnly);
        content.write(restoreAnswer.archive().c_str(), restoreAnswer.archive().size());
        content.write(restoreAnswer.meta().c_str(), restoreAnswer.meta().size());

        QFileInfo qfileinfo(restorePath.c_str());
        if (qfileinfo.isDir())
        {
            Archiver::unpack("curunpack.pck", restorePath.c_str());
            sendRestoreResult(1);
        }
        else
        {
            std::cerr << restorePath << " isn't directory." << std::endl;
            sendRestoreResult(0);
        }

        //TODO: make more meaningful
        //sendRestoreResult(1);
        mClientState = WAIT_USER_INPUT;
    }
    else
    {
        std::cerr << "Unexpected RESTORE_RESULT" << std::endl;
    }

}

void ClientLogic::OnReceiveBackupResults(const char *buffer, uint64_t bufferSize)
{
    if (mClientState == WAIT_BACKUP_RESULT)
    {
        networkUtils::protobufStructs::ServerBackupResult backupResult;
        backupResult.ParseFromArray(buffer, bufferSize);
        if (backupResult.issuccess())
        {
            emit sigWriteToConsole("Backup was succed. BackupId = " + inttostr(backupResult.backupid()));
        }
        else
        {
            emit sigWriteToConsole("Backup wasn't succed.");
        }
        mClientState = WAIT_USER_INPUT;
    }
    else
    {
        std::cerr << "Unexpected BACKUP_RESULT" << std::endl;
    }

}

void ClientLogic::OnServerError(const char *buffer, uint64_t bufferSize)
{
    networkUtils::protobufStructs::ServerError serverError;
    serverError.ParseFromArray(buffer, bufferSize);
    emit sigWriteToConsole("Server error: " + serverError.errormessage() + "\nState is aborted.");
    mClientState = ABORTED;
}
