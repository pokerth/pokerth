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

        Label {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            horizontalAlignment: Text.AlignLeft
            text: qsTr("Internetspiel")
            font.bold: true
            font.pointSize: 12
            color: Config.StaticData.palette.secondary.col200
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.fillHeight: false
            Layout.topMargin: 0
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.alignment: Qt.AlignTop
            color: Config.StaticData.palette.secondary.col500
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 12

                // Server-Konfigurationsmodus
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

                            SpinBox {
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
                            model: [qsTr("Normal"), qsTr("Registrierte Spieler"), qsTr("Rang-Spiel")]
                            currentIndex: SettingsManager ? SettingsManager.readConfigInt("InternetGameType") : 0
                            onActivated: {
                                if (SettingsManager) SettingsManager.writeConfigInt("InternetGameType", currentIndex)
                            }
                        }

                        CheckBox {
                            id: useInternetGamePassword
                            Layout.columnSpan: 2
                            text: qsTr("Spiel-Passwort verwenden")
                            checked: SettingsManager ? SettingsManager.readConfigInt("UseInternetGamePassword") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("UseInternetGamePassword", checked ? 1 : 0)
                            }
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

                        CheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Zuschauer erlauben")
                            checked: SettingsManager ? SettingsManager.readConfigInt("InternetGameAllowSpectators") !== 0 : true
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("InternetGameAllowSpectators", checked ? 1 : 0)
                            }
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

                        CheckBox {
                            id: useAvatarServer
                            text: qsTr("Avatar-Server verwenden")
                            checked: SettingsManager ? SettingsManager.readConfigInt("UseAvatarServer") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("UseAvatarServer", checked ? 1 : 0)
                            }
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

                        CheckBox {
                            text: qsTr("TLS/SSL verwenden (verschlüsselte Verbindung)")
                            checked: SettingsManager ? SettingsManager.readConfigInt("InternetServerUseTls") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("InternetServerUseTls", checked ? 1 : 0)
                            }
                        }

                        CheckBox {
                            text: qsTr("IPv6 verwenden")
                            checked: SettingsManager ? SettingsManager.readConfigInt("InternetServerUseIpv6") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("InternetServerUseIpv6", checked ? 1 : 0)
                            }
                        }

                        CheckBox {
                            text: qsTr("SCTP verwenden (statt TCP)")
                            checked: SettingsManager ? SettingsManager.readConfigInt("InternetServerUseSctp") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("InternetServerUseSctp", checked ? 1 : 0)
                            }
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

                        CheckBox {
                            text: qsTr("Lobby-Chat verwenden")
                            checked: SettingsManager ? SettingsManager.readConfigInt("UseLobbyChat") !== 0 : true
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("UseLobbyChat", checked ? 1 : 0)
                            }
                        }

                        CheckBox {
                            text: qsTr("Tisch automatisch verlassen nach Spielende")
                            checked: SettingsManager ? SettingsManager.readConfigInt("NetAutoLeaveGameAfterFinish") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetAutoLeaveGameAfterFinish", checked ? 1 : 0)
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
