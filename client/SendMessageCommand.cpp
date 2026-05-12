#include "SendMessageCommand.h"
#include "NetworkClient.h"

void SendMessageCommand::execute() {
    m_network->rawSend(m_data);
}