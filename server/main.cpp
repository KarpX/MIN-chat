#include <QCoreApplication>
#include "ChatServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    ChatServer server;
    server.startServer(8080);

    return a.exec();
}