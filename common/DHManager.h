#ifndef DHMANAGER_H
#define DHMANAGER_H

#include <QByteArray>
#include <openssl/evp.h>

class DHManager {
public:
    DHManager();
    ~DHManager();

    QByteArray getPublicKey() const;

    QByteArray generateSharedSecret(const QByteArray& peerPubKeyDER);

private:
    EVP_PKEY* m_pkey = nullptr;
};

#endif // DHMANAGER_H