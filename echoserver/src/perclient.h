#ifndef PERCLIENT_H
#define PERCLIENT_H

#include "../../echoclient/src/networkstream.h"
#include <QTcpSocket>

class PerClient : public NetworkStream
{
    Q_OBJECT
public:
    PerClient(size_t clientNumber, QTcpSocket *clientSocket, QObject* parent = 0);

private:
    size_t mClientNumber;

signals:
    void newClientMessege(size_t clientNumber, const QString & message);
    void releaseClientPlace(size_t clientNumber);

public slots:
    void slotSendClientMessege(size_t clientNumber, const QString & message);

private slots:
    void slotNewNetMessege(const QString & message);
    void disconnectSocket();
};

#endif // PERCLIENT_H
