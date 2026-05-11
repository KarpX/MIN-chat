#include "ChatController.h"
#include <QCryptographicHash>
#include "../common/AES256GCMStrategy.h"
#include "../common/MessageFactory.h"

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
    auto msg = MessageFactory::create(data);
    if (!msg) return;

    switch (msg->type()) {
    case MessageType::Auth: {
        auto* authMsg = static_cast<AuthMessage*>(msg.get());
        if (authMsg->isSuccess) {
            m_myId = authMsg->userId;
            emit authSuccess(authMsg->userName);
        } else {
            emit authFailed();
        }
        break;
    }
    case MessageType::SearchResult: {
        auto* srchMsg = static_cast<SearchResultMessage*>(msg.get());
        if (srchMsg->userId != m_myId) {
            QString lastText = "";
            auto history = m_db.getMsgs(m_myId, srchMsg->userId);
            if (!history.isEmpty()) {
                auto last = history.last();
                lastText = QString::fromUtf8(m_crypto.decryptData(last.encryptedData, KEY));
                if (last.senderId == m_myId) lastText = "Вы: " + lastText;
            }
            emit userFound(srchMsg->userName, srchMsg->userId, lastText);
        }
        break;
    }
    case MessageType::Status: {
        auto* statMsg = static_cast<StatusMessage*>(msg.get());
        if (statMsg->status != "Печатает...") m_userStatuses[statMsg->userId] = statMsg->status;
        emit userStatusChanged(statMsg->userId, statMsg->status);
        break;
    }
    case MessageType::Text: {
        auto* txtMsg = static_cast<TextMessage*>(msg.get());
        QString dec = QString::fromUtf8(m_crypto.decryptData(txtMsg->encryptedData, KEY));

        if (txtMsg->senderId != m_myId) {
            if (!m_db.isMessageExists(txtMsg->senderId, m_myId, txtMsg->timestamp, txtMsg->encryptedData)) {
                m_db.saveContact(txtMsg->senderId, txtMsg->senderName);
                m_db.saveMsg(txtMsg->senderId, m_myId, txtMsg->encryptedData, txtMsg->timestamp);
            }
            m_userStatuses[txtMsg->senderId] = "В сети";
            emit userStatusChanged(txtMsg->senderId, "В сети");

            if (txtMsg->senderId == m_targetId) {
                if (!m_sessionMsgs.contains(txtMsg->timestamp + dec)) {
                    m_sessionMsgs.insert(txtMsg->timestamp + dec);
                    emit newMessageReceived(dec, false, txtMsg->timestamp);
                }
            } else {
                emit userFound(txtMsg->senderName, txtMsg->senderId, dec);
            }
        }
        break;
    }
    default:
        break;
    }
}

void ChatController::onStatusChanged(const QString &s) { emit networkStatusChanged(s); }