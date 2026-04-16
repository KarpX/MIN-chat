#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H

#include <QObject>
#include <QDebug>
#include "../common/CryptoManager.h"
#include "../common/AES256GCMStrategy.h"
#include "NetworkClient.h"
#include "../common/INetworkObserver.h"

// ChatController наследует INetworkObserver, поэтому тут override РАБОТАЕТ
class ChatController : public QObject, public INetworkObserver {
    Q_OBJECT
public:
    explicit ChatController(QObject *parent = nullptr) : QObject(parent) {
        m_crypto.setStrategy(std::make_unique<AES256GCMStrategy>());
        m_network.addObserver(this);
        m_network.connectToServer("127.0.0.1", 8080);
    }

    Q_INVOKABLE void sendMessage(const QString &text) {
        QByteArray encrypted = m_crypto.encryptData(text.toUtf8(), "secret_key");
        m_network.sendMessage(encrypted);
    }

    void onMessageReceived(const QByteArray &data) override {
        QByteArray decrypted = m_crypto.decryptData(data, "secret_key");
        QString message = QString::fromUtf8(decrypted);
        qDebug() << "--- [КОНТРОЛЛЕР] Получено и расшифровано:" << message;
        emit newMessageReceived(message);
    }

    void onStatusChanged(const QString &status) override {
        qDebug() << "--- [КОНТРОЛЛЕР] Статус сети:" << status;
        emit networkStatusChanged(status);
    }

signals:
    void newMessageReceived(QString text);
    void networkStatusChanged(QString status);

private:
    CryptoManager m_crypto;
    NetworkClient m_network;
};

#endif // CHATCONTROLLER_H