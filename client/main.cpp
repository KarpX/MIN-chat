#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ChatController.h"
#include <QDirIterator>
#include <QtQuickControls2/QQuickStyle>

int main(int argc, char *argv[]) {
    qputenv("QT_QUICK_CONTROLS_STYLE", "Material");
    QGuiApplication app(argc, argv);
    ChatController controller;
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("chatController", &controller);

    engine.load(QUrl(QStringLiteral("qrc:/ChatClient/main.qml")));
    return app.exec();
}