#include "MessageFactory.h"

std::unique_ptr<IMessage> MessageFactory::create(const QByteArray& rawData) {
    QByteArray cleanData = rawData.trimmed();
    QList<QByteArray> p = cleanData.split('|');
    if (p.isEmpty()) return nullptr;

    QByteArray cmd = p[0];

    if (cmd == "AUTH_OK" && p.size() >= 3) {
        return std::make_unique<AuthMessage>(true, p[1].toInt(), QString::fromUtf8(p[2]));
    }
    if (cmd == "AUTH_ERR") {
        return std::make_unique<AuthMessage>(false, -1, "");
    }
    if (cmd == "SRCH_RES" && p.size() >= 3) {
        return std::make_unique<SearchResultMessage>(p[1].toInt(), QString::fromUtf8(p[2]));
    }
    if (cmd == "STATUS" && p.size() >= 3) {
        return std::make_unique<StatusMessage>(p[1].toInt(), QString::fromUtf8(p[2]).trimmed());
    }
    if (cmd == "TYPING" && p.size() >= 2) {
        return std::make_unique<StatusMessage>(p[1].toInt(), "Печатает...");
    }
    if (cmd == "MSG" && p.size() >= 5) {
        return std::make_unique<TextMessage>(
            p[1].toInt(),
            QString::fromUtf8(p[2]),
            QByteArray::fromBase64(p[3]),
            QString::fromUtf8(p[4])
            );
    }
    if (cmd == "FILE" && p.size() >= 7) {
        QByteArray encData = QByteArray::fromBase64(p[5]);

        auto proxy = std::make_unique<FileMessageProxy>("", QString::fromUtf8(p[3]), p[4].toInt());

        return std::make_unique<FileMessage>(
            p[1].toInt(),
            QString::fromUtf8(p[2]),
            QString::fromUtf8(p[6]),
            encData,
            std::move(proxy)
        );
    }

    return nullptr;
}