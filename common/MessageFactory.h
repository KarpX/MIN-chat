#ifndef MESSAGEFACTORY_H
#define MESSAGEFACTORY_H

#include "IMessage.h"
#include <QByteArray>
#include <QList>
#include <memory>

class MessageFactory {
public:
    static std::unique_ptr<IMessage> create(const QByteArray& rawData);
};

#endif // MESSAGEFACTORY_H