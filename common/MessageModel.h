#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H
#include <QString>

struct MessageData {
    int sender;
    int text;
    QString timestamp;
};

#endif // MESSAGEMODEL_H
