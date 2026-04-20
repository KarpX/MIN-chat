#ifndef CRYPTOMANAGER_H
#define CRYPTOMANAGER_H
#include <memory>
#include <QByteArray>
#include "IEncryptionStrategy.h"

class CryptoManager {
public:
    void setStrategy(std::unique_ptr<IEncryptionStrategy> strategy);
    QByteArray encryptData(const QByteArray &data, const QByteArray &key);
    QByteArray decryptData(const QByteArray &data, const QByteArray &key);
private:
    std::unique_ptr<IEncryptionStrategy> m_strategy;
};

#endif