#ifndef CONNECTIONSTATES_H
#define CONNECTIONSTATES_H
#include <QString>
#include <qDebug>

#include "IConnectionState.h"

class HandshakeState : public IConnectionState {
public:
    void send(NetworkClient* context, const QByteArray &data) override {
        qDebug() << "Ошибка: Нельзя отправить сообщение, идет обмен ключами!";
    }
    QString stateName() const override { return "Обмен ключами..."; }
};

class OnlineState : public IConnectionState {
public:
    void send(NetworkClient* context, const QByteArray &data) override; // Реализация ниже
    QString stateName() const override { return "В сети"; }
};

#endif // CONNECTIONSTATES_H
