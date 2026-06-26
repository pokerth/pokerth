import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// "Netzwerkspiel beitreten" – portiert aus dem Qt-Widgets joinNetworkGameDialog.
// Verbindet zu einem Netzwerk-Server (Adresse/Port/IPv6/SCTP) → tritt automatisch
// dem ersten Spiel bei → Warteraum. Inkl. gespeicherter Server-Profile.
Rectangle {
    id: networkGameEnterPage
    objectName: "networkGameEnterPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    property bool connecting: false
    property string statusMessage: ""
    property string selectedProfile: ""

    readonly property var profiles: (typeof NetworkGame !== "undefined" && NetworkGame)
        ? NetworkGame.serverProfiles : []

    Component.onCompleted: {
        if (typeof NetworkGame !== "undefined" && NetworkGame) {
            NetworkGame.refreshProfiles()
            portSpin.value = NetworkGame.defaultPort()
        }
    }

    Connections {
        target: (typeof ServerConnection !== "undefined") ? ServerConnection : null
        function onShowLobby() {
            mainStackView.replace(mainStackView.currentItem, "LobbyPage.qml")
        }
        function onConnectionFailed(errorMessage) {
            networkGameEnterPage.connecting = false
            networkGameEnterPage.statusMessage = errorMessage
        }
    }
    Connections {
        target: (typeof NetworkGame !== "undefined") ? NetworkGame : null
        function onJoinFailed(message) {
            networkGameEnterPage.connecting = false
            networkGameEnterPage.statusMessage = message
        }
    }

    function startConnect() {
        if (typeof NetworkGame === "undefined" || !NetworkGame) return
        networkGameEnterPage.statusMessage = ""
        networkGameEnterPage.connecting = true
        NetworkGame.joinGame(addressField.text, portSpin.value,
                             ipv6Switch.checked, sctpSwitch.checked)
    }

    function fillFromProfile(p) {
        addressField.text     = p.address || ""
        portSpin.value        = p.port || NetworkGame.defaultPort()
        ipv6Switch.checked    = p.ipv6 === true
        sctpSwitch.checked    = p.sctp === true
        profileNameField.text = p.name || ""
        networkGameEnterPage.selectedProfile = p.name || ""
    }

    // ── Formular ──────────────────────────────────────────────────────────────
    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        visible: !networkGameEnterPage.connecting
        // AlwaysOff statt transientem Default: sonst blitzt die Scrollbar beim
        // Seitenaufbau sekundenlang auf, obwohl nichts zu scrollen ist.
        ScrollBar.vertical.policy: scrollView.contentHeight > scrollView.height + 1
                                   ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 0

            // Header
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 56
                color: Config.StaticData.palette.secondary.col600
                RowLayout {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                    spacing: 10
                    CustomButton {
                        text: qsTr("← Zurück")
                        implicitWidth: 90
                        implicitHeight: 36
                        onClicked: mainStackView.pop()
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Netzwerkspiel beitreten")
                        color: Config.StaticData.palette.secondary.col100
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 18
                        font.bold: true
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Config.StaticData.palette.secondary.col500
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                Layout.bottomMargin: 8
                spacing: 12

                // ── Server-Verbindung ────────────────────────────────────────
                Label {
                    text: qsTr("Server-Verbindung")
                    color: Config.StaticData.palette.secondary.col300
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 13
                    font.bold: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Label {
                        text: qsTr("Server-Adresse")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                    }
                    TextField {
                        id: addressField
                        Layout.fillWidth: true
                        placeholderText: qsTr("IP-Adresse oder Hostname")
                        font.family: Config.StaticData.loadedFont.font.family
                        color: Config.StaticData.palette.secondary.col100
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        background: Rectangle {
                            radius: 6
                            color: Config.StaticData.palette.secondary.col600
                            border.color: addressField.activeFocus
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            border.width: 1
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Port")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    CustomSpinBox {
                        id: portSpin
                        from: 1
                        to: 65535
                        value: 7234
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("IPv6 verwenden")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    Switch { id: ipv6Switch; checked: false }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("SCTP verwenden")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    Switch { id: sctpSwitch; checked: false }
                }

                // ── Server-Profile ───────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                }
                Label {
                    text: qsTr("Server-Profile")
                    color: Config.StaticData.palette.secondary.col300
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 13
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label {
                        text: qsTr("Profilname")
                        color: Config.StaticData.palette.secondary.col200
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 12
                        Layout.preferredWidth: 150
                        verticalAlignment: Text.AlignVCenter
                    }
                    TextField {
                        id: profileNameField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Name des Profils")
                        font.family: Config.StaticData.loadedFont.font.family
                        color: Config.StaticData.palette.secondary.col100
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        // Profilname = XML-Tag-Name in serverprofiles.xml: muss mit
                        // einem Buchstaben beginnen, danach Buchstaben/Ziffern (wie im
                        // Qt-Widgets-Client), damit beide Clients dieselbe Datei teilen.
                        validator: RegularExpressionValidator {
                            regularExpression: /[A-Za-z][A-Za-z0-9]*/
                        }
                        background: Rectangle {
                            radius: 6
                            color: Config.StaticData.palette.secondary.col600
                            border.color: profileNameField.activeFocus
                                ? Config.StaticData.palette.secondary.col200
                                : Config.StaticData.palette.secondary.col400
                            border.width: 1
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    CustomButton {
                        text: qsTr("Speichern")
                        Layout.fillWidth: true
                        enabled: profileNameField.text.trim().length > 0
                                 && addressField.text.trim().length > 0
                        onClicked: {
                            NetworkGame.saveProfile(profileNameField.text.trim(),
                                                    addressField.text.trim(),
                                                    portSpin.value,
                                                    ipv6Switch.checked,
                                                    sctpSwitch.checked)
                            networkGameEnterPage.selectedProfile = profileNameField.text.trim()
                        }
                    }
                    CustomButton {
                        text: qsTr("Löschen")
                        Layout.fillWidth: true
                        enabled: networkGameEnterPage.selectedProfile !== ""
                        onClicked: {
                            NetworkGame.deleteProfile(networkGameEnterPage.selectedProfile)
                            networkGameEnterPage.selectedProfile = ""
                        }
                    }
                }

                // Profil-Liste
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(profileList.contentHeight + 2, 180)
                    visible: networkGameEnterPage.profiles.length > 0
                    color: Config.StaticData.palette.secondary.col600
                    border.color: Config.StaticData.palette.secondary.col500
                    border.width: 1
                    radius: 4

                    ListView {
                        id: profileList
                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true
                        model: networkGameEnterPage.profiles
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {
                            policy: profileList.contentHeight > profileList.height + 4
                                    ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                        }

                        delegate: ItemDelegate {
                            id: profileDelegate
                            required property int index
                            required property var modelData
                            width: ListView.view.width
                            height: 36

                            background: Rectangle {
                                color: networkGameEnterPage.selectedProfile === profileDelegate.modelData.name
                                       ? Config.Theme.colorAccent
                                       : (profileDelegate.index % 2 === 0
                                          ? Config.StaticData.palette.secondary.col700
                                          : Config.StaticData.palette.secondary.col600)
                            }
                            contentItem: RowLayout {
                                spacing: 8
                                Text {
                                    Layout.fillWidth: true
                                    text: profileDelegate.modelData.name
                                    elide: Text.ElideRight
                                    color: networkGameEnterPage.selectedProfile === profileDelegate.modelData.name
                                           ? "#101010" : Config.StaticData.palette.secondary.col100
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: 12
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                }
                                Text {
                                    text: (profileDelegate.modelData.address || "") + ":" + (profileDelegate.modelData.port || "")
                                    color: networkGameEnterPage.selectedProfile === profileDelegate.modelData.name
                                           ? "#202020" : Config.StaticData.palette.secondary.col300
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                            onClicked: networkGameEnterPage.fillFromProfile(profileDelegate.modelData)
                        }
                    }
                }

                Label {
                    visible: networkGameEnterPage.statusMessage !== ""
                    text: networkGameEnterPage.statusMessage
                    color: "#ef4444"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 12
                }

                // ── Aktionen ─────────────────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Config.StaticData.palette.secondary.col500
                    Layout.topMargin: 8
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 10
                    CustomButton {
                        text: qsTr("Abbrechen")
                        Layout.fillWidth: true
                        onClicked: mainStackView.pop()
                    }
                    CustomButton {
                        text: qsTr("Verbinden")
                        Layout.fillWidth: true
                        enabled: addressField.text.trim().length > 0
                        onClicked: networkGameEnterPage.startConnect()
                    }
                }
            }
        }
    }

    // ── Verbindungsaufbau-Ansicht ─────────────────────────────────────────────
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.8, 360)
        spacing: 20
        visible: networkGameEnterPage.connecting

        BusyIndicator {
            running: networkGameEnterPage.connecting
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 48
            implicitHeight: 48
        }
        Label {
            text: qsTr("Verbinde mit Server …")
            Layout.alignment: Qt.AlignHCenter
            color: Config.StaticData.palette.secondary.col200
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: Config.Theme.fontSizeTitle
            font.bold: true
        }
        CustomButton {
            text: qsTr("Abbrechen")
            Layout.fillWidth: true
            onClicked: {
                if (typeof ServerConnection !== "undefined" && ServerConnection)
                    ServerConnection.cancelConnection()
                networkGameEnterPage.connecting = false
            }
        }
    }
}
