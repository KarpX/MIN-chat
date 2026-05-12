#include "DHManager.h"
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

DHManager::DHManager() {
    // 1. Создаем контекст для эллиптической кривой prime256v1 (стандарт безопасности)
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    EVP_PKEY_keygen_init(pctx);
    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1);

    // 2. Генерируем пару ключей (Приватный останется в памяти, Публичный отдадим)
    EVP_PKEY_keygen(pctx, &m_pkey);
    EVP_PKEY_CTX_free(pctx);
}

DHManager::~DHManager() {
    if (m_pkey) EVP_PKEY_free(m_pkey);
}

QByteArray DHManager::getPublicKey() const {
    unsigned char *der = nullptr;
    int len = i2d_PUBKEY(m_pkey, &der); // Конвертируем ключ в бинарный формат DER
    QByteArray pubKey((char*)der, len);
    OPENSSL_free(der);
    return pubKey;
}

QByteArray DHManager::generateSharedSecret(const QByteArray& peerPubKeyDER) {
    // 1. Читаем публичный ключ собеседника
    const unsigned char* p = (const unsigned char*)peerPubKeyDER.data();
    EVP_PKEY* peerKey = d2i_PUBKEY(NULL, &p, peerPubKeyDER.size());
    if (!peerKey) return QByteArray();

    // 2. Вычисляем общий секрет
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(m_pkey, NULL);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_derive_set_peer(ctx, peerKey);

    size_t secretLen;
    EVP_PKEY_derive(ctx, NULL, &secretLen);
    QByteArray sharedSecret(secretLen, 0);
    EVP_PKEY_derive(ctx, (unsigned char*)sharedSecret.data(), &secretLen);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peerKey);

    // 3. Хешируем результат через SHA-256, чтобы получить ровно 32 байта (требование AES-256)
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)sharedSecret.data(), sharedSecret.size(), hash);

    return QByteArray((char*)hash, SHA256_DIGEST_LENGTH);
}