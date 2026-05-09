#include "ChatServer.h"
#include <QDebug>
#include <QDateTime>

ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {
    if (sqlite3_open("server_data.db", &m_db) != SQLITE_OK) {
        qDebug() << "Ошибка открытия серверной БД:" << sqlite3_errmsg(m_db);
    }
    sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, password TEXT);", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "CREATE TABLE IF NOT EXISTS off_msgs (sid INTEGER, rid INTEGER, sname TEXT, data TEXT, time TEXT);", nullptr, nullptr, nullptr);
}

ChatServer::~ChatServer() {
    sqlite3_close(m_db);
}

void ChatServer::startServer(int port) {
    if (this->listen(QHostAddress::Any, port)) {
        qDebug() << "Сервер запущен на порту:" << port;
    } else {
        qDebug() << "Не удалось запустить сервер!";
    }
}

void ChatServer::incomingConnection(qintptr sd) {
    QTcpSocket* s = new QTcpSocket(this);
    s->setSocketDescriptor(sd);

    connect(s, &QTcpSocket::readyRead, this, [this, s]() {
        while (s->canReadLine()) {
            QByteArray line = s->readLine().trimmed();
            if (line.isEmpty()) continue;

            QList<QByteArray> p = line.split('|');
            if (p.isEmpty()) continue;
            QByteArray cmd = p[0];

            if (cmd == "REG" || cmd == "LOGIN") {
                if (p.size() < 3) continue;
                sqlite3_stmt* st;
                if (cmd == "REG") {
                    sqlite3_prepare_v2(m_db, "INSERT INTO users (username, password) VALUES (?, ?);", -1, &st, nullptr);
                } else {
                    sqlite3_prepare_v2(m_db, "SELECT id FROM users WHERE username=? AND password=?;", -1, &st, nullptr);
                }

                sqlite3_bind_text(st, 1, p[1].constData(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(st, 2, p[2].constData(), -1, SQLITE_TRANSIENT);

                if (sqlite3_step(st) == (cmd == "REG" ? SQLITE_DONE : SQLITE_ROW)) {
                    int id = (cmd == "REG") ? (int)sqlite3_last_insert_rowid(m_db) : sqlite3_column_int(st, 0);

                    m_idToSock[id] = s;
                    m_sockToId[s] = id;
                    m_idToName[id] = QString::fromUtf8(p[1]);

                    s->write("AUTH_OK|" + QByteArray::number(id) + "|" + p[1] + "\n");
                    s->flush();

                    QByteArray statusPkt = "STATUS|" + QByteArray::number(id) + "|В сети\n";
                    for(auto* client : m_idToSock.values()) {
                        if(client != s) client->write(statusPkt);
                    }

                    for(int onlineId : m_idToSock.keys()) {
                        if(onlineId != id) s->write("STATUS|" + QByteArray::number(onlineId) + "|В сети\n");
                    }

                    sqlite3_stmt* ost;
                    sqlite3_prepare_v2(m_db, "SELECT sid, sname, data, time FROM off_msgs WHERE rid=?;", -1, &ost, nullptr);
                    sqlite3_bind_int(ost, 1, id);
                    while (sqlite3_step(ost) == SQLITE_ROW) {
                        QByteArray offMsg = "MSG|" + QByteArray::number(sqlite3_column_int(ost, 0)) + "|" +
                                            (const char*)sqlite3_column_text(ost, 1) + "|" +
                                            (const char*)sqlite3_column_text(ost, 2) + "|" +
                                            (const char*)sqlite3_column_text(ost, 3) + "\n";
                        s->write(offMsg);
                    }
                    sqlite3_finalize(ost);

                    QString delQuery = QString("DELETE FROM off_msgs WHERE rid=%1").arg(id);
                    sqlite3_exec(m_db, delQuery.toUtf8().constData(), nullptr, nullptr, nullptr);

                } else {
                    s->write("AUTH_ERR|Invalid credentials or user exists\n");
                }
                sqlite3_finalize(st);
            }
            else if (cmd == "SEARCH") {
                if (p.size() < 2) continue;
                sqlite3_stmt* st;
                sqlite3_prepare_v2(m_db, "SELECT id, username FROM users WHERE username LIKE ? LIMIT 8;", -1, &st, nullptr);
                QString pat = QString::fromUtf8(p[1]) + "%";
                sqlite3_bind_text(st, 1, pat.toUtf8().constData(), -1, SQLITE_TRANSIENT);

                while (sqlite3_step(st) == SQLITE_ROW) {
                    QByteArray res = "SRCH_RES|" + QByteArray::number(sqlite3_column_int(st, 0)) + "|" +
                                     (const char*)sqlite3_column_text(st, 1) + "\n";
                    s->write(res);
                }
                sqlite3_finalize(st);
            }
            else if (cmd == "MSG") {
                if (p.size() < 3) continue;
                int rid = p[1].toInt();
                int sid = m_sockToId.value(s, -1);
                if (sid == -1) continue;

                QString curTime = QDateTime::currentDateTime().toString("hh:mm");
                QByteArray msg = "MSG|" + QByteArray::number(sid) + "|" + m_idToName[sid].toUtf8() + "|" +
                                 p[2] + "|" + curTime.toUtf8() + "\n";

                if (m_idToSock.contains(rid)) {
                    m_idToSock[rid]->write(msg);
                    m_idToSock[rid]->flush();
                } else {
                    sqlite3_stmt* ost;
                    sqlite3_prepare_v2(m_db, "INSERT INTO off_msgs (sid, rid, sname, data, time) VALUES (?, ?, ?, ?, ?);", -1, &ost, nullptr);
                    sqlite3_bind_int(ost, 1, sid);
                    sqlite3_bind_int(ost, 2, rid);
                    sqlite3_bind_text(ost, 3, m_idToName[sid].toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(ost, 4, p[2].constData(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(ost, 5, curTime.toUtf8().constData(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(ost);
                    sqlite3_finalize(ost);
                }
            }
        }
    });

    connect(s, &QTcpSocket::disconnected, this, [this, s]() {
        if (m_sockToId.contains(s)) {
            int id = m_sockToId.value(s);
            m_idToSock.remove(id);
            m_sockToId.remove(s);
            QByteArray statusPkt = "STATUS|" + QByteArray::number(id) + "|Не в сети\n";
            for(auto* client : m_idToSock.values()) client->write(statusPkt);
        }
        s->deleteLater();
        qDebug() << "Клиент отключился.";
    });
}