#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H
#include <QObject>
#include <QCryptographicHash>
#include <QDateTime>
#include <QSet>
#include "NetworkClient.h"
#include "../common/CryptoManager.h"
#include "../common/AES256GCMStrategy.h"
#include "../common/DatabaseManager.h"

class ChatController : public QObject, public INetworkObserver {
    Q_OBJECT
    int m_myId = -1; int m_targetId = -1;
    QSet<QString> m_sessionMsgs; // Кэш сообщений для текущей сессии
public:
    const QByteArray KEY = "12345678901234567890123456789012";
    explicit ChatController(QObject *p = nullptr) : QObject(p) {
        m_crypto.setStrategy(std::make_unique<AES256GCMStrategy>());
        m_network.addObserver(this); m_network.connectToServer("127.0.0.1", 8080);
    }
    Q_INVOKABLE void login(QString u, QString p) { m_network.rawSend("LOGIN|" + u.trimmed().toUtf8() + "|" + QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Sha256).toHex() + "\n"); }
    Q_INVOKABLE void registerUser(QString u, QString p) { m_network.rawSend("REG|" + u.trimmed().toUtf8() + "|" + QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Sha256).toHex() + "\n"); }
    Q_INVOKABLE void searchUser(QString n) { if(!n.trimmed().isEmpty()) m_network.rawSend("SEARCH|" + n.trimmed().toUtf8() + "\n"); }
    Q_INVOKABLE void loadSavedContacts() { if(m_myId == -1) return; auto list = m_db.getContacts(m_myId); for(auto& c : list) emit userFound(c.username, c.id); }

    Q_INVOKABLE void selectChat(int id, QString name) {
        m_targetId = id;
        m_db.saveContact(id, name);
        m_sessionMsgs.clear(); // Очистка при смене чата
        loadHistory();
    }

    void loadHistory() {
        if(m_targetId == -1 || m_myId == -1) return;
        auto recs = m_db.getMsgs(m_myId, m_targetId);
        for (auto& r : recs) {
            QString txt = QString::fromUtf8(m_crypto.decryptData(r.encryptedData, KEY));
            QString msgKey = r.timestamp + txt; // Уникальный ключ сообщения
            if (!m_sessionMsgs.contains(msgKey)) {
                m_sessionMsgs.insert(msgKey);
                emit newMessageReceived(txt, r.senderId == m_myId, r.timestamp);
            }
        }
    }

    Q_INVOKABLE void sendMessage(QString text) {
        if(m_targetId == -1 || m_myId == -1) return;
        QString t = QDateTime::currentDateTime().toString("hh:mm"); // ФОРМАТ HH:MM
        QByteArray enc = m_crypto.encryptData(text.toUtf8(), KEY);
        m_db.saveMsg(m_myId, m_targetId, enc, t);
        m_sessionMsgs.insert(t + text);
        emit newMessageReceived(text, true, t);
        m_network.rawSend("MSG|" + QByteArray::number(m_targetId) + "|" + enc.toBase64() + "\n");
    }

    void onMessageReceived(const QByteArray &data) override {
        QList<QByteArray> p = data.split('|');
        if (p.isEmpty()) return;
        QByteArray cmd = p[0];
        if (cmd == "AUTH_OK") { m_myId = p[1].toInt(); emit authSuccess(QString::fromUtf8(p[2])); }
        else if (cmd == "SRCH_RES") { if (p[1].toInt() != m_myId) emit userFound(QString::fromUtf8(p[2]), p[1].toInt()); }
        else if (cmd == "MSG") {
            int sid = p[1].toInt();
            QString sname = QString::fromUtf8(p[2]);
            QByteArray enc = QByteArray::fromBase64(p[3]);
            QString t = QString::fromUtf8(p[4]); // Получаем время от сервера (HH:MM)
            QString decTxt = QString::fromUtf8(m_crypto.decryptData(enc, KEY));

            if (sid != m_myId) {
                // Если такого сообщения еще нет в БД - сохраняем
                if (!m_db.isMessageExists(sid, m_myId, t, enc)) {
                    m_db.saveContact(sid, sname);
                    m_db.saveMsg(sid, m_myId, enc, t);
                }
                // Если чат открыт и это не дубль на экране - выводим
                if (sid == m_targetId && !m_sessionMsgs.contains(t + decTxt)) {
                    m_sessionMsgs.insert(t + decTxt);
                    emit newMessageReceived(decTxt, false, t);
                } else if (sid != m_targetId) { emit userFound(sname, sid); }
            }
        }
    }
    void onStatusChanged(const QString &s) override { emit networkStatusChanged(s); }

signals:
    void newMessageReceived(QString t, bool m, QString tm);
    void networkStatusChanged(QString s);
    void authSuccess(QString n);
    void userFound(QString n, int id);
private:
    CryptoManager m_crypto; NetworkClient m_network; DatabaseManager m_db;
};
#endif