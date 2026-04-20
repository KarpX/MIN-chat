#include "AES256GCMStrategy.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

AES256GCMStrategy::AES256GCMStrategy() {}

QByteArray AES256GCMStrategy::encrypt(const QByteArray &data, const QByteArray &key) {
    QByteArray iv(IV_SIZE, 0);
    RAND_bytes((unsigned char*)iv.data(), IV_SIZE);
    QByteArray ciphertext(data.size(), 0);
    QByteArray tag(TAG_SIZE, 0);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len = 0, finalLen = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, (unsigned char*)key.data(), (unsigned char*)iv.data());
    EVP_EncryptUpdate(ctx, (unsigned char*)ciphertext.data(), &len, (unsigned char*)data.data(), data.size());
    EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext.data() + len, &finalLen);
    ciphertext.resize(len + finalLen);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data());
    EVP_CIPHER_CTX_free(ctx);
    return iv + tag + ciphertext;
}

QByteArray AES256GCMStrategy::decrypt(const QByteArray &data, const QByteArray &key) {
    if (data.size() < (IV_SIZE + TAG_SIZE)) return data;
    QByteArray iv = data.left(IV_SIZE);
    QByteArray tag = data.mid(IV_SIZE, TAG_SIZE);
    QByteArray ciphertext = data.mid(IV_SIZE + TAG_SIZE);
    QByteArray decrypted(ciphertext.size(), 0);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, (unsigned char*)key.data(), (unsigned char*)iv.data());
    EVP_DecryptUpdate(ctx, (unsigned char*)decrypted.data(), &len, (unsigned char*)ciphertext.data(), ciphertext.size());
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, (void*)tag.data());
    int ret = EVP_DecryptFinal_ex(ctx, (unsigned char*)decrypted.data() + len, &len);
    EVP_CIPHER_CTX_free(ctx);
    return (ret > 0) ? decrypted : QByteArray("--- DECRYPT ERROR ---");
}

QString AES256GCMStrategy::getAlgorithmName() const { return "AES-256-GCM"; }