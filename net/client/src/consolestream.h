#ifndef CONSOLESTREAM_H
#define CONSOLESTREAM_H

#include <QObject>
#include <string>
#include <QSocketNotifier>

class ConsoleStream : public QObject {
    Q_OBJECT
public:
    explicit ConsoleStream(QObject *parent = 0);

signals:
    void readln(const std::string& str);

public slots:
    void println(const std::string& str);

private:
    QSocketNotifier stdinNotifier;

private slots:
    void newInputString();
};

#endif // CONSOLESTREAM_H
