import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config
import "../components"


Rectangle {
    id: serverConnectionPage
    // An den sichtbaren Bereich (StackView unterhalb der Topbar) binden, nicht
    // an das ganze Fenster – sonst ist die Box vertikal nicht zentriert.
    width: mainStackView.width
    height: mainStackView.height
    color: "transparent"

    Image {
        id: serverConnectionBackground
        anchors.fill: parent
        source: "../resources/startWindowBackground.png"
        fillMode: Image.PreserveAspectCrop
    }

    Component.onCompleted: {
        // Load saved credentials from config
        usernameInput.text = ServerConnection.savedUsername
        passwordInput.text = ServerConnection.savedPassword
        rememberMeCheckbox.checked = ServerConnection.rememberPassword
    }

    // Connections to backend signals
    Connections {
        target: ServerConnection

        function onConnectionProgressChanged(progress) {
            connectionProgress.value = progress
        }

        function onStatusMessageChanged(message) {
            statusText.text = message
        }

        function onConnectionSucceeded() {
            // console.log("Connection succeeded!")
        }

        function onConnectionFailed(errorMessage) {
            // console.log("Connection failed:", errorMessage)
            statusText.text = errorMessage
            statusText.color = Config.Theme.colorError
            // Lesezeit an die Meldungslänge koppeln: Die ausführlichen Texte
            // (z.B. Verbindungsaufbau fehlgeschlagen) waren in den festen
            // 3,5 s nicht zu lesen, bevor die Seite zurücksprang.
            errorResetTimer.interval = Math.min(12000, 3500 + errorMessage.length * 50)
            errorResetTimer.restart()
        }

        function onShowLobby() {
            // console.log("[NAV] onShowLobby → replace currentItem with LobbyPage.qml | depth before:", mainStackView.depth)
            mainStackView.replace(mainStackView.currentItem, "LobbyPage.qml")
        }
    }

    Timer {
        id: errorResetTimer
        interval: 3500
        repeat: false
        onTriggered: {
            mainStack.currentIndex = 0
            statusText.color = Config.StaticData.palette.secondary.col300
        }
    }

    Flickable {
        id: serverConnScroller
        anchors.fill: parent
        contentWidth: serverConnScroller.width
        contentHeight: serverConnScrollContent.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {
            // AlwaysOff statt AsNeeded: AsNeeded blendet die Bar bei jeder
            // contentHeight-Änderung transient ein (mehrsekündiges Fade) –
            // sichtbar als kurz aufblitzende Scrollbar beim Seitenaufbau,
            // obwohl nichts zu scrollen ist.
            policy: serverConnScroller.contentHeight > serverConnScroller.height + 1
                    ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        Item {
            id: serverConnScrollContent
            width: serverConnScroller.width
            // Mindesthöhe = Viewport → Box bleibt vertikal zum Fensterrand
            // zentriert; bei zu niedrigem Fenster kann gescrollt werden.
            height: Math.max(serverConnScroller.height,
                             Config.Theme.brandBoxHeight + Config.Theme.margin * 2)

        // Card – wie auf der StartPage (vertikal zentriert)
        Rectangle {
            id: loginCard
            anchors.centerIn: parent
            width: Math.min(parent.width - Config.Theme.margin * 2, Config.Theme.brandBoxWidth)
            height: Config.Theme.brandBoxHeight
            color: "transparent"

            Rectangle {
                anchors.fill: parent
                color: Config.Theme.colorBox
                opacity: 0.92
                radius: 5
            }

            // PokerTH-Logo + Kartensymbole oben in der Card, fix positioniert –
            // exakt wie auf der StartPage (Config.Theme.margin vom oberen Rand).
            BrandHeader {
                id: loginLogo
                anchors.top: parent.top
                anchors.topMargin: Config.Theme.margin
                anchors.horizontalCenter: parent.horizontalCenter
                logoSize: Config.Theme.brandLogoSize
            }

            StackLayout {
                id: mainStack
                anchors.fill: parent
                anchors.margins: 28
                anchors.topMargin: loginLogo.y + loginLogo.height + 12   // Logo + Abstand
                currentIndex: 0

                // View 0: Auswahl
                ColumnLayout {
                    id: initialChoicesView
                    spacing: 18

                    Item { Layout.fillHeight: true }

                    CustomButton {
                        text: qsTr("Login as User")
                        Layout.fillWidth: true
                        onClicked: mainStack.currentIndex = 1
                    }

                    CustomButton {
                        text: qsTr("Register")
                        Layout.fillWidth: true
                        onClicked: ServerConnection.openExternalUrl(ServerConnection.registerUrl)
                    }

                    CustomButton {
                        text: qsTr("Continue as Guest")
                        Layout.fillWidth: true
                        onClicked: {
                            var guestName = "Guest" + Math.floor(Math.random() * 10000)
                            usernameLabel.text = guestName
                            connectionProgress.value = 0
                            mainStack.currentIndex = 2
                            ServerConnection.connectToServer(guestName, "", true, false)
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // View 1: Login-Formular
                ColumnLayout {
                    id: loginFormView
                    spacing: 12

                    Item { Layout.fillHeight: true }

                    AppLabel {
                        text: qsTr("User Login")
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        font.pixelSize: Config.Theme.fontSizeTitle
                        color: Config.StaticData.palette.secondary.col200
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Config.StaticData.palette.secondary.col500
                    }

                    TextField {
                        id: usernameInput
                        placeholderText: qsTr("Username")
                        Layout.fillWidth: true
                        Layout.preferredHeight: Config.Theme.touchTarget
                        font.family: Config.StaticData.loadedFont.font.family
                        color: Config.StaticData.palette.secondary.col200
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        background: Rectangle {
                            color: Config.StaticData.palette.secondary.col600
                            border.color: Config.StaticData.palette.secondary.col500
                            border.width: 1
                            radius: 3
                        }
                    }

                    TextField {
                        id: passwordInput
                        placeholderText: qsTr("Password")
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                        Layout.preferredHeight: Config.Theme.touchTarget
                        font.family: Config.StaticData.loadedFont.font.family
                        color: Config.StaticData.palette.secondary.col200
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        background: Rectangle {
                            color: Config.StaticData.palette.secondary.col600
                            border.color: Config.StaticData.palette.secondary.col500
                            border.width: 1
                            radius: 3
                        }
                    }

                    CheckBox {
                        id: rememberMeCheckbox
                        text: qsTr("Remember me")
                        checked: false
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Config.Theme.spacing

                        CustomButton {
                            text: qsTr("Back")
                            Layout.fillWidth: true
                            onClicked: mainStack.currentIndex = 0
                        }

                        CustomButton {
                            text: qsTr("Login")
                            Layout.fillWidth: true
                            onClicked: {
                                // console.log("Login clicked. Username:", usernameInput.text, "Remember me:", rememberMeCheckbox.checked)
                                usernameLabel.text = usernameInput.text
                                connectionProgress.value = 0
                                mainStack.currentIndex = 2
                                ServerConnection.connectToServer(usernameInput.text, passwordInput.text, false, rememberMeCheckbox.checked)
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // View 2: Verbindungsaufbau
                ColumnLayout {
                    id: guestLoginView
                    spacing: 20

                    Item { Layout.fillHeight: true }

                    AppText {
                        text: qsTr("Connecting as...")
                        font.pixelSize: Config.Theme.fontSizeBody
                        color: Config.StaticData.palette.secondary.col300
                        Layout.alignment: Qt.AlignHCenter
                    }

                    AppText {
                        id: usernameLabel
                        text: qsTr("Username/Guest")
                        font.pixelSize: Config.Theme.fontSizeTitle
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        Layout.alignment: Qt.AlignHCenter
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        ProgressBar {
                            id: connectionProgress
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            from: 0
                            to: 100
                            value: 0

                            background: Rectangle {
                                implicitWidth: 200
                                implicitHeight: 8
                                color: Config.StaticData.palette.secondary.col600
                                border.color: Config.StaticData.palette.secondary.col500
                                border.width: 1
                                radius: 4
                            }

                            contentItem: Item {
                                implicitWidth: 200
                                implicitHeight: 6

                                Rectangle {
                                    width: connectionProgress.visualPosition * parent.width
                                    height: parent.height
                                    radius: 4
                                    color: Config.StaticData.palette.secondary.col300
                                }
                            }
                        }

                        AppText {
                            id: statusText
                            text: qsTr("Initializing connection...")
                            font.pixelSize: Config.Theme.fontSizeBody
                            color: Config.StaticData.palette.secondary.col300
                            // Fehlermeldungen sind mehrzeilig (s. ServerConnectionHandler::
                            // networkErrorMessage) – ohne Umbruch lief der Text aus der Box.
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    Item { Layout.fillHeight: true }

                    CustomButton {
                        text: qsTr("Cancel")
                        Layout.fillWidth: true
                        onClicked: {
                            ServerConnection.cancelConnection()
                            connectionProgress.value = 0
                            mainStack.currentIndex = 0
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
        }
    }
}