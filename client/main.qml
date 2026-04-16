import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

Window {
    visible: true
    width: 400
    height: 500
    title: "MINChat Test"
    color: "#eeeeee" // Светло-серый фон

    Connections {
        target: chatController
        // Имя функции в QML = "on" + ИмяСигнала (с большой буквы)
        function onNewMessageReceived(text) {
            console.log("QML получил сообщение:", text)
            messageModel.append({ "msg": text })
        }
        function onNetworkStatusChanged(status) {
            console.log("QML получил статус:", status)
            statusLabel.text = "Статус: " + status
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: "#333333" // Темная плашка для статуса
            radius: 5
            Text {
                id: statusLabel
                anchors.centerIn: parent
                text: "Статус: Ожидание..."
                color: "white" // Белый текст на темном фоне
                font.bold: true
            }
        }

        ListView {
            id: chatList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ListModel { id: messageModel }
            delegate: Rectangle {
                width: chatList.width
                height: 40
                color: index % 2 == 0 ? "#ffffff" : "#f9f9f9" // Чередование цветов строк
                border.color: "#dddddd"
                Text {
                    anchors.centerIn: parent
                    text: model.msg
                    color: "black" // ТЕКСТ ТОЧНО ЧЕРНЫЙ
                    font.pixelSize: 16
                }
            }
        }

        RowLayout {
            spacing: 10
            TextField {
                id: inputField
                Layout.fillWidth: true
                placeholderText: "Напишите что-нибудь..."
                color: "black" // Текст ввода черный
                focus: true
                onAccepted: sendBtn.clicked()
            }
            Button {
                id: sendBtn
                text: "ОТПРАВИТЬ"
                onClicked: {
                    if (inputField.text !== "") {
                        chatController.sendMessage(inputField.text)
                        inputField.text = ""
                    }
                }
            }
        }
    }
}