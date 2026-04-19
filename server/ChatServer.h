#ifndef CHATSERVER_H
#define CHATSERVER_H
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <sqlite3.h>
#include <QDebug>
#include <QDateTime>

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr) : QTcpServer(parent) {
        sqlite3_open("server_data.db", &m_db);
        sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, password TEXT);", 0, 0, 0);
        sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS off_msgs (sid INTEGER, rid INTEGER, sname TEXT, data TEXT, time TEXT);", 0, 0, 0);
    }
    ~ChatServer() { sqlite3_close(m_db); }
    void startServer(int port) { if (this->listen(QHostAddress::Any, port)) qDebug() << "Сервер запущен на порту:" << port; }

protected:
    void incomingConnection(qintptr sd) override {
        QTcpSocket* s = new QTcpSocket(this);
        s->setSocketDescriptor(sd);
        connect(s, &QTcpSocket::readyRead, this, [this, s]() {
            while (s->canReadLine()) {
                QByteArray line = s->readLine().trimmed();
                QList<QByteArray> p = line.split('|');
                if (p.isEmpty()) continue;
                QByteArray cmd = p[0];

                if (cmd == "REG" || cmd == "LOGIN") {
                    sqlite3_stmt* st;
                    if(cmd == "REG") sqlite3_prepare_v2(m_db, "INSERT INTO users (username, password) VALUES (?, ?);", -1, &st, 0);
                    else sqlite3_prepare_v2(m_db, "SELECT id FROM users WHERE username=? AND password=?;", -1, &st, 0);
                    sqlite3_bind_text(st, 1, p[1].constData(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(st, 2, p[2].constData(), -1, SQLITE_TRANSIENT);

                    if (sqlite3_step(st) == (cmd == "REG" ? SQLITE_DONE : SQLITE_ROW)) {
                        int id = (cmd == "REG") ? (int)sqlite3_last_insert_rowid(m_db) : sqlite3_column_int(st, 0);
                        m_idToSock[id] = s; m_sockToId[s] = id; m_idToName[id] = QString::fromUtf8(p[1]);
                        s->write("AUTH_OK|" + QByteArray::number(id) + "|" + p[1] + "\n");

                        sqlite3_stmt* ost;
                        sqlite3_prepare_v2(m_db, "SELECT sid, sname, data, time FROM off_msgs WHERE rid=?;", -1, &ost, 0);
                        sqlite3_bind_int(ost, 1, id);
                        while(sqlite3_step(ost) == SQLITE_ROW) {
                            s->write("MSG|" + QByteArray::number(sqlite3_column_int(ost, 0)) + "|" + (const char*)sqlite3_column_text(ost, 1) + "|" + (const char*)sqlite3_column_text(ost, 2) + "|" + (const char*)sqlite3_column_text(ost, 3) + "\n");
                        }
                        sqlite3_finalize(ost);
                        sqlite3_exec(m_db, QString("DELETE FROM off_msgs WHERE rid=%1").arg(id).toUtf8().constData(), 0, 0, 0);
                    } else s->write("AUTH_ERR|Error\n");
                    sqlite3_finalize(st);
                } else if (cmd == "SEARCH") {
                    sqlite3_stmt* st;
                    sqlite3_prepare_v2(m_db, "SELECT id, username FROM users WHERE username LIKE ? LIMIT 8;", -1, &st, 0);
                    QString pat = QString::fromUtf8(p[1]) + "%";
                    sqlite3_bind_text(st, 1, pat.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    while (sqlite3_step(st) == SQLITE_ROW) {
                        s->write("SRCH_RES|" + QByteArray::number(sqlite3_column_int(st, 0)) + "|" + (const char*)sqlite3_column_text(st, 1) + "\n");
                    }
                    sqlite3_finalize(st);
                } else if (cmd == "MSG") {
                    int rid = p[1].toInt(); int sid = m_sockToId[s];
                    QString curTime = QDateTime::currentDateTime().toString("hh:mm"); // ФОРМАТ HH:MM
                    QByteArray msg = "MSG|" + QByteArray::number(sid) + "|" + m_idToName[sid].toUtf8() + "|" + p[2] + "|" + curTime.toUtf8() + "\n";
                    if (m_idToSock.contains(rid)) m_idToSock[rid]->write(msg);
                    else {
                        sqlite3_stmt* ost;
                        sqlite3_prepare_v2(m_db, "INSERT INTO off_msgs (sid, rid, sname, data, time) VALUES (?, ?, ?, ?, ?);", -1, &ost, 0);
                        sqlite3_bind_int(ost, 1, sid); sqlite3_bind_int(ost, 2, rid);
                        sqlite3_bind_text(ost, 3, m_idToName[sid].toUtf8().constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(ost, 4, p[2].constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(ost, 5, curTime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                        sqlite3_step(ost); sqlite3_finalize(ost);
                    }
                }
            }
        });
        connect(s, &QTcpSocket::disconnected, this, [this, s]() { if(m_sockToId.contains(s)) m_idToSock.remove(m_sockToId[s]); m_sockToId.remove(s); s->deleteLater(); });
    }
private:
    sqlite3* m_db;
    QMap<int, QTcpSocket*> m_idToSock;
    QMap<QTcpSocket*, int> m_sockToId;
    QMap<int, QString> m_idToName;
};
#endif