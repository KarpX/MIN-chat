#include <gtest/gtest.h>
#include "../common/DatabaseManager.h"
#include <QFile>

class DatabaseTest : public ::testing::Test {
protected:
    DatabaseManager* db;
    void SetUp() override {
        db = new DatabaseManager(":memory:");
    }
    void TearDown() override { delete db; }
};

TEST_F(DatabaseTest, SaveNormalTextMessage) {
    bool result = true;
    try {
        db->saveMsg(1, 2, "Hello", "12:00:00", false, "", 0);
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);
}

TEST_F(DatabaseTest, SaveFileMessage) {
    bool result = true;
    try {
        db->saveMsg(1, 2, "file_bytes", "12:05:00", true, "test.pdf", 1024);
    } catch (...) {
        result = false;
    }
    EXPECT_TRUE(result);
}

TEST_F(DatabaseTest, SaveMessageWithEmptyData) {
    db->saveMsg(1, 2, "", "12:10:00", false, "", 0);
    auto msgs = db->getMsgs(1, 2);
    EXPECT_EQ(msgs.size(), 1);
    EXPECT_TRUE(msgs[0].encryptedData.isEmpty());
}

TEST_F(DatabaseTest, SaveMessageWithUnicodeFileName) {
    db->saveMsg(1, 2, "data", "12:15:00", true, "отчёт_2024.docx", 500);
    auto msgs = db->getMsgs(1, 2);
    EXPECT_EQ(msgs[0].fileName, "отчёт_2024.docx");
}

TEST_F(DatabaseTest, SaveMessageWithHugeIds) {
    int sender = 999999;
    int receiver = 888888;
    db->saveMsg(sender, receiver, "data", "00:00:00", false, "", 0);
    auto msgs = db->getMsgs(sender, receiver);
    EXPECT_EQ(msgs.size(), 1);
}

TEST_F(DatabaseTest, SaveMultipleMessagesInRow) {
    db->saveMsg(1, 2, "Msg1", "10:00:01", false, "", 0);
    db->saveMsg(1, 2, "Msg2", "10:00:02", false, "", 0);
    db->saveMsg(1, 2, "Msg3", "10:00:03", false, "", 0);
    EXPECT_EQ(db->getMsgs(1, 2).size(), 3);
}

TEST_F(DatabaseTest, SaveZeroSizeFile) {
    db->saveMsg(1, 2, "empty", "10:10:00", true, "empty.txt", 0);
    auto msgs = db->getMsgs(1, 2);
    EXPECT_EQ(msgs[0].fileSize, 0);
}

TEST_F(DatabaseTest, GetMessagesFromEmptyDatabase) {
    auto msgs = db->getMsgs(1, 2);
    EXPECT_EQ(msgs.size(), 0);
}

TEST_F(DatabaseTest, GetMessagesBidirectional) {
    db->saveMsg(1, 2, "From Alice", "10:00", false, "", 0);
    db->saveMsg(2, 1, "From Bob", "10:01", false, "", 0);

    auto history = db->getMsgs(1, 2);
    EXPECT_EQ(history.size(), 2);
}

TEST_F(DatabaseTest, GetMessagesChronologicalOrder) {
    db->saveMsg(1, 2, "First", "09:00:00", false, "", 0);
    db->saveMsg(1, 2, "Second", "09:05:00", false, "", 0);

    auto history = db->getMsgs(1, 2);
    ASSERT_EQ(history.size(), 2);
    EXPECT_EQ(history[0].encryptedData, QByteArray("First"));
    EXPECT_EQ(history[1].encryptedData, QByteArray("Second"));
}

TEST_F(DatabaseTest, GetMessagesCorrectMetadata) {
    db->saveMsg(1, 2, "EncData", "11:00", true, "img.png", 5000);
    auto history = db->getMsgs(1, 2);
    ASSERT_EQ(history.size(), 1);
    EXPECT_TRUE(history[0].isFile);
    EXPECT_EQ(history[0].fileName, "img.png");
    EXPECT_EQ(history[0].fileSize, 5000);
}

TEST_F(DatabaseTest, GetMessagesIsolation) {
    db->saveMsg(1, 2, "Secret", "10:00", false, "", 0);
    db->saveMsg(3, 4, "Other", "10:00", false, "", 0);

    auto history = db->getMsgs(1, 2);
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].encryptedData, QByteArray("Secret"));
}

TEST_F(DatabaseTest, GetMessagesDataIntegrity) {
    QByteArray rawData("\x00\xFF\x00\xAA\xBB", 4);
    db->saveMsg(1, 2, rawData, "12:00", false, "", 0);

    auto history = db->getMsgs(1, 2);
    EXPECT_EQ(history[0].encryptedData, rawData);
}

TEST_F(DatabaseTest, MessageExistsPositive) {
    QByteArray data = "SomeData";
    QString time = "14:00:00";
    db->saveMsg(1, 2, data, time, false, "", 0);

    EXPECT_TRUE(db->isMessageExists(1, 2, time, data));
}

TEST_F(DatabaseTest, MessageExistsNegativeDifferentTime) {
    QByteArray data = "SomeData";
    db->saveMsg(1, 2, data, "14:00:00", false, "", 0);

    EXPECT_FALSE(db->isMessageExists(1, 2, "14:00:01", data));
}

TEST_F(DatabaseTest, MessageExistsNegativeDifferentData) {
    QString time = "14:00:00";
    db->saveMsg(1, 2, "DataA", time, false, "", 0);

    EXPECT_FALSE(db->isMessageExists(1, 2, time, QByteArray("DataB")));
}

TEST_F(DatabaseTest, MessageExistsNegativeEmptyDb) {
    EXPECT_FALSE(db->isMessageExists(1, 2, "10:00", "hello"));
}

TEST_F(DatabaseTest, MessageExistsBidirectionalCheck) {
    db->saveMsg(1, 2, "Hi", "10:00", false, "", 0);

    EXPECT_TRUE(db->isMessageExists(1, 2, "10:00", "Hi"));
    EXPECT_FALSE(db->isMessageExists(2, 1, "10:00", "Hi"));
}

TEST_F(DatabaseTest, MessageExistsWithNullInputs) {
    EXPECT_FALSE(db->isMessageExists(0, 0, "", QByteArray()));
}

TEST_F(DatabaseTest, SaveAndRetrieveContact) {
    db->saveContact(1, "Alice");
    auto contacts = db->getContactsWithLastMsg(2);
    EXPECT_EQ(contacts.size(), 0);
}

TEST_F(DatabaseTest, SaveAndRetrieveMessage) {
    db->saveMsg(1, 2, "enc_data", "10:00", false, "", 0);
    auto msgs = db->getMsgs(1, 2);
    ASSERT_EQ(msgs.size(), 1);
    EXPECT_EQ(msgs[0].encryptedData, QByteArray("enc_data"));
    EXPECT_FALSE(msgs[0].isFile);
}

TEST_F(DatabaseTest, SaveAndRetrieveFileMessage) {
    db->saveMsg(3, 4, "file_data", "11:00", true, "test.png", 1024);
    auto msgs = db->getMsgs(3, 4);
    ASSERT_EQ(msgs.size(), 1);
    EXPECT_TRUE(msgs[0].isFile);
    EXPECT_EQ(msgs[0].fileName, "test.png");
}

TEST_F(DatabaseTest, CheckMessageExists) {
    db->saveMsg(1, 2, "data1", "12:00", false, "", 0);

    EXPECT_TRUE(db->isMessageExists(1, 2, "12:00", "data1"));
    EXPECT_FALSE(db->isMessageExists(1, 2, "12:01", "data1"));
}

TEST_F(DatabaseTest, MessageOrderIsCorrect) {
    db->saveMsg(1, 2, "first", "01:00", false, "", 0);
    db->saveMsg(2, 1, "second", "02:00", false, "", 0);

    auto msgs = db->getMsgs(1, 2);
    ASSERT_EQ(msgs.size(), 2);
    EXPECT_EQ(msgs[0].encryptedData, QByteArray("first"));
    EXPECT_EQ(msgs[1].encryptedData, QByteArray("second"));
}