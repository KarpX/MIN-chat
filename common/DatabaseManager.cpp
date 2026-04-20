#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(const QString& path) {
    sqlite3_open(path.toStdString().c_str(), &db);
    createTables();
}

DatabaseManager::~DatabaseManager() {
    sqlite3_close(db);
}

void DatabaseManager::createTables() {
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, s_id INTEGER, r_id INTEGER, data BLOB, time TEXT);", 0, 0, 0);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS contacts (id INTEGER PRIMARY KEY, name TEXT);", 0, 0, 0);
}

bool DatabaseManager::isMessageExists(int sid, int rid, const QString& time, const QByteArray& data) {
    sqlite3_stmt* st;
    sqlite3_prepare_v2(db, "SELECT id FROM messages WHERE s_id=? AND r_id=? AND time=? AND data=?;", -1, &st, 0);
    sqlite3_bind_int(st, 1, sid);
    sqlite3_bind_int(st, 2, rid);
    sqlite3_bind_text(st, 3, time.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(st, 4, data.data(), data.size(), SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(st) == SQLITE_ROW);
    sqlite3_finalize(st);
    return exists;
}

void DatabaseManager::saveContact(int id, const QString& name) {
    sqlite3_stmt* st;
    sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO contacts (id, name) VALUES (?, ?);", -1, &st, 0);
    sqlite3_bind_int(st, 1, id);
    sqlite3_bind_text(st, 2, name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_step(st);
    sqlite3_finalize(st);
}

QVector<ContactRecord> DatabaseManager::getContactsWithLastMsg(int myId) {
    QVector<ContactRecord> res;
    sqlite3_stmt* st;

    // ИСПРАВЛЕНИЕ: Используем LEFT JOIN и COALESCE, чтобы чат не пропадал, если имени нет в БД
    const char* sql =
        "SELECT latest.peer_id, COALESCE(c.name, 'User ' || latest.peer_id), m.data, (m.s_id = ?) as is_me "
        "FROM ("
        "  SELECT MAX(id) as last_id, "
        "  CASE WHEN s_id = ? THEN r_id ELSE s_id END as peer_id "
        "  FROM messages "
        "  WHERE s_id = ? OR r_id = ? "
        "  GROUP BY peer_id"
        ") latest "
        "LEFT JOIN contacts c ON c.id = latest.peer_id "
        "JOIN messages m ON m.id = latest.last_id "
        "ORDER BY latest.last_id DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &st, 0) == SQLITE_OK) {
        sqlite3_bind_int(st, 1, myId);
        sqlite3_bind_int(st, 2, myId);
        sqlite3_bind_int(st, 3, myId);
        sqlite3_bind_int(st, 4, myId);

        while (sqlite3_step(st) == SQLITE_ROW) {
            ContactRecord rec;
            rec.id = sqlite3_column_int(st, 0);
            rec.username = QString::fromUtf8((const char*)sqlite3_column_text(st, 1));
            rec.lastEncryptedData = QByteArray((const char*)sqlite3_column_blob(st, 2), sqlite3_column_bytes(st, 2));
            rec.lastWasMe = (sqlite3_column_int(st, 3) > 0);
            res.append(rec);
        }
    }
    sqlite3_finalize(st);
    return res;
}

void DatabaseManager::saveMsg(int sid, int rid, const QByteArray& d, const QString& t) {
    sqlite3_stmt* st;
    sqlite3_prepare_v2(db, "INSERT INTO messages (s_id, r_id, data, time) VALUES (?, ?, ?, ?);", -1, &st, 0);
    sqlite3_bind_int(st, 1, sid);
    sqlite3_bind_int(st, 2, rid);
    sqlite3_bind_blob(st, 3, d.data(), d.size(), SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, t.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_step(st);
    sqlite3_finalize(st);
}

QVector<MessageRecord> DatabaseManager::getMsgs(int myId, int targetId) {
    QVector<MessageRecord> res;
    sqlite3_stmt* st;
    sqlite3_prepare_v2(db, "SELECT s_id, r_id, data, time FROM messages WHERE (s_id=? AND r_id=?) OR (s_id=? AND r_id=?) ORDER BY id ASC;", -1, &st, 0);
    sqlite3_bind_int(st, 1, myId);
    sqlite3_bind_int(st, 2, targetId);
    sqlite3_bind_int(st, 3, targetId);
    sqlite3_bind_int(st, 4, myId);

    while (sqlite3_step(st) == SQLITE_ROW) {
        MessageRecord rec;
        rec.senderId = sqlite3_column_int(st, 0);
        rec.receiverId = sqlite3_column_int(st, 1);
        rec.encryptedData = QByteArray((const char*)sqlite3_column_blob(st, 2), sqlite3_column_bytes(st, 2));
        rec.timestamp = QString::fromUtf8((const char*)sqlite3_column_text(st, 3));
        res.append(rec);
    }
    sqlite3_finalize(st);
    return res;
}