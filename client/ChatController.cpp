#include "ChatController.h"
#include <QCryptographicHash>
#include "../common/AES256GCMStrategy.h"
#include "../common/MessageFactory.h"
#include "SendMessageCommand.h"
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

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
            // Последнее сообщение могло быть файлом — не расшифровываем бинарные данные
            if (c.lastIsFile) {
                text = "📎 " + c.lastFileName;
            } else {
                text = QString::fromUtf8(m_crypto.decryptData(c.lastEncryptedData, KEY));
            }
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
        bool isMe = (r.senderId == m_myId);

        if (r.isFile) {
            // Файл: не расшифровываем содержимое как текст — только показываем метаданные
            QString dedupeKey = r.timestamp + "|file|" + r.fileName;
            if (!m_sessionMsgs.contains(dedupeKey)) {
                m_sessionMsgs.insert(dedupeKey);
                emit newMessageReceived("Файл: " + r.fileName, isMe, r.timestamp,
                                        true, r.fileName, r.fileSize);
            }
        } else {
            QString t = QString::fromUtf8(m_crypto.decryptData(r.encryptedData, KEY));
            QString dedupeKey = r.timestamp + "|text|" + t;
            if (!m_sessionMsgs.contains(dedupeKey)) {
                m_sessionMsgs.insert(dedupeKey);
                emit newMessageReceived(t, isMe, r.timestamp, false, QString(), 0);
            }
        }
    }
}

void ChatController::sendMessage(QString text) {
    if (m_targetId == -1) return;
    QString t = QDateTime::currentDateTime().toString("hh:mm:ss");
    QByteArray enc = m_crypto.encryptData(text.toUtf8(), KEY);
    // Текстовое сообщение: isFile=false, нет имени/размера файла
    m_db.saveMsg(m_myId, m_targetId, enc, t, false, QString(), 0);
    m_sessionMsgs.insert(t + "|text|" + text);
    emit newMessageReceived(text, true, t, false, QString(), 0);
    auto packet = "MSG|" + QByteArray::number(m_targetId) + "|" + enc.toBase64() + "\n";
    m_network.postCommand(std::make_unique<SendMessageCommand>(&m_network, packet));
}

void ChatController::sendFile(const QString &fileUrlStr) {
    if (m_targetId == -1) return;

    QUrl url(fileUrlStr);
    QString filePath = url.toLocalFile();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "sendFile: не удалось открыть файл:" << filePath;
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QString fileName = QFileInfo(filePath).fileName();
    int fileSize = fileData.size();
    QString t = QDateTime::currentDateTime().toString("hh:mm");

    QByteArray encData = m_crypto.encryptData(fileData, KEY);

    m_db.saveMsg(m_myId, m_targetId, encData, t, true, fileName, fileSize);
     m_sessionMsgs.insert(t + fileName);

    emit newMessageReceived("Файл: " + fileName, true, t, true, fileName, fileSize);

    auto packet = "FILE|" + QByteArray::number(m_targetId) + "|Вы|" +
                  fileName.toUtf8() + "|" +
                  QByteArray::number(fileSize) + "|" +
                  encData.toBase64() + "|" +
                  t.toUtf8() + "\n";

    m_network.postCommand(std::make_unique<SendMessageCommand>(&m_network, packet));
}

void ChatController::downloadFile(int peerId, QString timestamp, QString fileName) {
    if (m_myId == -1) return;

    // getMsgs возвращает все сообщения между двумя пользователями в обоих направлениях
    auto history = m_db.getMsgs(m_myId, peerId);
    QByteArray encData;

    for (const auto& msg : history) {
        // Точное совпадение: это файл + имя + время (timestamp один на всё сообщение)
        if (msg.isFile && msg.fileName == fileName && msg.timestamp == timestamp) {
            encData = msg.encryptedData;
            break;
        }
    }

    if (encData.isEmpty()) {
        qWarning() << "downloadFile: запись не найдена —" << fileName << "@" << timestamp;
        emit fileDownloadError(fileName);
        return;
    }

    QByteArray fileData = m_crypto.decryptData(encData, KEY);

    QString savePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                       + "/" + fileName;

    // Если файл уже есть — добавляем суффикс, не перезаписываем
    if (QFile::exists(savePath)) {
        QFileInfo fi(fileName);
        savePath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                   + "/" + fi.baseName() + "_2." + fi.completeSuffix();
    }

    QFile outFile(savePath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(fileData);
        outFile.close();
        qDebug() << "downloadFile: сохранён в" << savePath;
        emit fileDownloaded(savePath);
    } else {
        qWarning() << "downloadFile: не удалось сохранить" << savePath;
        emit fileDownloadError(fileName);
    }
}

void ChatController::onMessageReceived(const QByteArray &data) {
    auto msg = MessageFactory::create(data);
    if (!msg) return;

    switch (msg->type()) {
    case MessageType::Auth: {
        auto* authMsg = static_cast<AuthMessage*>(msg.get());
        if (authMsg->isSuccess) {
            m_myId   = authMsg->userId;
            m_myName = authMsg->userName;   // сохраняем для подписи в пакетах FILE
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
                if (last.isFile) {
                    lastText = "📎 " + last.fileName;
                } else {
                    lastText = QString::fromUtf8(m_crypto.decryptData(last.encryptedData, KEY));
                }
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
                m_db.saveMsg(txtMsg->senderId, m_myId, txtMsg->encryptedData, txtMsg->timestamp,
                             false, QString(), 0);
            }
            m_userStatuses[txtMsg->senderId] = "В сети";
            emit userStatusChanged(txtMsg->senderId, "В сети");

            if (txtMsg->senderId == m_targetId) {
                QString key = txtMsg->timestamp + "|text|" + dec;
                if (!m_sessionMsgs.contains(key)) {
                    m_sessionMsgs.insert(key);
                    emit newMessageReceived(dec, false, txtMsg->timestamp, false, QString(), 0);
                }
            } else {
                emit userFound(txtMsg->senderName, txtMsg->senderId, dec);
            }
        }
        break;
    }
    case MessageType::File: {
        auto* fileMsg = static_cast<FileMessage*>(msg.get());
        QString fName = fileMsg->fileContent->getFileName();
        int fSize = fileMsg->fileContent->getFileSize();

        if (fileMsg->senderId != m_myId) {
            // 1. Сохраняем в базу данных, если такого сообщения еще нет
            if (!m_db.isMessageExists(fileMsg->senderId, m_myId, fileMsg->timestamp, fileMsg->encryptedData)) {
                m_db.saveContact(fileMsg->senderId, fileMsg->senderName);
                m_db.saveMsg(fileMsg->senderId, m_myId, fileMsg->encryptedData, fileMsg->timestamp, true, fName, fSize);
            }

            m_userStatuses[fileMsg->senderId] = "В сети";
            emit userStatusChanged(fileMsg->senderId, "В сети");

            // 2. РЕАЛЬНОЕ ОТОБРАЖЕНИЕ (проверка на дубликаты в текущей сессии)
            // Ключ должен совпадать с тем, что мы используем в loadHistory
            QString sessionKey = fileMsg->timestamp + fName;

            if (fileMsg->senderId == m_targetId) {
                if (!m_sessionMsgs.contains(sessionKey)) {
                    m_sessionMsgs.insert(sessionKey); // Помечаем, что сообщение уже в памяти
                    emit newMessageReceived("Файл: " + fName, false, fileMsg->timestamp, true, fName, fSize);
                }
            } else {
                // Если чат не открыт, просто обновляем превью в списке контактов слева
                emit userFound(fileMsg->senderName, fileMsg->senderId, "📎 " + fName);
            }
        }
        break;
    }
    default:
        break;
    }
}

void ChatController::onStatusChanged(const QString &s) { emit networkStatusChanged(s); }