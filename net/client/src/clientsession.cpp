#include "clientsession.h"
#include <archiver.h>
#include <fstream>

#include <iostream>
#include <cstring>
#include <string>
#include <time.h>
#include <vector>

#include <QFile>
#include <QFileInfo>
#include <QByteArray>


#define DEBUG_FLAG true
#define LOG(format, var) do{ \
    if (DEBUG_FLAG) fprintf(stderr, "line: %d.  " format, __LINE__ ,var); \
    }while(0)

ClientSession::ClientSession(NetworkStream* networkStream, ConsoleStream* consoleStream, QObject *parent)
    :QObject(parent)
    ,mClientState(NOT_STARTED) {
    consoleStream->setParent(this);
    networkStream->setParent(this);
    connect(consoleStream, &ConsoleStream::readln, this, &ClientSession::onConsoleInput);
    connect(this, &ClientSession::sigWriteToConsole, consoleStream, &ConsoleStream::println);
    connect(networkStream,  &NetworkStream::newNetMessage, this, &ClientSession::onNetworkInput);
    connect(this, &ClientSession::sigWriteToNetwork, networkStream, &NetworkStream::sendNetMessage);
    connect(networkStream, &NetworkStream::connected, this, &ClientSession::onStart);
}

void ClientSession::onConsoleInput(const std::string& message) {
    if (mClientState == ABORTED) {
        std::cerr << "Current state - ABORTED. Unexpected user input.";
        return;
    }

    if (mClientState == WAIT_LOGIN_INPUT) {
        if (message.size() > 4 && message.substr(0, 4) == "reg ") {
            sendRegistrationRequest(message.substr(4));
        } else {
            sendLoginRequest(message);
        }
        return;
    }

    if (mClientState != WAIT_USER_INPUT) {
        emit sigWriteToConsole("Busy. Try later.");
        return;
    }


    if (message == "exit")
        exit(0);
    if (message == "help") {
        emit sigWriteToConsole("Commands:\n"
                               "ls [backupId] -- list all backups shortly or all info about one backup.\n"
                               "restore backupId path -- download backup by backupId and restore it in path.\n"
                               "backup path -- make backup.");

    }

    else if (message == "ls") {
        askLs();
    } else if (message.substr(0, 2) == "ls") {
        askLs(message.substr(3));
    } else if (message.substr(0, 7) == "restore") {
        makeRestoreRequest(message.substr(8));
    } else if (message.substr(0, 6) == "backup") {
        makeBackup(message.substr(7));
    } else {
        emit sigWriteToConsole("Unknown command: " + message + "\nType \"help\" to show available commands.");
    }
}


void ClientSession::onNetworkInput(const QByteArray &message) {
    utils::commandType cmd = *((utils::commandType*)message.data());
    switch (cmd) {
    case utils::ansToClientRegister:
        procRegistrationAns(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToClientLogin:
        procLoginAns(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToLsSummary:
        procSummaryLs(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToLsDetailed:
        procDetailedLs(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToBackup:
        procReceiveBackupResults(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::ansToRestore:
        procReceiveArchiveToRestore(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::serverExit:
        procServerError(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::notFoundBackupByIdOnServer:
        procNotFoundBackupId(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    case utils::serverError:
        procServerExit(message.data()+utils::commandSize, message.size()-utils::commandSize);
        break;
    default:
        std::cerr << "Unsupported command from server." << std::endl;
    }
}


void ClientSession::onStart() {
    if (mClientState == NOT_STARTED) {
        mClientState = WAIT_LOGIN_INPUT;
        emit sigWriteToConsole("Print login and password:");
    } else {
        mClientState = ABORTED;
        std::cerr << "Error, connecting after connected..." << std::endl;
    }
}

void ClientSession::sendRegistrationRequest(const std::string & loginAndPassword) {
    networkUtils::protobufStructs::LoginClientRequest loginRequest;
    size_t spacePosition = loginAndPassword.find(' ');
    if (spacePosition <= 0 || spacePosition >= loginAndPassword.size()) {
        emit sigWriteToConsole("Error! Please, print login and password dividing by space:");
        return;
    }
    std::string login = loginAndPassword.substr(0, spacePosition);
    std::string password = loginAndPassword.substr(spacePosition + 1);
    loginRequest.set_login(login);
    loginRequest.set_password(password);

    std::string serializatedLoginRequest;
    if (!loginRequest.SerializeToString(&serializatedLoginRequest)) {
        std::cerr << "Error with serialization login request cmd." << std::endl;
        return;
    }
    sendSerializatedMessage(serializatedLoginRequest, utils::clientRegister, loginRequest.ByteSize());
    mClientState = WAIT_LOGIN_STATUS;
}

void ClientSession::sendLoginRequest(const std::string& loginAndPassword) {
    networkUtils::protobufStructs::LoginClientRequest loginRequest;
    size_t spacePosition = loginAndPassword.find(' ');
    if (spacePosition <= 0 || spacePosition >= loginAndPassword.size()) {
        emit sigWriteToConsole("Error! Please, print login and password dividing by space:");
        return;
    }
    std::string login = loginAndPassword.substr(0, spacePosition);
    std::string password = loginAndPassword.substr(spacePosition + 1);
    loginRequest.set_login(login);
    loginRequest.set_password(password);

    std::string serializatedLoginRequest;
    if (!loginRequest.SerializeToString(&serializatedLoginRequest)) {
        std::cerr << "Error with serialization login request cmd." << std::endl;
        return;
    }
    sendSerializatedMessage(serializatedLoginRequest, utils::clientLogin, loginRequest.ByteSize());
    mClientState = WAIT_LOGIN_STATUS;
}

void ClientSession::sendLsFromClient(networkUtils::protobufStructs::LsClientRequest ls) {
    std::string serializationLs;
    if (!ls.SerializeToString(&serializationLs)) {
        std::cerr << "Error with serialization LS cmd." << std::endl;
        return;
    }
    sendSerializatedMessage(serializationLs, utils::ls, ls.ByteSize());
}

void ClientSession::askLs() {
    networkUtils::protobufStructs::LsClientRequest ls;
    ls.set_isls(1);
    sendLsFromClient(ls);
    mClientState = WAIT_LS_RESULT;
}

void ClientSession::askLs(const std::string& command) {
    networkUtils::protobufStructs::LsClientRequest ls;
    ls.set_isls(1);
    std::uint64_t backupId = std::stoull(command);
    ls.set_backupid(backupId);
    sendLsFromClient(ls);
    mClientState = WAIT_LS_RESULT;
}

void ClientSession::makeRestoreRequest(const std::string& command) {
    networkUtils::protobufStructs::RestoreClientRequest restoreRequest;
    size_t indexAfterNumber = 0;
    std::uint64_t backupId = std::stoull(command, &indexAfterNumber);
    ++indexAfterNumber;
    restoreRequest.set_backupid(backupId);

    if (indexAfterNumber >= command.size()) {
        std::cerr << "Missing path in restore request" << std::endl;
        return;
    }

    restorePath = command.substr(indexAfterNumber);

    std::string serializatedRestoreRequest;
    if (!restoreRequest.SerializeToString(&serializatedRestoreRequest)) {
        std::cerr << "Error with serialization restore request cmd." << std::endl;
        return;
    }
    sendSerializatedMessage(serializatedRestoreRequest, utils::restore, restoreRequest.ByteSize());
    mClientState = WAIT_RESTORE_RESULT;
    LOG("Send restore request. Size = %d\n", restoreRequest.ByteSize());
}

void ClientSession::makeBackup(const std::string& command) {
    networkUtils::protobufStructs::ClientBackupRequest backupRequest;
    Archiver::pack(command.c_str(), "curpack.pck");
    QFile archive("curpack.pck");
    archive.open(QIODevice::ReadOnly);
    QByteArray archiveByte = archive.readAll();
    backupRequest.set_archive(archiveByte.data(), archiveByte.size());
    backupRequest.set_path(command);

    std::string serializatedBackupRequest;
    if (!backupRequest.SerializeToString(&serializatedBackupRequest)) {
        std::cerr << "Error with serialization backup request cmd." << std::endl;
        return;
    }
    sendSerializatedMessage(serializatedBackupRequest, utils::backup, backupRequest.ByteSize());

    LOG("archivesize = %ld\n", backupRequest.archive().size());

    mClientState = WAIT_BACKUP_RESULT;
}

void ClientSession::sendRestoreResult(bool restoreResult) {
    networkUtils::protobufStructs::ClientReplyAfterRestore reply;
    reply.set_issuccess(restoreResult);

    std::string serializedReply;
    if (!reply.SerializeToString(&serializedReply)) {
        std::cerr << "Error with serialization backup request cmd." << std::endl;
        return;
    }
    sendSerializatedMessage(serializedReply, utils::replyAfterRestore, reply.ByteSize());
    mClientState = WAIT_USER_INPUT;
    LOG("Send replyAfterRestore. Size = %d\n", reply.ByteSize());
}

void ClientSession::procRegistrationAns(const char *buffer, uint64_t bufferSize) {
    if (mClientState != WAIT_LOGIN_STATUS) {
        std::cerr << "Unexpected RegistrationAns" << std::endl;
        mClientState = ABORTED;
        return;
    }

    networkUtils::protobufStructs::LoginServerAnswer serverAnswer;
    serverAnswer.ParseFromArray(buffer, bufferSize);
    if (serverAnswer.issuccess()) {
        mClientState = WAIT_USER_INPUT;
        emit sigWriteToConsole("Registration was success. Print command or \"exit\" to close app:");
    } else {
        mClientState = WAIT_LOGIN_INPUT;
        emit sigWriteToConsole("Registration was unsuccess. Print login and password:");
    }
}

void ClientSession::procLoginAns(const char *buffer, uint64_t bufferSize) {
    if (mClientState != WAIT_LOGIN_STATUS) {
        std::cerr << "Unexpected LoginAns" << std::endl;
        mClientState = ABORTED;
        return;
    }

    networkUtils::protobufStructs::LoginServerAnswer serverAnswer;
    serverAnswer.ParseFromArray(buffer, bufferSize);
    if (serverAnswer.issuccess()) {
        mClientState = WAIT_USER_INPUT;
        emit sigWriteToConsole("Login was success. Print command or \"exit\" to close app:");
    } else {
        mClientState = WAIT_LOGIN_INPUT;
        emit sigWriteToConsole("Login was unsuccess. Print login and password:");
    }
}

std::string shortBackupInfoToString(const networkUtils::protobufStructs::ShortBackupInfo& backupInfo) {
    time_t time = (time_t)backupInfo.time();
    return std::to_string(backupInfo.backupid()) + " " + backupInfo.path() + " " + ctime(&time);
}

void ClientSession::procDetailedLs(const char *buffer, uint64_t bufferSize) {
    if (mClientState != WAIT_LS_RESULT) {
        std::cerr << "Unexpected DETAILED_LS_RESULT" << std::endl;
        mClientState = ABORTED;
        return;
    }
    networkUtils::protobufStructs::LsDetailedServerAnswer lsDetailed;
    lsDetailed.ParseFromArray(buffer, bufferSize);

    QFile meta("curpack.pck");
    meta.open(QIODevice::WriteOnly);
    meta.write(lsDetailed.meta().c_str(), lsDetailed.meta().size());
    meta.close();
    LOG("lsDetailed.meta().size() = %ld\n", lsDetailed.meta().size());

    QString fsTree;

    QTextStream qTextStream(&fsTree,  QIODevice::WriteOnly);
    Archiver::printArchiveFsTree("curpack.pck", qTextStream);
    qTextStream.flush();

    LOG("fsTree.size() = %d\n", fsTree.size());

    std::string temp = shortBackupInfoToString(lsDetailed.shortbackupinfo()) + "\n" + fsTree.toStdString();
    emit sigWriteToConsole(temp);

    mClientState = WAIT_USER_INPUT;
}

void ClientSession::procSummaryLs(const char *buffer, uint64_t bufferSize) {
    if (mClientState != WAIT_LS_RESULT) {
        std::cerr << "Unexpected SUMMURY_LS_RESULT" << std::endl;
        mClientState = ABORTED;
        return;
    }

    networkUtils::protobufStructs::LsSummaryServerAnswer lsSummary;
    lsSummary.ParseFromArray(buffer, bufferSize);

    std::string ans;
    for (int i = 0; i < lsSummary.shortbackupinfo_size(); ++i) {
        ans += shortBackupInfoToString(lsSummary.shortbackupinfo(i));
    }
    ans += std::to_string(lsSummary.shortbackupinfo_size()) + " backups.";
    emit sigWriteToConsole(ans);
    mClientState = WAIT_USER_INPUT;
}

void ClientSession::procReceiveArchiveToRestore(const char *buffer, uint64_t bufferSize) {
    if (mClientState != WAIT_RESTORE_RESULT) {
        std::cerr << "Unexpected RESTORE_RESULT" << std::endl;
        mClientState = ABORTED;
        return;
    }

    networkUtils::protobufStructs::RestoreServerAnswer restoreAnswer;
    restoreAnswer.ParseFromArray(buffer, bufferSize);
    LOG("Receive restore. Size = %d\n", restoreAnswer.ByteSize());
    LOG("archivesize = %ld\n", restoreAnswer.archive().size());

    QFile archive("curunpack.pck");
    archive.open(QIODevice::WriteOnly);
    archive.write(restoreAnswer.archive().c_str(), restoreAnswer.archive().size());
    archive.close();

    LOG("restorePath: %s\n", restorePath.c_str());
    QFileInfo qfileinfo(restorePath.c_str());
    LOG("Absolute restorePath: %s\n", qfileinfo.canonicalFilePath().toStdString().c_str());
    try {
        if (qfileinfo.isDir()) {
            Archiver::unpack("curunpack.pck", restorePath.c_str());
            sendRestoreResult(1);
        } else {
            std::cerr << restorePath << " isn't directory." << std::endl;
            sendRestoreResult(0);
        }
    } catch (Archiver::ArchiverException e) {
        std::cerr << e.whatQMsg().toStdString() << std::endl;
        sendRestoreResult(0);
    }

    mClientState = WAIT_USER_INPUT;
}

void ClientSession::procNotFoundBackupId(const char *buffer, uint64_t bufferSize) {
    networkUtils::protobufStructs::ServerError serverError;
    serverError.ParseFromArray(buffer, bufferSize);

    emit sigWriteToConsole(serverError.errormessage());

    mClientState = WAIT_USER_INPUT;
}


void ClientSession::procReceiveBackupResults(const char *buffer, uint64_t bufferSize) {
    if (mClientState != WAIT_BACKUP_RESULT) {
        std::cerr << "Unexpected BACKUP_RESULT" << std::endl;
        mClientState = ABORTED;
        return;
    }

    networkUtils::protobufStructs::ServerBackupResult backupResult;
    backupResult.ParseFromArray(buffer, bufferSize);
    if (backupResult.issuccess()) {
        emit sigWriteToConsole("Backup was succed. BackupId = " + std::to_string(backupResult.backupid()));
    } else {
        emit sigWriteToConsole("Backup wasn't succed.");
    }
    mClientState = WAIT_USER_INPUT;
}

void ClientSession::procServerError(const char *buffer, uint64_t bufferSize) {
    networkUtils::protobufStructs::ServerError serverError;
    serverError.ParseFromArray(buffer, bufferSize);
    emit sigWriteToConsole("Server error: " + serverError.errormessage());
    mClientState = WAIT_USER_INPUT;
}

void ClientSession::procServerExit(const char *buffer, uint64_t bufferSize) {
    networkUtils::protobufStructs::ServerError serverError;
    serverError.ParseFromArray(buffer, bufferSize);
    emit sigWriteToConsole("Server error: " + serverError.errormessage() + "\nState is aborted.");
    mClientState = ABORTED;
}

void ClientSession::sendSerializatedMessage(const std::string& binaryMessage, utils::commandType cmdType, int messageSize) {
    std::vector<char> message(messageSize + utils::commandSize);
    std::memcpy(message.data(), (char*)&cmdType, utils::commandSize);
    std::memcpy(message.data() + utils::commandSize, binaryMessage.c_str(), messageSize);
    emit sigWriteToNetwork(QByteArray(message.data(), messageSize + utils::commandSize));
}
