#include <QCoreApplication>
#include <QCommandLineParser>
#include "ChatServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Защищенный сервер чата");
    parser.addHelpOption();

    QCommandLineOption portOption(QStringList() << "p" << "port", "Порт для прослушивания", "port", "8080");
    parser.addOption(portOption);
    parser.process(a);

    int port = parser.value(portOption).toInt();

    ChatServer server;
    server.startServer(port);

    return a.exec();
}