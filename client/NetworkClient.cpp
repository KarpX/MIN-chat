#include "NetworkClient.h"
#include "../common/ConnectionStates.h"

// NetworkClient::NetworkClient(QObject *parent) : QObject(parent) {
//     m_socket = new QTcpSocket(this);
//     m_state = std::make_unique<HandshakeState>();

//     connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
//     connect(m_socket, &QTcpSocket::connected, this, [this](){
//         this->setState(std::make_unique<OnlineState>());
//     });
// }

// void NetworkClient::onReadyRead() {
//     QByteArray data = m_socket->readAll();
//     for(auto* obs : m_observers) obs->onMessageReceived(data);
// }

NetworkClient::NetworkClient(QObject *parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);
    m_state = std::make_unique<HandshakeState>();

    connect(m_socket, &QTcpSocket::connected, this, [this](){
        qDebug() << "--- [ШАГ 1] Сокет подключился. Отправляем приветствие...";
        this->rawSend("SYS_HELLO_CLIENT");
    });

    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
}

void NetworkClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    qDebug() << "--- [СЕТЬ] Получены сырые данные:" << data;

    if (data == "SYS_HANDSHAKE_OK") {
        this->setState(std::make_unique<OnlineState>());
        return;
    }

    if (m_observers.empty()) {
        qDebug() << "!!! ВНИМАНИЕ: У NetworkClient нет наблюдателей!";
    }

    for(auto* obs : m_observers) {
        obs->onMessageReceived(data);
    }
}

void NetworkClient::rawSend(const QByteArray &data) {
    if(m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
    }
}

void NetworkClient::connectToServer(const QString &host, int port) {
    m_socket->connectToHost(host, port);
}

void OnlineState::send(NetworkClient* context, const QByteArray &data) {
    context->rawSend(data);
}

void NetworkClient::setState(std::unique_ptr<IConnectionState> newState) {
    m_state = std::move(newState);
    notifyStatus(m_state->stateName());
}