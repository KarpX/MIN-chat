#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <sqlite3.h>
#include <QString>
#include <QVector>

struct MessageRecord { int senderId; int receiverId; QByteArray encryptedData; QString timestamp; };
struct ContactRecord { int id; QString username; };

class DatabaseManager {
public:
    DatabaseManager(const QString& path = "chat_history.db") {
        sqlite3_open(path.toStdString().c_str(), &db);
        sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, s_id INTEGER, r_id INTEGER, data BLOB, time TEXT);", 0, 0, 0);
        sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS contacts (id INTEGER PRIMARY KEY, name TEXT);", 0, 0, 0);
    }
    ~DatabaseManager() { sqlite3_close(db); }

    bool isMessageExists(int sid, int rid, const QString& time, const QByteArray& data) {
        sqlite3_stmt* st;
        const char* sql = "SELECT id FROM messages WHERE s_id=? AND r_id=? AND time=? AND data=?;";
        sqlite3_prepare_v2(db, sql, -1, &st, 0);
        sqlite3_bind_int(st, 1, sid);
        sqlite3_bind_int(st, 2, rid);
        sqlite3_bind_text(st, 3, time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(st, 4, data.data(), data.size(), SQLITE_TRANSIENT);
        bool exists = (sqlite3_step(st) == SQLITE_ROW);
        sqlite3_finalize(st);
        return exists;
    }

    void saveContact(int id, const QString& name) {
        sqlite3_stmt* st; sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO contacts (id, name) VALUES (?, ?);", -1, &st, 0);
        sqlite3_bind_int(st, 1, id);
        sqlite3_bind_text(st, 2, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_step(st); sqlite3_finalize(st);
    }

    QVector<ContactRecord> getContacts(int myId) {
        QVector<ContactRecord> res; sqlite3_stmt* st;
        sqlite3_prepare_v2(db, "SELECT id, name FROM contacts WHERE id != ?;", -1, &st, 0);
        sqlite3_bind_int(st, 1, myId);
        while (sqlite3_step(st) == SQLITE_ROW) res.append({sqlite3_column_int(st, 0), QString::fromUtf8((const char*)sqlite3_column_text(st, 1))});
        sqlite3_finalize(st); return res;
    }

    void saveMsg(int sid, int rid, const QByteArray& d, const QString& t) {
        sqlite3_stmt* st; sqlite3_prepare_v2(db, "INSERT INTO messages (s_id, r_id, data, time) VALUES (?, ?, ?, ?);", -1, &st, 0);
        sqlite3_bind_int(st, 1, sid); sqlite3_bind_int(st, 2, rid);
        sqlite3_bind_blob(st, 3, d.data(), d.size(), SQLITE_TRANSIENT);
        sqlite3_bind_text(st, 4, t.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_step(st); sqlite3_finalize(st);
    }

    QVector<MessageRecord> getMsgs(int myId, int targetId) {
        QVector<MessageRecord> res; sqlite3_stmt* st;
        sqlite3_prepare_v2(db, "SELECT s_id, r_id, data, time FROM messages WHERE (s_id=? AND r_id=?) OR (s_id=? AND r_id=?) ORDER BY id ASC;", -1, &st, 0);
        sqlite3_bind_int(st, 1, myId); sqlite3_bind_int(st, 2, targetId);
        sqlite3_bind_int(st, 3, targetId); sqlite3_bind_int(st, 4, myId);
        while (sqlite3_step(st) == SQLITE_ROW) {
            res.append({sqlite3_column_int(st, 0), sqlite3_column_int(st, 1), QByteArray((const char*)sqlite3_column_blob(st, 2), sqlite3_column_bytes(st, 2)), QString::fromUtf8((const char*)sqlite3_column_text(st, 3))});
        }
        sqlite3_finalize(st); return res;
    }
private:
    sqlite3* db;
};
#endif