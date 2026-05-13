#include <gtest/gtest.h>
#include "../common/MessageFactory.h"
#include "../common/IMessage.h"

TEST(MessageFactoryTest, ParseAuthOk) {
    auto msg = MessageFactory::create("AUTH_OK|42|Ivan");
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->type(), MessageType::Auth);
    auto authMsg = static_cast<AuthMessage*>(msg.get());
    EXPECT_TRUE(authMsg->isSuccess);
    EXPECT_EQ(authMsg->userId, 42);
    EXPECT_EQ(authMsg->userName, "Ivan");
}

TEST(MessageFactoryTest, ParseTextMsg) {
    auto msg = MessageFactory::create("MSG|10|Alice|SGVsbG8=|12:00");
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->type(), MessageType::Text);
    auto txtMsg = static_cast<TextMessage*>(msg.get());
    EXPECT_EQ(txtMsg->senderId, 10);
    EXPECT_EQ(txtMsg->encryptedData, QByteArray::fromBase64("SGVsbG8="));
}

TEST(MessageFactoryTest, ParseFileMsg) {
    auto msg = MessageFactory::create("FILE|5|Bob|doc.pdf|1024|SGVsbG8=|13:00");
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->type(), MessageType::File);
}

TEST(MessageFactoryTest, ParseDHReq) {
    auto msg = MessageFactory::create("DH_REQ|2|UHVibGljS2V5");
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->type(), MessageType::KeyExchange);
    EXPECT_FALSE(static_cast<KeyExchangeMessage*>(msg.get())->isAck);
}

TEST(MessageFactoryTest, ParseEmptyStringReturnsNull) {
    auto msg = MessageFactory::create("");
    EXPECT_EQ(msg, nullptr);
}

TEST(MessageFactoryTest, ParseInvalidCommandReturnsNull) {
    auto msg = MessageFactory::create("UNKNOWN_CMD|123");
    EXPECT_EQ(msg, nullptr);
}

TEST(MessageFactoryTest, ParseMissingArgumentsReturnsNull) {
    auto msg = MessageFactory::create("MSG|10");
    EXPECT_EQ(msg, nullptr);
}