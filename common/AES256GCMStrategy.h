#ifndef AES256GCMSTRATEGY_H
#define AES256GCMSTRATEGY_H
#include "IEncryptionStrategy.h"
#include "QString"

class AES256GCMStrategy : public IEncryptionStrategy {
public:
    QByteArray encrypt(const QByteArray &data, const QByteArray &key) override {
        return "AES_GCM_ENCRYPTED_" + data;
    }

    QByteArray decrypt(const QByteArray &data, const QByteArray &key) override {
        Q_UNUSED(key);
        if (data.startsWith("AES_GCM_ENCRYPTED_")) {
            return data.mid(18);
        }
        return data;
    }

    QString getAlgorithmName() const override { return "AES-256-GCM"; }
};
#endif // AES256GCMSTRATEGY_H
