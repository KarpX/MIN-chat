#ifndef IMESSAGE_H
#define IMESSAGE_H

#include <QString>
#include <QByteArray>

#include "FileMessageProxy.h"

enum class MessageType {
    Auth,
    SearchResult,
    Status,
    Text,
    File,
    KeyExchange,
    Unknown
};

class IMessage {
public:
    virtual ~IMessage() = default;
    virtual MessageType type() const = 0;
};


struct AuthMessage : public IMessage {
    bool isSuccess;
    int userId;
    QString userName;

    AuthMessage(bool success, int id, QString name)
        : isSuccess(success), userId(id), userName(name) {}

    MessageType type() const override { return MessageType::Auth; }
};

struct SearchResultMessage : public IMessage {
    int userId;
    QString userName;

    SearchResultMessage(int id, QString name)
        : userId(id), userName(name) {}

    MessageType type() const override { return MessageType::SearchResult; }
};

struct StatusMessage : public IMessage {
    int userId;
    QString status;

    StatusMessage(int id, QString st)
        : userId(id), status(st) {}

    MessageType type() const override { return MessageType::Status; }
};

struct TextMessage : public IMessage {
    int senderId;
    QString senderName;
    QByteArray encryptedData;
    QString timestamp;

    TextMessage(int id, QString name, QByteArray data, QString time)
        : senderId(id), senderName(name), encryptedData(data), timestamp(time) {}

    MessageType type() const override { return MessageType::Text; }
};

struct FileMessage : public IMessage {
    int senderId;
    QString senderName;
    QString timestamp;
    QByteArray encryptedData;
    std::unique_ptr<IFileContent> fileContent;

    FileMessage(int id, QString name, QString time, QByteArray encData, std::unique_ptr<IFileContent> content)
        : senderId(id),
        senderName(name),
        timestamp(time),
        encryptedData(encData),
        fileContent(std::move(content)) {}

    MessageType type() const override { return MessageType::File; }
};

struct KeyExchangeMessage : public IMessage {
    int senderId;
    bool isAck; // false = запрос (REQ), true = ответ (ACK)
    QByteArray publicKey;

    KeyExchangeMessage(int id, bool ack, QByteArray key)
        : senderId(id), isAck(ack), publicKey(key) {}

    MessageType type() const override { return MessageType::KeyExchange; }
};
#endif // IMESSAGE_H