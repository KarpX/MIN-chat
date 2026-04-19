#ifndef AES256GCMSTRATEGY_H
#define AES256GCMSTRATEGY_H
#include "IEncryptionStrategy.h"
#include "QString"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

class AES256GCMStrategy : public IEncryptionStrategy {
private:
    const int KEY_SIZE = 32;
    const int IV_SIZE = 12;
    const int TAG_SIZE = 16;
public:
    QByteArray encrypt(const QByteArray &data, const QByteArray &key) override {
        QByteArray iv(IV_SIZE, 0);
        RAND_bytes((unsigned char*)iv.data(), IV_SIZE);

        QByteArray ciphertext(data.size(), 0);
        QByteArray tag(TAG_SIZE, 0);

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        int len;
        int ciphertext_len;

        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, (unsigned char*)key.data(), (unsigned char*)iv.data());

        EVP_EncryptUpdate(ctx, (unsigned char*)ciphertext.data(), &len, (unsigned char*)data.data(), data.size());
        ciphertext_len = len;

        EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext.data() + len, &len);
        ciphertext_len += len;

        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data());
        EVP_CIPHER_CTX_free(ctx);

        return iv + tag + ciphertext;
    }

    QByteArray decrypt(const QByteArray &data, const QByteArray &key) override {
        if (data.size() < (IV_SIZE + TAG_SIZE)) return data;

        QByteArray iv = data.left(IV_SIZE);
        QByteArray tag = data.mid(IV_SIZE, TAG_SIZE);
        QByteArray ciphertext = data.mid(IV_SIZE + TAG_SIZE);

        QByteArray decrypted(ciphertext.size(), 0);

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        int len;
        int plaintext_len;

        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, (unsigned char*)key.data(), (unsigned char*)iv.data());
        EVP_DecryptUpdate(ctx, (unsigned char*)decrypted.data(), &len, (unsigned char*)ciphertext.data(), ciphertext.size());
        plaintext_len = len;

        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, (void*)tag.data());

        int ret = EVP_DecryptFinal_ex(ctx, (unsigned char*)decrypted.data() + len, &len);
        EVP_CIPHER_CTX_free(ctx);

        if (ret > 0) {
            plaintext_len += len;
            return decrypted.left(plaintext_len);
        } else {
            return "--- ОШИБКА ДЕШИФРОВАНИЯ (ДАННЫЕ ПОВРЕЖДЕНЫ) ---";
        }
    }

    QString getAlgorithmName() const override { return "AES-256-GCM"; }
};
#endif // AES256GCMSTRATEGY_H
