#ifndef COMMANDLINEMANAGER_H
#define COMMANDLINEMANAGER_H

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QObject>
#include <QString>

class CommandLineManager : public QObject
{
    Q_OBJECT
public:
    explicit CommandLineManager(QCoreApplication &app, QObject *parent = 0);
    void process();

signals:

public slots:

private:
    QCommandLineParser parser;
    static const QString inputOption;
    static const QString outputOption;

};


#endif // COMMANDLINEMANAGER_H
