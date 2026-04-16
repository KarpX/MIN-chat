#ifndef CRYPTOMANAGER_H
#define CRYPTOMANAGER_H

#include <memory>
#include "IEncryptionStrategy.h"

class CryptoManager {
public:
    void setStrategy(std::unique_ptr<IEncryptionStrategy> strategy) {
        m_strategy = std::move(strategy);
    }

    QByteArray encryptData(const QByteArray &data, const QByteArray &key) {
        if (!m_strategy) return data;
        return m_strategy->encrypt(data, key);
    }

    QByteArray decryptData(const QByteArray &data, const QByteArray &key) {
        if (!m_strategy) return data;
        return m_strategy->decrypt(data, key);
    }

private:
    std::unique_ptr<IEncryptionStrategy> m_strategy;
};

#endif // CRYPTOMANAGER_H
