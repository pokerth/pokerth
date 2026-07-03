import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: networkGameSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: networkGameSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Netzwerkspiel"); topGap: 4 }

        ScrollView {
            id: netScrollView
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

                // Spieler & Startkapital
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Spieler & Startkapital")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Anzahl der Spieler:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netNumberOfPlayers
                            from: 2
                            to: 10
                            value: SettingsManager ? SettingsManager.readConfigInt("NetNumberOfPlayers") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetNumberOfPlayers", value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Startkapital:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netStartCash
                            from: 100
                            to: 1000000
                            stepSize: 100
                            value: SettingsManager ? SettingsManager.readConfigInt("NetStartCash") : 2000
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetStartCash", value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Erster Small Blind:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netFirstSmallBlind
                            from: 5
                            to: 10000
                            stepSize: 5
                            value: SettingsManager ? SettingsManager.readConfigInt("NetFirstSmallBlind") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetFirstSmallBlind", value)
                            }
                        }
                    }
                }

                // Blinds erhöhen
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Blinds erhöhen")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ButtonGroup {
                            id: netRaiseBlindsGroup
                        }

                        RadioButton {
                            id: netRaiseBlindsAtHands
                            text: qsTr("Blinds bei Anzahl der Hände erhöhen")
                            checked: SettingsManager ? SettingsManager.readConfigInt("NetRaiseBlindsAtHands") !== 0 : true
                            ButtonGroup.group: netRaiseBlindsGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("NetRaiseBlindsAtHands", 1)
                                    SettingsManager.writeConfigInt("NetRaiseBlindsAtMinutes", 0)
                                }
                            }
                        }

                        RowLayout {
                            Layout.leftMargin: 30

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Small Blind erhöhen alle:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtHands.checked
                            }

                            CustomSpinBox {
                                id: netRaiseSmallBlindEveryHands
                                from: 1
                                to: 100
                                value: SettingsManager ? SettingsManager.readConfigInt("NetRaiseSmallBlindEveryHands") : 5
                                enabled: netRaiseBlindsAtHands.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("NetRaiseSmallBlindEveryHands", value)
                                }
                            }

                            Label {
                                text: qsTr("Hände")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtHands.checked
                            }
                        }

                        RadioButton {
                            id: netRaiseBlindsAtMinutes
                            text: qsTr("Blinds zeitbasiert erhöhen")
                            checked: SettingsManager ? SettingsManager.readConfigInt("NetRaiseBlindsAtMinutes") !== 0 : false
                            ButtonGroup.group: netRaiseBlindsGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("NetRaiseBlindsAtHands", 0)
                                    SettingsManager.writeConfigInt("NetRaiseBlindsAtMinutes", 1)
                                }
                            }
                        }

                        RowLayout {
                            Layout.leftMargin: 30

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Small Blind erhöhen alle:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtMinutes.checked
                            }

                            CustomSpinBox {
                                id: netRaiseSmallBlindEveryMinutes
                                from: 1
                                to: 60
                                value: SettingsManager ? SettingsManager.readConfigInt("NetRaiseSmallBlindEveryMinutes") : 5
                                enabled: netRaiseBlindsAtMinutes.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("NetRaiseSmallBlindEveryMinutes", value)
                                }
                            }

                            Label {
                                text: qsTr("Minuten")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtMinutes.checked
                            }
                        }
                    }
                }

                // Blind-Erhöhungsreihenfolge
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Blind-Erhöhungsreihenfolge")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ButtonGroup {
                            id: netBlindsOrderGroup
                        }

                        RadioButton {
                            id: netAlwaysDoubleBlinds
                            text: qsTr("Blinds immer verdoppeln")
                            checked: SettingsManager ? SettingsManager.readConfigInt("NetAlwaysDoubleBlinds") !== 0 : true
                            ButtonGroup.group: netBlindsOrderGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("NetAlwaysDoubleBlinds", 1)
                                    SettingsManager.writeConfigInt("NetManualBlindsOrder", 0)
                                }
                            }
                        }

                        RadioButton {
                            id: netManualBlindsOrder
                            text: qsTr("Manuelle Blind-Reihenfolge")
                            checked: SettingsManager ? SettingsManager.readConfigInt("NetManualBlindsOrder") !== 0 : false
                            ButtonGroup.group: netBlindsOrderGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("NetAlwaysDoubleBlinds", 0)
                                    SettingsManager.writeConfigInt("NetManualBlindsOrder", 1)
                                }
                            }
                        }

                        Button {
                            Layout.leftMargin: 30
                            text: qsTr("Manuelle Blind-Reihenfolge bearbeiten...")
                            enabled: netManualBlindsOrder.checked
                            onClicked: {
                                // TODO: Dialog für manuelle Blind-Reihenfolge öffnen
                            }
                        }
                    }
                }

                // Timing
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Zeiteinstellungen")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Verzögerung zwischen Händen\n(Sekunden):")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netDelayBetweenHands
                            from: 0
                            to: 30
                            value: SettingsManager ? SettingsManager.readConfigInt("NetDelayBetweenHands") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetDelayBetweenHands", value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Timeout für Spieleraktion\n(Sekunden):")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netTimeOutPlayerAction
                            from: 10
                            to: 120
                            value: SettingsManager ? SettingsManager.readConfigInt("NetTimeOutPlayerAction") : 20
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetTimeOutPlayerAction", value)
                            }
                        }
                    }
                }

                // Server-Einstellungen
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Server-Einstellungen")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Server-Port:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: serverPort
                            from: 1024
                            to: 65535
                            value: SettingsManager ? SettingsManager.readConfigInt("ServerPort") : 7234
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("ServerPort", value)
                            }
                        }

                        ConfigCheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("IPv6 verwenden")
                            configKey: "ServerUseIpv6"
                            defaultChecked: false
                        }

                        ConfigCheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("SCTP verwenden")
                            configKey: "ServerUseSctp"
                            defaultChecked: false
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
