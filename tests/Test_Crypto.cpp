#include <gtest/gtest.h>
#include "../common/AES256GCMStrategy.h"
#include "../common/DHManager.h"

class CryptoTest : public ::testing::Test {
protected:
    AES256GCMStrategy aes;
    QByteArray testKey = "12345678901234567890123456789012";
};

TEST_F(CryptoTest, EncryptNormalText) { EXPECT_NE("Data", aes.encrypt("Data", testKey)); }
TEST_F(CryptoTest, EncryptEmpty) { EXPECT_NO_THROW(aes.encrypt("", testKey)); }
TEST_F(CryptoTest, EncryptLargeData) { EXPECT_NO_THROW(aes.encrypt(QByteArray(1000000, 'A'), testKey)); }
TEST_F(CryptoTest, EncryptIVUniqueness) { EXPECT_NE(aes.encrypt("A", testKey), aes.encrypt("A", testKey)); }
TEST_F(CryptoTest, EncryptBinary) { QByteArray bin("\x00\xFF\xDE\xAD\xBE\xEF", 5); EXPECT_NO_THROW(aes.encrypt(bin, testKey)); }
TEST_F(CryptoTest, EncryptResultSize) { EXPECT_GT(aes.encrypt("A", testKey).size(), 28); }

TEST_F(CryptoTest, DecryptSuccess) { QByteArray e = aes.encrypt("Hi", testKey); EXPECT_EQ("Hi", aes.decrypt(e, testKey)); }
TEST_F(CryptoTest, DecryptWrongKey) { QByteArray e = aes.encrypt("Hi", testKey); EXPECT_EQ("--- DECRYPT ERROR ---", aes.decrypt(e, "00000000000000000000000000000000")); }
TEST_F(CryptoTest, DecryptCorruptedData) { QByteArray e = aes.encrypt("Hi", testKey); e[20] = !e[20]; EXPECT_EQ("--- DECRYPT ERROR ---", aes.decrypt(e, testKey)); }
TEST_F(CryptoTest, DecryptShortData) { EXPECT_EQ("123", aes.decrypt("123", testKey)); }
TEST_F(CryptoTest, DecryptEmpty) { EXPECT_EQ("", aes.decrypt("", testKey)); }
TEST_F(CryptoTest, DecryptTamperedTag) { QByteArray e = aes.encrypt("Hi", testKey); e[13] = 'X'; EXPECT_EQ("--- DECRYPT ERROR ---", aes.decrypt(e, testKey)); }
TEST_F(CryptoTest, DecryptHugeData) { QByteArray data(5000, 'z'); EXPECT_EQ(data, aes.decrypt(aes.encrypt(data, testKey), testKey)); }

TEST(DHManagerTest, SecretMatch) {
    DHManager a, b;
    EXPECT_EQ(a.generateSharedSecret(b.getPublicKey()), b.generateSharedSecret(a.getPublicKey()));
}
TEST(DHManagerTest, SecretSize) { DHManager a, b; EXPECT_EQ(a.generateSharedSecret(b.getPublicKey()).size(), 32); }
TEST(DHManagerTest, InvalidKey) { DHManager a; EXPECT_TRUE(a.generateSharedSecret("trash").isEmpty()); }
TEST(DHManagerTest, NullKey) { DHManager a; EXPECT_TRUE(a.generateSharedSecret(QByteArray()).isEmpty()); }
TEST(DHManagerTest, Repeatability) { DHManager a, b; QByteArray p = b.getPublicKey(); EXPECT_EQ(a.generateSharedSecret(p), a.generateSharedSecret(p)); }
TEST(DHManagerTest, CrossPlatformSafety) { DHManager a; QByteArray pk = a.getPublicKey(); EXPECT_FALSE(pk.contains("PRIVATE KEY")); }

TEST_F(CryptoTest, EncryptDecryptValidText) {
    QByteArray original = "Hello World!";
    QByteArray encrypted = aes.encrypt(original, testKey);
    ASSERT_NE(original, encrypted);
    QByteArray decrypted = aes.decrypt(encrypted, testKey);
    EXPECT_EQ(original, decrypted);
}

TEST_F(CryptoTest, EncryptEmptyString) {
    QByteArray original = "";
    QByteArray encrypted = aes.encrypt(original, testKey);
    EXPECT_EQ(aes.decrypt(encrypted, testKey), original);
}

TEST_F(CryptoTest, DecryptWithWrongKeyFails) {
    QByteArray original = "Secret";
    QByteArray encrypted = aes.encrypt(original, testKey);
    QByteArray wrongKey = "00000000000000000000000000000000";
    EXPECT_EQ(aes.decrypt(encrypted, wrongKey), QByteArray("--- DECRYPT ERROR ---"));
}

TEST_F(CryptoTest, CorruptedCiphertextFails) {
    QByteArray encrypted = aes.encrypt("Secret", testKey);
    encrypted[encrypted.size() - 1] = encrypted[encrypted.size() - 1] ^ 0xFF;
    EXPECT_EQ(aes.decrypt(encrypted, testKey), QByteArray("--- DECRYPT ERROR ---"));
}

TEST_F(CryptoTest, TamperedTagFails) {
    QByteArray encrypted = aes.encrypt("Secret", testKey);
    encrypted[15] = 'X';
    EXPECT_EQ(aes.decrypt(encrypted, testKey), QByteArray("--- DECRYPT ERROR ---"));
}

TEST_F(CryptoTest, OutputIsLargerThanInput) {
    QByteArray original = "A";
    QByteArray encrypted = aes.encrypt(original, testKey);
    EXPECT_GT(encrypted.size(), original.size());
}

TEST_F(CryptoTest, DecryptTooShortData) {
    QByteArray shortData = "12345";
    EXPECT_EQ(aes.decrypt(shortData, testKey), shortData);
}

TEST(DHManagerTest, GenerateKeysSuccess) {
    DHManager dh;
    EXPECT_FALSE(dh.getPublicKey().isEmpty());
}

TEST(DHManagerTest, DifferentManagersHaveDifferentKeys) {
    DHManager dh1; DHManager dh2;
    EXPECT_NE(dh1.getPublicKey(), dh2.getPublicKey());
}

TEST(DHManagerTest, SharedSecretMatch) {
    DHManager alice; DHManager bob;
    QByteArray aliceSecret = alice.generateSharedSecret(bob.getPublicKey());
    QByteArray bobSecret = bob.generateSharedSecret(alice.getPublicKey());
    EXPECT_EQ(aliceSecret, bobSecret);
    EXPECT_EQ(aliceSecret.size(), 32);
}

TEST(DHManagerTest, InvalidPeerKeyReturnsEmpty) {
    DHManager dh;
    QByteArray invalidKey = "invalid_der_data";
    EXPECT_TRUE(dh.generateSharedSecret(invalidKey).isEmpty());
}

TEST(DHManagerTest, SameKeyProducesSameSecret) {
    DHManager alice; DHManager bob;
    QByteArray secret1 = alice.generateSharedSecret(bob.getPublicKey());
    QByteArray secret2 = alice.generateSharedSecret(bob.getPublicKey());
    EXPECT_EQ(secret1, secret2);
}