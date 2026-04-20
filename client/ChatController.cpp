#include "ChatController.h"
#include <QCryptographicHash>
#include "../common/AES256GCMStrategy.h"

ChatController::ChatController(QObject *p) : QObject(p) {
    m_crypto.setStrategy(std::make_unique<AES256GCMStrategy>());
    m_network.addObserver(this);
    m_network.connectToServer("127.0.0.1", 8080);
}

void ChatController::login(QString u, QString p) {
    m_network.rawSend("LOGIN|" + u.trimmed().toUtf8() + "|" +
                      QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Sha256).toHex() + "\n");
}

void ChatController::registerUser(QString u, QString p) {
    m_network.rawSend("REG|" + u.trimmed().toUtf8() + "|" +
                      QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Sha256).toHex() + "\n");
}

void ChatController::searchUser(QString n) {
    if (!n.trimmed().isEmpty()) m_network.rawSend("SEARCH|" + n.trimmed().toUtf8() + "\n");
}

void ChatController::loadSavedContacts() {
    if (m_myId == -1) return;
    auto contacts = m_db.getContactsWithLastMsg(m_myId);
    for (auto& c : contacts) {
        QString text = "";
        if (!c.lastEncryptedData.isEmpty()) {
            text = QString::fromUtf8(m_crypto.decryptData(c.lastEncryptedData, KEY));
            if (c.lastWasMe) text = "Вы: " + text;
        }
        emit userFound(c.username, c.id, text);
    }
}

void ChatController::selectChat(int id, QString name) {
    m_targetId = id;
    m_db.saveContact(id, name);
    m_sessionMsgs.clear();
    loadHistory();
    QString cachedStatus = m_userStatuses.value(id, "Не в сети");
    emit userStatusChanged(id, cachedStatus);
}

void ChatController::sendTypingStatus() {
    if (m_targetId != -1)
        m_network.rawSend("TYPING|" + QByteArray::number(m_targetId) + "\n");
}

void ChatController::loadHistory() {
    if (m_targetId == -1 || m_myId == -1) return;
    auto rcs = m_db.getMsgs(m_myId, m_targetId);
    for (auto& r : rcs) {
        QString t = QString::fromUtf8(m_crypto.decryptData(r.encryptedData, KEY));
        if (!m_sessionMsgs.contains(r.timestamp + t)) {
            m_sessionMsgs.insert(r.timestamp + t);
            emit newMessageReceived(t, r.senderId == m_myId, r.timestamp);
        }
    }
}

void ChatController::sendMessage(QString text) {
    if (m_targetId == -1) return;
    QString t = QDateTime::currentDateTime().toString("hh:mm");
    QByteArray enc = m_crypto.encryptData(text.toUtf8(), KEY);
    m_db.saveMsg(m_myId, m_targetId, enc, t);
    m_sessionMsgs.insert(t + text);
    emit newMessageReceived(text, true, t);
    m_network.rawSend("MSG|" + QByteArray::number(m_targetId) + "|" + enc.toBase64() + "\n");
}

void ChatController::onMessageReceived(const QByteArray &data) {
    QByteArray cleanData = data.trimmed();
    QList<QByteArray> p = cleanData.split('|');
    if (p.isEmpty()) return;

    if (p[0] == "AUTH_OK") {
        if (p.size() < 3) return;
        m_myId = p[1].toInt();
        emit authSuccess(QString::fromUtf8(p[2]));
    }
    else if (p[0] == "AUTH_ERR") { emit authFailed(); }
    else if (p[0] == "SRCH_RES") {
        if (p.size() < 3) return;
        int fid = p[1].toInt();
        if (fid != m_myId) {
            QString lastText = "";
            auto history = m_db.getMsgs(m_myId, fid);
            if (!history.isEmpty()) {
                auto last = history.last();
                lastText = QString::fromUtf8(m_crypto.decryptData(last.encryptedData, KEY));
                if (last.senderId == m_myId) lastText = "Вы: " + lastText;
            }
            emit userFound(QString::fromUtf8(p[2]), fid, lastText);
        }
    }
    else if (p[0] == "STATUS") {
        if (p.size() < 3) return;
        int sid = p[1].toInt();
        QString status = QString::fromUtf8(p[2]).trimmed();
        if (status != "Печатает...") m_userStatuses[sid] = status;
        emit userStatusChanged(sid, status);
    }
    else if (p[0] == "TYPING") {
        if (p.size() < 2) return;
        emit userStatusChanged(p[1].toInt(), "Печатает...");
    }
    else if (p[0] == "MSG") {
        if (p.size() < 5) return;
        int sid = p[1].toInt();
        QString sn  = QString::fromUtf8(p[2]);
        QByteArray enc = QByteArray::fromBase64(p[3]);
        QString t   = QString::fromUtf8(p[4]);
        QString dec = QString::fromUtf8(m_crypto.decryptData(enc, KEY));

        if (sid != m_myId) {
            if (!m_db.isMessageExists(sid, m_myId, t, enc)) {
                m_db.saveContact(sid, sn);
                m_db.saveMsg(sid, m_myId, enc, t);
            }
            m_userStatuses[sid] = "В сети";
            emit userStatusChanged(sid, "В сети");
            if (sid == m_targetId) {
                if (!m_sessionMsgs.contains(t + dec)) {
                    m_sessionMsgs.insert(t + dec);
                    emit newMessageReceived(dec, false, t);
                }
            } else {
                emit userFound(sn, sid, dec);
            }
        }
    }
}

void ChatController::onStatusChanged(const QString &s) { emit networkStatusChanged(s); }