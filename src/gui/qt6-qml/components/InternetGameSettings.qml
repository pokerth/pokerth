import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: internetGameSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: internetGameSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Internetspiel"); topGap: 4 }

        ScrollView {
            id: inetScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width - 12
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Server-Konfiguration")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ButtonGroup {
                            id: serverConfigGroup
                        }

                        RadioButton {
                            id: automaticServerConfig
                            text: qsTr("Automatische Server-Konfiguration")
                            checked: SettingsManager ? SettingsManager.readConfigInt("InternetServerConfigMode") === 0 : true
                            ButtonGroup.group: serverConfigGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("InternetServerConfigMode", 0)
                                }
                            }
                        }

                        RowLayout {
                            Layout.leftMargin: 30
                            Layout.fillWidth: true

                            Label {
                                text: qsTr("Server-Liste:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: automaticServerConfig.checked
                            }

                            TextField {
                                id: serverListAddress
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("InternetServerListAddress") : ""
                                enabled: automaticServerConfig.checked
                                onEditingFinished: {
                                    if (SettingsManager) SettingsManager.writeConfigString("InternetServerListAddress", text)
                                }
                            }
                        }

                        RadioButton {
                            id: manualServerConfig
                            text: qsTr("Manuelle Server-Konfiguration")
                            checked: SettingsManager ? SettingsManager.readConfigInt("InternetServerConfigMode") === 1 : false
                            ButtonGroup.group: serverConfigGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("InternetServerConfigMode", 1)
                                }
                            }
                        }

                        GridLayout {
                            Layout.leftMargin: 30
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8

                            Label {
                                text: qsTr("Server-Adresse:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: manualServerConfig.checked
                            }

                            TextField {
                                id: internetServerAddress
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("InternetServerAddress") : ""
                                enabled: manualServerConfig.checked
                                onEditingFinished: {
                                    if (SettingsManager) SettingsManager.writeConfigString("InternetServerAddress", text)
                                }
                            }

                            Label {
                                text: qsTr("Server-Port:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: manualServerConfig.checked
                            }

                            CustomSpinBox {
                                id: internetServerPort
                                from: 1024
                                to: 65535
                                value: SettingsManager ? SettingsManager.readConfigInt("InternetServerPort") : 7234
                                enabled: manualServerConfig.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("InternetServerPort", value)
                                }
                            }

                            Label {
                                text: qsTr("Server-Passwort:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: manualServerConfig.checked
                            }

                            TextField {
                                id: serverPassword
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("ServerPassword") : ""
                                echoMode: TextInput.Password
                                enabled: manualServerConfig.checked
                                onEditingFinished: {
                                    if (SettingsManager) SettingsManager.writeConfigString("ServerPassword", text)
                                }
                            }
                        }
                    }
                }

                // Spiel-Einstellungen
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Spiel-Einstellungen")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            text: qsTr("Spiel-Name:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        TextField {
                            id: internetGameName
                            Layout.fillWidth: true
                            text: SettingsManager ? SettingsManager.readConfigString("InternetGameName") : ""
                            onEditingFinished: {
                                if (SettingsManager) SettingsManager.writeConfigString("InternetGameName", text)
                            }
                        }

                        Label {
                            text: qsTr("Spiel-Typ:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        ComboBox {
                            id: internetGameType
                            Layout.fillWidth: true
                            // Reihenfolge und Index entsprechen GameType-1
                            // (GAME_TYPE_NORMAL … GAME_TYPE_RANKING).
                            model: [qsTr("Normal"),
                                    qsTr("Nur registrierte Spieler"),
                                    qsTr("Nur eingeladene Spieler"),
                                    qsTr("Ranglistenspiel")]
                            currentIndex: {
                                if (!SettingsManager)
                                    return 0
                                var type = SettingsManager.readConfigInt("InternetGameType")
                                return (type >= 0 && type < model.length) ? type : 0
                            }
                            onActivated: {
                                if (SettingsManager) SettingsManager.writeConfigInt("InternetGameType", currentIndex)
                            }
                        }

                        ConfigCheckBox {
                            id: useInternetGamePassword
                            Layout.columnSpan: 2
                            text: qsTr("Spiel-Passwort verwenden")
                            configKey: "UseInternetGamePassword"
                            defaultChecked: false
                        }

                        Label {
                            text: qsTr("Spiel-Passwort:")
                            color: Config.StaticData.palette.secondary.col200
                            enabled: useInternetGamePassword.checked
                        }

                        TextField {
                            id: internetGamePassword
                            Layout.fillWidth: true
                            text: SettingsManager ? SettingsManager.readConfigString("InternetGamePassword") : ""
                            echoMode: TextInput.Password
                            enabled: useInternetGamePassword.checked
                            onEditingFinished: {
                                if (SettingsManager) SettingsManager.writeConfigString("InternetGamePassword", text)
                            }
                        }

                        ConfigCheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Zuschauer erlauben")
                            configKey: "InternetGameAllowSpectators"
                        }
                    }
                }

                // Avatar-Server
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Avatar-Server")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ConfigCheckBox {
                            id: useAvatarServer
                            text: qsTr("Avatar-Server verwenden")
                            configKey: "UseAvatarServer"
                            defaultChecked: false
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 30

                            Label {
                                text: qsTr("Avatar-Server-Adresse:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: useAvatarServer.checked
                            }

                            TextField {
                                id: avatarServerAddress
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("AvatarServerAddress") : ""
                                enabled: useAvatarServer.checked
                                onEditingFinished: {
                                    if (SettingsManager) SettingsManager.writeConfigString("AvatarServerAddress", text)
                                }
                            }
                        }
                    }
                }

                // Verbindungsoptionen
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Verbindungsoptionen")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ConfigCheckBox {
                            text: qsTr("TLS/SSL verwenden (verschlüsselte Verbindung)")
                            configKey: "InternetServerUseTls"
                            defaultChecked: false
                        }

                        ConfigCheckBox {
                            text: qsTr("IPv6 verwenden")
                            configKey: "InternetServerUseIpv6"
                            defaultChecked: false
                        }

                        ConfigCheckBox {
                            text: qsTr("SCTP verwenden (statt TCP)")
                            configKey: "InternetServerUseSctp"
                            defaultChecked: false
                        }
                    }
                }

                // Weitere Optionen
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Weitere Optionen")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ConfigCheckBox {
                            text: qsTr("Tisch automatisch verlassen nach Spielende")
                            configKey: "NetAutoLeaveGameAfterFinish"
                            defaultChecked: false
                        }
                    }
                }

                // Ignorierte Spieler
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Ignorierte Spieler")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            text: qsTr("Spieler auf der Ignore-Liste werden nicht im Chat angezeigt.")
                            color: Config.StaticData.palette.secondary.col400
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            font.pixelSize: Config.Theme.fontSizeBody
                        }

                        ListView {
                            id: ignoreListView
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(contentHeight, 160)
                            clip: true
                            model: SettingsManager ? SettingsManager.readConfigStringList("PlayerIgnoreList") : []

                            delegate: RowLayout {
                                width: ignoreListView.width
                                height: Config.Theme.touchTarget * 0.8

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData
                                    color: Config.StaticData.palette.secondary.col200
                                    verticalAlignment: Text.AlignVCenter
                                }

                                CustomButton {
                                    text: qsTr("Entfernen")
                                    Layout.preferredWidth: 100
                                    onClicked: {
                                        if (!SettingsManager) return
                                        var list = SettingsManager.readConfigStringList("PlayerIgnoreList")
                                        list.splice(index, 1)
                                        SettingsManager.writeConfigStringList("PlayerIgnoreList", list)
                                        ignoreListView.model = SettingsManager.readConfigStringList("PlayerIgnoreList")
                                    }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                visible: ignoreListView.count === 0
                                color: "transparent"

                                Label {
                                    anchors.centerIn: parent
                                    text: qsTr("(keine ignorierten Spieler)")
                                    color: Config.StaticData.palette.secondary.col500
                                    font.italic: true
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}
