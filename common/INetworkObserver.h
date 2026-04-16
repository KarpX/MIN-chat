#ifndef INETWORKOBSERVER_H
#define INETWORKOBSERVER_H
#include <QByteArray>

class INetworkObserver {
public:
    virtual ~INetworkObserver() = default;
    virtual void onMessageReceived(const QByteArray &data) = 0;
    virtual void onStatusChanged(const QString &status) = 0;
};
#endif // INETWORKOBSERVER_H