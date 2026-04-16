#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QTcpSocket>
#include <QObject>
#include <memory>
#include <vector>
#include "../common/INetworkObserver.h"
#include "../common/IConnectionState.h"

class NetworkClient : public QObject {
    Q_OBJECT
public:
    explicit NetworkClient(QObject *parent = nullptr);

    void addObserver(INetworkObserver* obs) { m_observers.push_back(obs); }

    void setState(std::unique_ptr<IConnectionState> newState);

    void sendMessage(const QByteArray &data) { m_state->send(this, data); }
    void rawSend(const QByteArray &data);
    void connectToServer(const QString &host, int port);

private slots:
    void onReadyRead();

private:
    void notifyStatus(const QString &status) {
        for(auto* obs : m_observers) obs->onStatusChanged(status);
    }

    QTcpSocket* m_socket;
    std::unique_ptr<IConnectionState> m_state;
    std::vector<INetworkObserver*> m_observers;

    friend class OnlineState;
    friend class HandshakeState;
};

#endif // NETWORKCLIENT_H