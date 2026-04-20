#ifndef AES256GCMSTRATEGY_H
#define AES256GCMSTRATEGY_H

#include "IEncryptionStrategy.h"
#include <QString>
#include <QByteArray>

class AES256GCMStrategy : public IEncryptionStrategy {
public:
    AES256GCMStrategy();
    QByteArray encrypt(const QByteArray &data, const QByteArray &key) override;
    QByteArray decrypt(const QByteArray &data, const QByteArray &key) override;
    QString getAlgorithmName() const override;

private:
    const int KEY_SIZE = 32;
    const int IV_SIZE = 12;
    const int TAG_SIZE = 16;
};

#endif // AES256GCMSTRATEGY_H