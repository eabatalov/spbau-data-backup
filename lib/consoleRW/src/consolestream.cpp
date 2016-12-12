#include "consolestream.h"
#include <iostream>
#include <string>
#include <cassert>
#include <unistd.h>
#include <QTextStream>

ConsoleStream::ConsoleStream(QObject *parent)
    : QObject(parent)
    , stdinNotifier(STDIN_FILENO, QSocketNotifier::Read) {
    connect(&stdinNotifier, &QSocketNotifier::activated, this, &ConsoleStream::newInputString);
}

void ConsoleStream::println(const std::string& str) {
    std::cout << str << std::endl;
}

void ConsoleStream::newInputString() {
    QTextStream qin(stdin);
    QString line = qin.readLine();
    emit readln(line.toStdString());
}
