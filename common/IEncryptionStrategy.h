#ifndef IENCRYPTIONSTRATEGY_H
#define IENCRYPTIONSTRATEGY_H
#include <QByteArray>
#include <QString>

class IEncryptionStrategy {
public:
    virtual ~IEncryptionStrategy() = default;
    virtual QByteArray encrypt(const QByteArray &data, const QByteArray &key) = 0;
    virtual QByteArray decrypt(const QByteArray &data, const QByteArray &key) = 0;
    virtual QString getAlgorithmName() const = 0;
};

#endif