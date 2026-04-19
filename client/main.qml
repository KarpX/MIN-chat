import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

ApplicationWindow {
    id: window; visible: true; width: 1000; height: 700; title: "MINChat Secure"
    Material.theme: Material.Dark; Material.accent: Material.Blue

    Shortcut { sequence: "Escape"; onActivated: window.contentItem.forceActiveFocus() }
    background: Rectangle { color: "#121212"; MouseArea { anchors.fill: parent; onClicked: window.contentItem.forceActiveFocus() } }

    StackView { id: stackView; anchors.fill: parent; initialItem: loginPage }

    Component {
        id: loginPage
        Page {
            ColumnLayout {
                anchors.centerIn: parent; spacing: 20
                Label { text: "MINChat"; font.pixelSize: 32; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                TextField { id: uIn; placeholderText: "Имя пользователя"; Layout.preferredWidth: 300 }
                TextField { id: pIn; placeholderText: "Пароль"; echoMode: TextInput.Password; Layout.preferredWidth: 300 }
                Button { text: "ВОЙТИ"; Layout.fillWidth: true; onClicked: chatController.login(uIn.text, pIn.text) }
                Button { text: "РЕГИСТРАЦИЯ"; flat: true; Layout.fillWidth: true; onClicked: chatController.registerUser(uIn.text, pIn.text) }
            }
        }
    }

    Component {
        id: chatPage
        Page {
            Component.onCompleted: chatController.loadSavedContacts()
            RowLayout {
                anchors.fill: parent; spacing: 0
                Rectangle {
                    Layout.preferredWidth: 60; Layout.fillHeight: true; color: "#1a1a1a"
                    ColumnLayout {
                        anchors.fill: parent; anchors.topMargin: 20; spacing: 20
                        Button { text: "👤"; flat: true; Layout.alignment: Qt.AlignHCenter; onClicked: { searchField.text = ""; window.contentItem.forceActiveFocus() } }
                        Button { text: "⚙️"; flat: true; Layout.alignment: Qt.AlignHCenter }
                        Item { Layout.fillHeight: true }
                        Text { id: connStat; text: "..."; color: "gray"; font.pixelSize: 10; Layout.alignment: Qt.AlignHCenter }
                    }
                }
                Rectangle {
                    Layout.preferredWidth: 300; Layout.fillHeight: true; color: "#252525"; border.color: "#333333"
                    ColumnLayout {
                        anchors.fill: parent; spacing: 0
                        TextField {
                            id: searchField; placeholderText: "Поиск..."; Layout.fillWidth: true; Layout.margins: 5
                            onTextChanged: { contactsModel.clear(); if(text.trim()==="") chatController.loadSavedContacts(); else chatController.searchUser(text) }
                            onAccepted: window.contentItem.forceActiveFocus()
                        }
                        ListView {
                            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                            model: ListModel { id: contactsModel }
                            delegate: ItemDelegate {
                                width: parent.width; text: model.name
                                onClicked: { curName.text = model.name; messageModel.clear(); chatController.selectChat(model.userId, model.name); window.contentItem.forceActiveFocus() }
                            }
                        }
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; color: "#121212"
                    property bool isSel: curName.text !== "Выберите чат"
                    ColumnLayout {
                        anchors.fill: parent; spacing: 0
                        Rectangle { Layout.fillWidth: true; height: 60; color: "#252525"; Label { id: curName; text: "Выберите чат"; anchors.centerIn: parent; font.bold: true } }
                        ListView {
                            id: msgList; Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 15; spacing: 10; clip: true; visible: parent.parent.isSel
                            model: ListModel { id: messageModel }
                            delegate: RowLayout {
                                width: msgList.width; layoutDirection: model.isMe ? Qt.RightToLeft : Qt.LeftToRight
                                Rectangle {
                                    Layout.preferredWidth: Math.min(txt.implicitWidth + 30, 400); Layout.preferredHeight: txt.implicitHeight + 25
                                    color: model.isMe ? "#2196F3" : "#333333"; radius: 10
                                    Text { id: txt; text: model.msg; anchors.centerIn: parent; color: "white"; wrapMode: Text.WordWrap; width: parent.width - 20 }
                                    Text { text: model.time; anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 3; font.pixelSize: 8; color: "lightgray" }
                                }
                            }
                            onCountChanged: msgList.currentIndex = count - 1
                        }
                        Rectangle {
                            Layout.fillWidth: true; height: 70; color: "#252525"; visible: parent.parent.isSel
                            RowLayout {
                                anchors.fill: parent; anchors.margins: 10; spacing: 10
                                TextField { id: msgIn; Layout.fillWidth: true; placeholderText: "Сообщение..."; onAccepted: sBtn.clicked() }
                                Button { id: sBtn; text: "➤"; onClicked: { if(msgIn.text!==""){ chatController.sendMessage(msgIn.text); msgIn.text=""; window.contentItem.forceActiveFocus() } } }
                            }
                        }
                        Label { text: "Выберите чат"; anchors.centerIn: parent; color: "gray"; visible: !parent.parent.isSel }
                    }
                }
            }
            Connections {
                target: chatController
                function onUserFound(n, id) {
                    for(var i=0; i<contactsModel.count; i++) if(contactsModel.get(i).userId === id) return;
                    contactsModel.append({"name": n, "userId": id})
                }
                function onNewMessageReceived(t, m, tm) {
                    // Проверка на дубликаты перед отрисовкой
                    for(var j=0; j<messageModel.count; j++) {
                        if(messageModel.get(j).msg === t && messageModel.get(j).time === tm) return;
                    }
                    messageModel.append({"msg": t, "isMe": m, "time": tm})
                }
                function onNetworkStatusChanged(s) { connStat.text = s }
            }
        }
    }
    Connections { target: chatController; function onAuthSuccess(n) { stackView.push(chatPage) } }
}