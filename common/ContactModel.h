#ifndef CONTACTMODEL_H
#define CONTACTMODEL_H
#include <QString>

struct ContactRecord {
    int id;
    QString username;
    QByteArray lastEncryptedData;
    bool lastWasMe  = false;

    bool lastIsFile   = false;
    QString lastFileName;
};

#endif // CONTACTMODEL_H
