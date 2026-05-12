#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H
#include <QObject>
#include <QSet>
#include <QMap>
#include <QDateTime>
#include "../common/INetworkObserver.h"
#include "../common/CryptoManager.h"
#include "../common/DatabaseManager.h"
#include "NetworkClient.h"

class ChatController : public QObject, public INetworkObserver {
    Q_OBJECT
public:
    explicit ChatController(QObject *parent = nullptr);
    Q_INVOKABLE void login(QString u, QString p);
    Q_INVOKABLE void registerUser(QString u, QString p);
    Q_INVOKABLE void searchUser(QString n);
    Q_INVOKABLE void loadSavedContacts();
    Q_INVOKABLE void selectChat(int id, QString name);
    Q_INVOKABLE void sendMessage(QString text);
    Q_INVOKABLE void sendTypingStatus();
    Q_INVOKABLE void sendFile(const QString &fileUrl);
    Q_INVOKABLE void downloadFile(int peerId, QString timestamp, QString fileName);
    void onMessageReceived(const QByteArray &data) override;
    void onStatusChanged(const QString &status) override;

signals:
    void networkStatusChanged(QString s);
    void authSuccess(QString n);
    void authFailed();
    void userFound(QString n, int id, QString lastMsg);
    void userStatusChanged(int id, QString status);
    void newMessageReceived(const QString& text, bool isMe, const QString& timestamp, bool isFile = false,
    const QString& fileName = QString(), int fileSize = 0);
    void fileDownloaded(const QString& savePath);
    void fileDownloadError(const QString& fileName);

private:
    void loadHistory();

    int m_myId = -1;
    QString m_myName;
    int m_targetId = -1;
    QSet<QString>    m_sessionMsgs;
    QMap<int, QString> m_userStatuses;

    CryptoManager  m_crypto;
    NetworkClient  m_network;
    DatabaseManager m_db;
    const QByteArray KEY = "12345678901234567890123456789012";
};

#endif