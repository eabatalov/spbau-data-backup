#ifndef THREADDELETER_H
#define THREADDELETER_H

#include <QObject>

#include "clientsessiononservercreator.h"

class ThreadDeleter : public QObject
{
    Q_OBJECT
public:
    explicit ThreadDeleter(QThread *thread, QObject *parent = 0);

signals:
    void deleteMe();

public slots:

private:
    QThread* mThread;

private slots:
    void deleteThread();

};

#endif // THREADDELETER_H
