#ifndef CONTACTMODEL_H
#define CONTACTMODEL_H
#include <QString>

struct ContactRecord {
    int id;
    QString username;
    QByteArray lastEncryptedData;
    bool lastWasMe;
};

#endif // CONTACTMODEL_H
