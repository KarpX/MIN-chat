#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ChatController.h"
#include <QDirIterator>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    ChatController controller;
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("chatController", &controller);

    engine.load(QUrl(QStringLiteral("qrc:/ChatClient/main.qml")));
    return app.exec();
}