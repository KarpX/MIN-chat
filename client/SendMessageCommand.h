#ifndef SENDMESSAGECOMMAND_H
#define SENDMESSAGECOMMAND_H

#include "../common/ICommand.h"
#include <QByteArray>

class NetworkClient;

class SendMessageCommand : public ICommand {
public:
    SendMessageCommand(NetworkClient* net, const QByteArray& data)
        : m_network(net), m_data(data) {}

    void execute() override;

private:
    NetworkClient* m_network;
    QByteArray m_data;
};

#endif