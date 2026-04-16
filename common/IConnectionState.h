#ifndef ICONNECTIONSTATE_H
#define ICONNECTIONSTATE_H
#include <QByteArray>
#include <QString>

class NetworkClient;

class IConnectionState {
public:
    virtual ~IConnectionState() = default;
    virtual void send(NetworkClient* context, const QByteArray &data) = 0;
    virtual QString stateName() const = 0;
};
#endif // ICONNECTIONSTATE_H
