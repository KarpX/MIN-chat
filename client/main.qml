import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    visible: true
    width: 1100
    height: 720
    title: "MINChat Secure"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    background: Rectangle {
        color: "#0f0f0f"
        MouseArea { anchors.fill: parent; onClicked: window.contentItem.forceActiveFocus() }
    }

    Shortcut { sequence: "Escape"; onActivated: window.contentItem.forceActiveFocus() }

    StackView { id: stackView; anchors.fill: parent; initialItem: loginPage }

    property var userStatuses: ({})

    function statusColor(s) {
        if (s === "В сети")      return "#4CAF50"
        if (s === "Печатает...") return "#FFC107"
        return "#555"
    }

    // ─── Toast-уведомление ──────────────────────────────────────────────────
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32
        width: toastText.implicitWidth + 40
        height: 44
        radius: 22
        color: toastSuccess ? "#1b5e20" : "#b71c1c"
        opacity: 0
        z: 999

        property bool toastSuccess: true

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 14
        }

        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: toast; property: "opacity"; to: 1;    duration: 200 }
            PauseAnimation  { duration: 2400 }
            NumberAnimation { target: toast; property: "opacity"; to: 0;    duration: 300 }
        }

        function show(text, success) {
            toastText.text  = text
            toast.toastSuccess = success !== false
            opacity = 0
            toastAnim.restart()
        }
    }

    // ─── FloatField ─────────────────────────────────────────────────────────
    component FloatField: Item {
        id: ff
        height: 62
        property alias text: inp.text
        property alias echoMode: inp.echoMode
        property string label: ""
        signal editingFinished()
        readonly property bool floating: inp.activeFocus || inp.text.length > 0
        Text {
            id: lbl; text: ff.label
            color: inp.activeFocus ? "#1565C0" : "#777"
            font.pixelSize: ff.floating ? 11 : 14
            y: ff.floating ? 0 : 24; x: 14; z: 2
            Rectangle {
                visible: ff.floating; anchors.centerIn: parent
                width: parent.contentWidth + 8; height: parent.contentHeight + 2
                color: "#0f0f0f"; z: -1
            }
            Behavior on y              { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            Behavior on font.pixelSize { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            Behavior on color          { ColorAnimation  { duration: 150 } }
        }
        Rectangle {
            id: fieldBox; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: 46; color: "#1a1a1a"; radius: 8
            border.color: inp.activeFocus ? "#1565C0" : "#2e2e2e"
            border.width: inp.activeFocus ? 2 : 1
        }
        TextInput {
            id: inp; anchors.left: fieldBox.left; anchors.right: fieldBox.right; anchors.verticalCenter: fieldBox.verticalCenter
            anchors.leftMargin: 14; anchors.rightMargin: 14; font.pixelSize: 14; color: "white"
            selectionColor: "#1565C0"; selectedTextColor: "white"; clip: true
            Keys.onReturnPressed: ff.editingFinished()
            Keys.onEnterPressed:  ff.editingFinished()
        }
        MouseArea { anchors.fill: fieldBox; cursorShape: Qt.IBeamCursor; onPressed: { inp.forceActiveFocus(); mouse.accepted = false } }
    }

    // ─── Страница авторизации ────────────────────────────────────────────────
    Component {
        id: loginPage
        Page {
            id: loginPage_item
            background: Rectangle { color: "#0f0f0f" }
            property bool loginError: false
            Connections {
                target: chatController
                function onAuthFailed() { loginPage_item.loginError = true }
            }
            ColumnLayout {
                anchors.centerIn: parent; spacing: 0; width: 340
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter; spacing: 6; Layout.bottomMargin: 40
                    Rectangle { width: 56; height: 56; radius: 16; color: "#1565C0"; Layout.alignment: Qt.AlignHCenter
                        Text { anchors.centerIn: parent; text: "M"; font.pixelSize: 28; font.bold: true; color: "white" } }
                    Label { text: "MINChat"; font.pixelSize: 28; font.bold: true; color: "white"; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "Защищённый мессенджер"; font.pixelSize: 13; color: "#555"; Layout.alignment: Qt.AlignHCenter }
                }
                FloatField { id: uIn; Layout.fillWidth: true; label: "Имя пользователя"; Layout.bottomMargin: 18; onEditingFinished: pIn.inp.forceActiveFocus() }
                FloatField { id: pIn; Layout.fillWidth: true; label: "Пароль"; echoMode: TextInput.Password; Layout.bottomMargin: 20
                    onEditingFinished: { loginPage_item.loginError = false; chatController.login(uIn.text, pIn.text) } }
                Label { text: "Неверные данные пользователя"; font.pixelSize: 13; color: "#ef5350"; Layout.alignment: Qt.AlignHCenter; visible: loginPage_item.loginError; Layout.bottomMargin: 8 }
                Button { text: "ВОЙТИ"; Layout.fillWidth: true; height: 46; font.bold: true; font.pixelSize: 13; Material.background: "#1565C0"; Material.foreground: "white"; Layout.bottomMargin: 8
                    onClicked: { loginPage_item.loginError = false; chatController.login(uIn.text, pIn.text) } }
                Button { text: "РЕГИСТРАЦИЯ"; Layout.fillWidth: true; flat: true; font.pixelSize: 13; onClicked: { loginPage_item.loginError = false; chatController.registerUser(uIn.text, pIn.text) } }
            }
        }
    }

    // ─── Страница чата ───────────────────────────────────────────────────────
    Component {
        id: chatPage
        Page {
            id: chatPage_root
            background: Rectangle { color: "#0f0f0f" }
            Component.onCompleted: chatController.loadSavedContacts()

            property int currentChatId: -1
            property string activeChatStatus: "Не в сети"
            property bool typingCooldown: false

            function internalUpdateStatus(id, status) {
                var m = window.userStatuses
                m[id] = status
                window.userStatuses = m
                if (id === chatPage_root.currentChatId) chatPage_root.activeChatStatus = status
                for (var i = 0; i < contactsModel.count; i++) {
                    if (contactsModel.get(i).userId === id) {
                        contactsModel.setProperty(i, "status", status); break
                    }
                }
                if (status === "Печатает...") { typingResetTimer.targetId = id; typingResetTimer.restart() }
            }

            Timer { id: typingResetTimer; property int targetId: -1; interval: 3500; repeat: false
                onTriggered: { if (targetId !== -1) chatPage_root.internalUpdateStatus(targetId, "В сети") } }
            Timer { id: typingCooldownTimer; interval: 2000; repeat: false; onTriggered: chatPage_root.typingCooldown = false }

            FileDialog {
                id: fileDialog
                title: "Выберите файл для отправки"
                onAccepted: {
                    // selectedFile — это url, ChatController.sendFile принимает QString и сам делает QUrl::toLocalFile
                    chatController.sendFile(fileDialog.selectedFile.toString())
                }
            }

            Dialog {
                id: fingerprintDialog
                anchors.centerIn: parent
                width: 320
                modal: true
                title: "Отпечаток безопасности"
                background: Rectangle { color: "#1e1e1e"; radius: 12; border.color: "#333" }

                property string emojis: ""

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16

                    Label {
                        text: "Сравните эти эмодзи с экраном вашего собеседника. Если они совпадают, чат надежно зашифрован (E2EE)."
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: "#ccc"
                        font.pixelSize: 13
                    }

                    Text {
                        text: fingerprintDialog.emojis
                        font.pixelSize: 42
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Button {
                        text: "ПОНЯТНО"
                        Layout.alignment: Qt.AlignHCenter
                        Material.background: "#1565C0"
                        Material.foreground: "white"
                        onClicked: fingerprintDialog.close()
                    }
                }
            }

            RowLayout {
                anchors.fill: parent; spacing: 0

                // ── Боковая иконка-панель ────────────────────────────────────
                Rectangle {
                    Layout.preferredWidth: 64; Layout.fillHeight: true; color: "#141414"
                    ColumnLayout {
                        anchors.fill: parent; anchors.topMargin: 16; spacing: 8
                        Button { text: "👤"; flat: true; font.pixelSize: 18; Layout.alignment: Qt.AlignHCenter; onClicked: { searchField.text = ""; window.contentItem.forceActiveFocus() } }
                        Button { text: "⚙️"; flat: true; font.pixelSize: 18; Layout.alignment: Qt.AlignHCenter }
                        Item { Layout.fillHeight: true }
                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter; Layout.bottomMargin: 12; spacing: 4
                            Rectangle { width: 8; height: 8; radius: 4; color: connStat.text === "Online" ? "#4CAF50" : "#555"; Layout.alignment: Qt.AlignHCenter }
                            Text { id: connStat; text: "..."; color: "#555"; font.pixelSize: 9; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#222" }

                // ── Список контактов ─────────────────────────────────────────
                Rectangle {
                    Layout.preferredWidth: 300; Layout.fillHeight: true; color: "#181818"
                    ColumnLayout {
                        anchors.fill: parent; spacing: 0
                        Rectangle { Layout.fillWidth: true; height: 60; color: "#1e1e1e"
                            Label { text: "Чаты"; font.pixelSize: 17; font.bold: true; color: "white"; anchors.left: parent.left; anchors.leftMargin: 16; anchors.verticalCenter: parent.verticalCenter } }
                        TextField {
                            id: searchField; placeholderText: "Поиск..."
                            Layout.fillWidth: true; Layout.margins: 10; font.pixelSize: 13
                            background: Rectangle { color: "#252525"; border.color: searchField.activeFocus ? "#1565C0" : "#333"; border.width: 1; radius: 20 }
                            leftPadding: 16
                            onTextChanged: { contactsModel.clear(); if (text.trim() === "") chatController.loadSavedContacts(); else chatController.searchUser(text) }
                        }
                        ListView {
                            id: contactsListView; Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                            model: ListModel { id: contactsModel }
                            delegate: ItemDelegate {
                                id: contactDelegate; width: parent.width; height: 72
                                highlighted: chatPage_root.currentChatId === model.userId
                                background: Rectangle {
                                    color: contactDelegate.highlighted ? "#1a2a3a" : (contactDelegate.hovered ? "#202020" : "transparent")
                                    Rectangle { anchors.left: parent.left; width: 3; height: parent.height; color: "#1565C0"; visible: contactDelegate.highlighted; radius: 2 }
                                }
                                onClicked: {
                                    curName.text = model.name
                                    chatPage_root.currentChatId = model.userId
                                    chatPage_root.activeChatStatus = model.status || "Не в сети"
                                    messageModel.clear()
                                    chatController.selectChat(model.userId, model.name)
                                    window.contentItem.forceActiveFocus()
                                }
                                RowLayout {
                                    anchors.fill: parent; anchors.margins: 12; spacing: 12
                                    Item { width: 46; height: 46
                                        Rectangle { anchors.fill: parent; radius: 23; color: Qt.hsla((model.userId * 37) % 360 / 360, 0.5, 0.35, 1)
                                            Text { anchors.centerIn: parent; text: model.name.charAt(0).toUpperCase(); font.pixelSize: 18; font.bold: true; color: "white" } }
                                        Rectangle { width: 13; height: 13; radius: 6.5; color: window.statusColor(model.status); border.color: "#181818"; border.width: 2; anchors.right: parent.right; anchors.bottom: parent.bottom
                                            Behavior on color { ColorAnimation { duration: 250 } } }
                                    }
                                    ColumnLayout { Layout.fillWidth: true; spacing: 2
                                        Label { text: model.name; font.pixelSize: 14; font.bold: true; color: "white"; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Label {
                                            text: model.status === "Печатает..." ? "Печатает..." : (model.lastMsg || "Нет сообщений")
                                            font.pixelSize: 12
                                            color: model.status === "Печатает..." ? "#FFC107" : "#555"
                                            elide: Text.ElideRight; Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle { width: 1; Layout.fillHeight: true; color: "#222" }

                // ── Область чата ─────────────────────────────────────────────
                Rectangle {
                    id: chatArea; Layout.fillWidth: true; Layout.fillHeight: true; color: "#0f0f0f"
                    property bool isSel: chatPage_root.currentChatId !== -1

                    ColumnLayout {
                        anchors.fill: parent; spacing: 0

                        // Шапка чата
                        Rectangle { Layout.fillWidth: true; height: 64; color: "#181818"
                            RowLayout { anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 20; spacing: 14
                                Item { width: 40; height: 40; visible: chatArea.isSel
                                    Rectangle { anchors.fill: parent; radius: 20; color: Qt.hsla((chatPage_root.currentChatId * 37) % 360 / 360, 0.5, 0.35, 1)
                                        Text { anchors.centerIn: parent; text: curName.text.charAt(0).toUpperCase(); font.pixelSize: 15; font.bold: true; color: "white" } } }
                                ColumnLayout { spacing: 2
                                    RowLayout {
                                        spacing: 8
                                        Label { id: curName; text: "Выберите чат"; font.pixelSize: 16; font.bold: true; color: "white" }

                                        // НОВАЯ КНОПКА-ЗАМОК
                                        Rectangle {
                                            visible: chatArea.isSel
                                            Layout.preferredWidth: 28
                                            Layout.preferredHeight: 28
                                            Layout.alignment: Qt.AlignVCenter

                                            color: "transparent"
                                            radius: 14

                                            Text {
                                                anchors.centerIn: parent
                                                text: "🔒"
                                                font.pixelSize: 16
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                hoverEnabled: true // Включаем реакцию на наведение мыши

                                                onEntered: parent.color = "#333333" // Серый кружок при наведении
                                                onExited: parent.color = "transparent" // Убираем кружок

                                                onClicked: {
                                                    fingerprintDialog.emojis = chatController.getSecurityFingerprint(chatPage_root.currentChatId)
                                                    fingerprintDialog.open()
                                                }
                                            }
                                        }
                                    }
                                    RowLayout { spacing: 6; visible: chatArea.isSel
                                        Rectangle { width: 7; height: 7; radius: 3.5; color: window.statusColor(chatPage_root.activeChatStatus); Behavior on color { ColorAnimation { duration: 250 } } }
                                        Label { text: chatPage_root.activeChatStatus; font.pixelSize: 12; color: window.statusColor(chatPage_root.activeChatStatus); Behavior on color { ColorAnimation { duration: 250 } } }
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#222" }
                        }

                        // Список сообщений
                        ListView {
                            id: msgList
                            Layout.fillWidth: true; Layout.fillHeight: true
                            topMargin: 20; bottomMargin: 20; leftMargin: 20; rightMargin: 20
                            spacing: 16; clip: true
                            visible: chatArea.isSel
                            model: ListModel { id: messageModel }

                            delegate: Item {
                                id: delegateRoot
                                width: msgList.width - msgList.leftMargin - msgList.rightMargin
                                height: bubble.height + 2   // небольшой зазор снизу

                                // Скрытый Text для измерения ширины только для не-файловых сообщений
                                Text {
                                    id: sizer
                                    visible: false
                                    text: model.isFile ? "" : (model.msg || "")
                                    font.pixelSize: 15
                                    wrapMode: Text.NoWrap
                                }

                                readonly property real maxBubble: delegateRoot.width * 0.72
                                // Файлы — фиксированная ширина 300px; текст — по содержимому
                                readonly property real bubbleW: model.isFile
                                                                ? 300
                                                                : Math.min(sizer.implicitWidth + 32, maxBubble)

                                Rectangle {
                                    id: bubble
                                    width: delegateRoot.bubbleW
                                    height: contentLayout.implicitHeight + 20
                                    radius: 14
                                    color: model.isMe ? "#1565C0" : "#242424"
                                    anchors.right: model.isMe ? parent.right : undefined
                                    anchors.left:  model.isMe ? undefined    : parent.left

                                    // «хвостик» пузыря
                                    Rectangle {
                                        width: 12; height: 12; color: parent.color; radius: 2
                                        anchors.bottom: parent.bottom
                                        anchors.right: model.isMe ? parent.right : undefined
                                        anchors.left:  model.isMe ? undefined    : parent.left
                                    }

                                    ColumnLayout {
                                        id: contentLayout
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: 10
                                        spacing: 4

                                        // ── Текстовое сообщение ──────────────────────────────
                                        Text {
                                            visible: !model.isFile
                                            text: model.msg || ""
                                            color: "white"
                                            font.pixelSize: 15
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }

                                        // ── Файловое сообщение ───────────────────────────────
                                        RowLayout {
                                            visible: model.isFile === true
                                            spacing: 10
                                            Layout.fillWidth: true

                                            // Иконка файла
                                            Text { text: "📄"; font.pixelSize: 30; Layout.alignment: Qt.AlignVCenter }

                                            // Имя + размер
                                            ColumnLayout {
                                                spacing: 2
                                                Layout.fillWidth: true
                                                Text {
                                                    text: model.fileName || ""
                                                    color: "white"; font.bold: true; font.pixelSize: 14
                                                    elide: Text.ElideMiddle
                                                    Layout.fillWidth: true
                                                }
                                                Text {
                                                    visible: model.fileSize > 0
                                                    text: {
                                                        var s = model.fileSize || 0
                                                        if (s < 1024)        return s + " Б"
                                                        if (s < 1024*1024)   return (s / 1024).toFixed(1) + " КБ"
                                                        return (s / (1024*1024)).toFixed(2) + " МБ"
                                                    }
                                                    color: "#aaa"; font.pixelSize: 12
                                                }
                                            }

                                            // Кнопка скачать — показываем для ВСЕХ файловых сообщений
                                            Button {
                                                visible: model.isFile === true

                                                // Указываем размеры строго для Layout
                                                Layout.preferredWidth: 40
                                                Layout.preferredHeight: 40
                                                Layout.alignment: Qt.AlignVCenter

                                                // 1. Рисуем свой фон (отключаем Material Background)
                                                background: Rectangle {
                                                    color: model.isMe ? "#1565C0" : "#388E3C"
                                                    radius: 8 // Немного скруглим углы
                                                }

                                                // 2. Рисуем свой текст (отключаем обрезку Material-стиля)
                                                contentItem: Text {
                                                    text: "⬇️"
                                                    font.pixelSize: 20
                                                    color: "white"
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }

                                                onClicked: {
                                                    chatController.downloadFile(
                                                        chatPage_root.currentChatId,
                                                        model.time,
                                                        model.fileName
                                                    )
                                                }
                                            }
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                            height: timeText.implicitHeight
                                            Text {
                                                id: timeText
                                                text: model.time.substring(0, 5) || ""
                                                color: model.isMe ? "#90CAF9" : "#555"
                                                font.pixelSize: 11
                                                anchors.right: parent.right
                                            }
                                        }
                                    }
                                }
                            }

                            onCountChanged: Qt.callLater(function() { positionViewAtEnd() })
                        }

                        // Заглушка «Выберите чат»
                        Item {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            visible: !chatArea.isSel
                            ColumnLayout { anchors.centerIn: parent; spacing: 12
                                Text { text: "💬"; font.pixelSize: 48; Layout.alignment: Qt.AlignHCenter }
                                Label { text: "Выберите чат"; color: "#444"; font.pixelSize: 16; Layout.alignment: Qt.AlignHCenter }
                            }
                        }

                        // Панель ввода
                        Rectangle {
                            Layout.fillWidth: true; height: 72; color: "#141414"
                            visible: chatArea.isSel
                            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#222" }
                            RowLayout { anchors.fill: parent; anchors.margins: 12; spacing: 10
                                Button {
                                    text: "📎"
                                    width: 46; height: 46; font.pixelSize: 22
                                    Material.background: "transparent"
                                    Material.foreground: "#777"
                                    onClicked: fileDialog.open()
                                }
                                TextField {
                                    id: msgIn
                                    Layout.fillWidth: true
                                    placeholderText: "Написать сообщение..."
                                    font.pixelSize: 14
                                    background: Rectangle { color: "#1e1e1e"; border.color: msgIn.activeFocus ? "#1565C0" : "#2e2e2e"; border.width: 1; radius: 22 }
                                    leftPadding: 18; rightPadding: 18
                                    onTextChanged: {
                                        if (text.length > 0 && !chatPage_root.typingCooldown) {
                                            chatController.sendTypingStatus()
                                            chatPage_root.typingCooldown = true
                                            typingCooldownTimer.restart()
                                        }
                                    }
                                    onAccepted: {
                                        if (text.trim() !== "") {
                                            chatController.sendMessage(text)
                                            text = ""
                                        }
                                    }
                                }
                                Button {
                                    text: "➤"; width: 46; height: 46; font.pixelSize: 18
                                    Material.background: "#1565C0"; Material.foreground: "white"
                                    onClicked: {
                                        if (msgIn.text.trim() !== "") {
                                            chatController.sendMessage(msgIn.text)
                                            msgIn.text = ""
                                            window.contentItem.forceActiveFocus()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Connections ──────────────────────────────────────────────────
            Connections {
                target: chatController

                function onUserFound(n, id, last) {
                    for (var i = 0; i < contactsModel.count; i++) {
                        if (contactsModel.get(i).userId === id) {
                            if (last !== "") contactsModel.setProperty(i, "lastMsg", last)
                            if (n    !== "") contactsModel.setProperty(i, "name",    n)
                            return
                        }
                    }
                    contactsModel.append({
                        "name":    n,
                        "userId":  id,
                        "lastMsg": last,
                        "status":  window.userStatuses[id] || "Не в сети"
                    })
                }

                function onNewMessageReceived(t, isMe, tm, isFile, fName, fSize) {
                    if (!isMe) {
                        var key = isFile ? (tm + "|" + fName) : (tm + "|" + t)
                        for (var j = 0; j < messageModel.count; j++) {
                            var item = messageModel.get(j)
                            var itemKey = item.isFile ? (item.time + "|" + item.fileName) : (item.time + "|" + item.msg)
                            if (itemKey === key && item.isMe === isMe) return
                        }
                    }

                    messageModel.append({
                        "msg":      t      || "",
                        "isMe":     isMe   || false,
                        "time":     tm     || "",
                        "isFile":   isFile || false,
                        "fileName": fName  || "",
                        "fileSize": fSize  || 0
                    })

                    // Обновляем превью последнего сообщения в списке контактов
                    for (var k = 0; k < contactsModel.count; k++) {
                        if (contactsModel.get(k).userId === chatPage_root.currentChatId) {
                            var preview = (isMe ? "Вы: " : "") + (isFile ? "📎 " + fName : t)
                            contactsModel.setProperty(k, "lastMsg", preview)
                            contactsModel.move(k, 0, 1)
                            break
                        }
                    }
                }

                function onNetworkStatusChanged(s) { connStat.text = s }

                function onUserStatusChanged(id, status) { chatPage_root.internalUpdateStatus(id, status) }

                // Сигнал успешного скачивания файла
                function onFileDownloaded(path) {
                    var fname = path.replace(/.*[\/\\]/, "")   // только имя файла
                    toast.show("✓ Сохранено: " + fname + "\nПуть: " + path, true)
                }

                // Сигнал ошибки скачивания
                function onFileDownloadError(fileName) {
                    toast.show("✗ Не удалось скачать: " + fileName, false)
                }
            }
        }
    }

    Connections { target: chatController; function onAuthSuccess(n) { stackView.push(chatPage) } }
}
