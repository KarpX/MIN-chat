#include "ConnectionStates.h"
#include "NetworkClient.h"
#include <QDebug>
void HandshakeState::send(NetworkClient* c, const QByteArray &d) {
    Q_UNUSED(c); Q_UNUSED(d);
    qDebug() << "Wait! Handshake in progress...";
}
QString HandshakeState::stateName() const { return "Handshake"; }
void OnlineState::send(NetworkClient* c, const QByteArray &d) {
    if(c) c->rawSend(d);
}
QString OnlineState::stateName() const { return "Online"; }