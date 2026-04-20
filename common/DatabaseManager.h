#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <sqlite3.h>
#include <QString>
#include <QByteArray>
#include <QVector>
#include "MessageModel.h"
#include "ContactModel.h"

class DatabaseManager {
public:
    DatabaseManager(const QString& path = "chat_history.db");
    ~DatabaseManager();
    bool isMessageExists(int sid, int rid, const QString& time, const QByteArray& data);
    void saveContact(int id, const QString& name);
    QVector<ContactRecord> getContacts(int myId);
    void saveMsg(int sid, int rid, const QByteArray& d, const QString& t);
    QVector<MessageRecord> getMsgs(int myId, int targetId);
    QVector<ContactRecord> getContactsWithLastMsg(int myId);
private:
    sqlite3* db;
    void createTables();
};

#endif