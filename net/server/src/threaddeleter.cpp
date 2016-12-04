#include "threaddeleter.h"
#include <QThread>

ThreadDeleter::ThreadDeleter(QThread* thread, QObject *parent) : QObject(parent), mThread(thread)
{
    connect(thread, &QThread::finished, this, &ThreadDeleter::deleteThread);
    connect(this, &ThreadDeleter::deleteMe, this, &ThreadDeleter::deleteLater);
}

void ThreadDeleter::deleteThread() {
    delete mThread;
    emit deleteMe();
}
