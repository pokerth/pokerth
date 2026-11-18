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

        SettingsHeader { title: qsTr("Network Game"); topGap: 4 }

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

                // Players & starting money
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Players & Starting Capital")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("Number of Players:")
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
                            wrapMode: Text.WordWrap
                            text: qsTr("Starting Capital:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netStartCash
                            from: 1000
                            to: 1000000
                            stepSize: 50
                            value: SettingsManager ? SettingsManager.readConfigInt("NetStartCash") : 3000
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetStartCash", value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("First Small Blind:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netFirstSmallBlind
                            from: 5
                            to: 20000
                            stepSize: 5
                            value: SettingsManager ? SettingsManager.readConfigInt("NetFirstSmallBlind") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetFirstSmallBlind", value)
                            }
                        }
                    }
                }

                // Raise blinds
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Increase Blinds")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ButtonGroup {
                            id: netRaiseBlindsGroup
                        }

                        RadioButton {
                            id: netRaiseBlindsAtHands
                            text: qsTr("Increase Blinds after Number of Hands")
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
                                wrapMode: Text.WordWrap
                                text: qsTr("Increase Small Blind every:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtHands.checked
                            }

                            CustomSpinBox {
                                id: netRaiseSmallBlindEveryHands
                                from: 1
                                to: 999
                                value: SettingsManager ? SettingsManager.readConfigInt("NetRaiseSmallBlindEveryHands") : 8
                                enabled: netRaiseBlindsAtHands.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("NetRaiseSmallBlindEveryHands", value)
                                }
                            }

                            Label {
                                text: qsTr("Hands")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtHands.checked
                            }
                        }

                        RadioButton {
                            id: netRaiseBlindsAtMinutes
                            text: qsTr("Increase Blinds Time-Based")
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
                                wrapMode: Text.WordWrap
                                text: qsTr("Increase Small Blind every:")
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
                                text: qsTr("Minutes")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: netRaiseBlindsAtMinutes.checked
                            }
                        }
                    }
                }

                // Blind raise order
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Blind Raise Order")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ButtonGroup {
                            id: netBlindsOrderGroup
                        }

                        RadioButton {
                            id: netAlwaysDoubleBlinds
                            text: qsTr("Always Double Blinds")
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
                            text: qsTr("Manual Blind Order")
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
                            text: qsTr("Edit Manual Blind Order...")
                            enabled: netManualBlindsOrder.checked
                            onClicked: {
                                // TODO: open dialog for the manual blind order
                            }
                        }
                    }
                }

                // Timing
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Time Settings")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("Delay Between Hands\n(Seconds):")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netDelayBetweenHands
                            from: 5
                            to: 20
                            value: SettingsManager ? SettingsManager.readConfigInt("NetDelayBetweenHands") : 7
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NetDelayBetweenHands", value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("Player Action Timeout\n(Seconds):")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: netTimeOutPlayerAction
                            from: 5
                            to: 60
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
                    title: qsTr("Server Settings")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: qsTr("Server Port:")
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
                            text: qsTr("Use IPv6")
                            configKey: "ServerUseIpv6"
                            defaultChecked: false
                        }

                        ConfigCheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Use SCTP")
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
