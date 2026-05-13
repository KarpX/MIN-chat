#include <gtest/gtest.h>
#include "../common/DHManager.h"
#include "../common/AES256GCMStrategy.h"
#include "../common/IMessage.h"
#include "../common/MessageFactory.h"
#include "../common/DatabaseManager.h"
#include "../common/FileMessageProxy.h"
#include "../common/CryptoManager.h"

TEST(ScenarioTest, FactoryAndCryptoIntegration) {
    AES256GCMStrategy aes;
    QByteArray key = "12345678901234567890123456789012";
    QByteArray cipherText = aes.encrypt("Secret", key);

    QByteArray rawPacket = "MSG|42|Alice|" + cipherText.toBase64() + "|14:00";

    auto msg = MessageFactory::create(rawPacket);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->type(), MessageType::Text);

    auto txtMsg = static_cast<TextMessage*>(msg.get());

    EXPECT_EQ(txtMsg->senderId, 42);
    EXPECT_EQ(txtMsg->senderName, "Alice");

    QByteArray decryptedText = aes.decrypt(txtMsg->encryptedData, key);
    EXPECT_EQ(decryptedText, QByteArray("Secret"));
}

TEST(ScenarioTest, EndToEndEncryptionCycle) {
    DHManager aliceDH, bobDH;
    AES256GCMStrategy aes;

    QByteArray aliceSecret = aliceDH.generateSharedSecret(bobDH.getPublicKey());
    QByteArray bobSecret = bobDH.generateSharedSecret(aliceDH.getPublicKey());
    ASSERT_EQ(aliceSecret, bobSecret);

    QByteArray plainText = "Super Secret Data";
    QByteArray encrypted = aes.encrypt(plainText, aliceSecret);

    QByteArray decrypted = aes.decrypt(encrypted, bobSecret);
    EXPECT_EQ(decrypted, plainText);
}

TEST(ScenarioTest, CryptoManagerStrategySwitch) {
    CryptoManager manager;
    manager.setStrategy(std::make_unique<AES256GCMStrategy>());

    QByteArray data = "Test Data";
    QByteArray key = "12345678901234567890123456789012";

    QByteArray enc = manager.encryptData(data, key);
    EXPECT_NE(enc, data);
    EXPECT_EQ(manager.decryptData(enc, key), data);
}

TEST(ScenarioTest, FileProxyLazyLoading) {
    FileMessageProxy proxy("non_existent_path.pdf", "doc.pdf", 5000);
    EXPECT_EQ(proxy.getFileName(), "doc.pdf");
    EXPECT_EQ(proxy.getFileSize(), 5000);
}

TEST(ScenarioTest, FilePacketToProxyLifecycle) {
    QByteArray fileData = "fake_encrypted_blob";
    QByteArray rawPacket = "FILE|10|Boss|report.pdf|2048|" + fileData.toBase64() + "|15:30";

    auto msg = MessageFactory::create(rawPacket);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->type(), MessageType::File);

    auto fileMsg = static_cast<FileMessage*>(msg.get());
    EXPECT_EQ(fileMsg->fileContent->getFileName(), "report.pdf");
    EXPECT_EQ(fileMsg->fileContent->getFileSize(), 2048);

    EXPECT_EQ(fileMsg->encryptedData, fileData);
}

TEST(ScenarioTest, DatabaseEncryptionIntegrity) {
    DatabaseManager db(":memory:");
    AES256GCMStrategy aes;
    QByteArray key = "11111111111111111111111111111111";
    QByteArray originalText = "Top Secret Message";

    QByteArray encrypted = aes.encrypt(originalText, key);

    db.saveMsg(1, 2, encrypted, "12:00:00", false, "", 0);

    auto history = db.getMsgs(1, 2);
    ASSERT_EQ(history.size(), 1);
    QByteArray retrievedEncrypted = history[0].encryptedData;

    QByteArray decrypted = aes.decrypt(retrievedEncrypted, key);

    EXPECT_EQ(decrypted, originalText);
}