#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H
#include <QString>
#include <QByteArray>

struct MessageData {
    int sender;
    QString text;
    QString timestamp;
};

struct MessageRecord {
    int senderId;
    int receiverId;
    QByteArray encryptedData;
    QString timestamp;

    bool isFile   = false;
    QString fileName;
    int fileSize = 0;
};

#endif // MESSAGEMODEL_H
