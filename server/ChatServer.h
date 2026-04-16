#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr) : QTcpServer(parent) {}

    void startServer(int port) {
        if (this->listen(QHostAddress::Any, port)) {
            qDebug() << "Сервер запущен на порту" << port;
        }
    }

protected:
    void incomingConnection(qintptr socketDescriptor) override {
        QTcpSocket* socket = new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);

        m_clients.append(socket);
        qDebug() << "Новый клиент подключился. Всего клиентов:" << m_clients.size();

        // connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        //     QByteArray data = socket->readAll();
        //     qDebug() << "Сервер получил данные и пересылает их...";
        //     for (QTcpSocket* client : m_clients) {
        //         client->write(data);
        //     }
        // });

        // Внутри ChatServer.h в обработчике readyRead:
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            QByteArray data = socket->readAll();

            // 1. Если это системное приветствие
            if (data == "SYS_HELLO_CLIENT") {
                qDebug() << "Handshake от клиента. Отвечаем OK.";
                socket->write("SYS_HANDSHAKE_OK");
                socket->flush();
                return;
            }

            // 2. Если это обычное зашифрованное сообщение
            qDebug() << "Сервер пересылает сообщение:" << data;

            int count = 0;
            for (QTcpSocket* client : m_clients) {
                // Мы отправляем сообщение ВСЕМ клиентам, включая отправителя (для теста)
                if (client->state() == QAbstractSocket::ConnectedState) {
                    client->write(data);
                    client->flush(); // Выталкиваем данные немедленно
                    count++;
                }
            }
            qDebug() << "Разослано клиентам:" << count;
        });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_clients.removeOne(socket);
            socket->deleteLater();
            qDebug() << "Клиент отключился.";
        });
    }

private:
    QList<QTcpSocket*> m_clients;
};

#endif // CHATSERVER_H
