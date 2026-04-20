#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <sqlite3.h>
#include <QString>
#include <QList>

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer();

    void startServer(int port);

protected:
    void incomingConnection(qintptr sd) override;

private:
    sqlite3* m_db;
    QMap<int, QTcpSocket*> m_idToSock;
    QMap<QTcpSocket*, int> m_sockToId;
    QMap<int, QString> m_idToName;
};

#endif // CHATSERVER_H