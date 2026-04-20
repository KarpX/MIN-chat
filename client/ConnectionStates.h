#ifndef CONNECTIONSTATES_H
#define CONNECTIONSTATES_H

#include "../common/IConnectionState.h"
#include <QString>
#include <QByteArray>
class NetworkClient;
class HandshakeState : public IConnectionState {
public:
    void send(NetworkClient* context, const QByteArray &data) override;
    QString stateName() const override;
};
class OnlineState : public IConnectionState {
public:
    void send(NetworkClient* context, const QByteArray &data) override;
    QString stateName() const override;
};

#endif