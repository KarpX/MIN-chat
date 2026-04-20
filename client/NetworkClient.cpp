#include "NetworkClient.h"
#include "ConnectionStates.h"

NetworkClient::NetworkClient(QObject *parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);
    m_state = std::make_unique<HandshakeState>();

    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, [this](){
        this->setState(std::make_unique<OnlineState>());
        this->rawSend("SYS_HELLO_CLIENT\n");
    });
}

void NetworkClient::setState(std::unique_ptr<IConnectionState> newState) {
    m_state = std::move(newState);
    notifyStatus(m_state->stateName());
}

void NetworkClient::onReadyRead() {
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        if (line == "SYS_HANDSHAKE_OK") {
            this->setState(std::make_unique<OnlineState>());
        } else {
            for(auto* obs : m_observers) {
                obs->onMessageReceived(line);
            }
        }
    }
}

void NetworkClient::rawSend(const QByteArray &data) {
    if(m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
        m_socket->flush();
    }
}

void NetworkClient::connectToServer(const QString &host, int port) {
    m_socket->connectToHost(host, port);
}