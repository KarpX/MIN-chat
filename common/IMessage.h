#ifndef IMESSAGE_H
#define IMESSAGE_H

#include <QString>
#include <QByteArray>

enum class MessageType {
    Auth,
    SearchResult,
    Status,
    Text,
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

#endif // IMESSAGE_H