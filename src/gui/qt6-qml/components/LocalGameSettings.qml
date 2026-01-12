import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: localGameSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: localGameSettingsContent
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            horizontalAlignment: Text.AlignLeft
            text: qsTr("Lokales Spiel")
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
                            text: qsTr("Anzahl der Spieler:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SpinBox {
                            id: numberOfPlayers
                            from: 2
                            to: 10
                            value: SettingsManager ? SettingsManager.readConfigInt("NumberOfPlayers") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NumberOfPlayers", value)
                            }
                        }

                        Label {
                            text: qsTr("Startkapital:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SpinBox {
                            id: startCash
                            from: 100
                            to: 1000000
                            stepSize: 100
                            value: SettingsManager ? SettingsManager.readConfigInt("StartCash") : 2000
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("StartCash", value)
                            }
                        }

                        Label {
                            text: qsTr("Erster Small Blind:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SpinBox {
                            id: firstSmallBlind
                            from: 5
                            to: 10000
                            stepSize: 5
                            value: SettingsManager ? SettingsManager.readConfigInt("FirstSmallBlind") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("FirstSmallBlind", value)
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
                            id: raiseBlindsGroup
                        }

                        RadioButton {
                            id: raiseBlindsAtHands
                            text: qsTr("Blinds bei Anzahl der Hände erhöhen")
                            checked: SettingsManager ? SettingsManager.readConfigInt("RaiseBlindsAtHands") !== 0 : true
                            ButtonGroup.group: raiseBlindsGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("RaiseBlindsAtHands", 1)
                                    SettingsManager.writeConfigInt("RaiseBlindsAtMinutes", 0)
                                }
                            }
                        }

                        RowLayout {
                            Layout.leftMargin: 30

                            Label {
                                text: qsTr("Small Blind erhöhen alle:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: raiseBlindsAtHands.checked
                            }

                            SpinBox {
                                id: raiseSmallBlindEveryHands
                                from: 1
                                to: 100
                                value: SettingsManager ? SettingsManager.readConfigInt("RaiseSmallBlindEveryHands") : 5
                                enabled: raiseBlindsAtHands.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("RaiseSmallBlindEveryHands", value)
                                }
                            }

                            Label {
                                text: qsTr("Hände")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: raiseBlindsAtHands.checked
                            }
                        }

                        RadioButton {
                            id: raiseBlindsAtMinutes
                            text: qsTr("Blinds zeitbasiert erhöhen")
                            checked: SettingsManager ? SettingsManager.readConfigInt("RaiseBlindsAtMinutes") !== 0 : false
                            ButtonGroup.group: raiseBlindsGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("RaiseBlindsAtHands", 0)
                                    SettingsManager.writeConfigInt("RaiseBlindsAtMinutes", 1)
                                }
                            }
                        }

                        RowLayout {
                            Layout.leftMargin: 30

                            Label {
                                text: qsTr("Small Blind erhöhen alle:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: raiseBlindsAtMinutes.checked
                            }

                            SpinBox {
                                id: raiseSmallBlindEveryMinutes
                                from: 1
                                to: 60
                                value: SettingsManager ? SettingsManager.readConfigInt("RaiseSmallBlindEveryMinutes") : 5
                                enabled: raiseBlindsAtMinutes.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("RaiseSmallBlindEveryMinutes", value)
                                }
                            }

                            Label {
                                text: qsTr("Minuten")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: raiseBlindsAtMinutes.checked
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
                            id: blindsOrderGroup
                        }

                        RadioButton {
                            id: alwaysDoubleBlinds
                            text: qsTr("Blinds immer verdoppeln")
                            checked: SettingsManager ? SettingsManager.readConfigInt("AlwaysDoubleBlinds") !== 0 : true
                            ButtonGroup.group: blindsOrderGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("AlwaysDoubleBlinds", 1)
                                    SettingsManager.writeConfigInt("ManualBlindsOrder", 0)
                                }
                            }
                        }

                        RadioButton {
                            id: manualBlindsOrder
                            text: qsTr("Manuelle Blind-Reihenfolge")
                            checked: SettingsManager ? SettingsManager.readConfigInt("ManualBlindsOrder") !== 0 : false
                            ButtonGroup.group: blindsOrderGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("AlwaysDoubleBlinds", 0)
                                    SettingsManager.writeConfigInt("ManualBlindsOrder", 1)
                                }
                            }
                        }

                        Button {
                            Layout.leftMargin: 30
                            text: qsTr("Manuelle Blind-Reihenfolge bearbeiten...")
                            enabled: manualBlindsOrder.checked
                            onClicked: {
                                // TODO: Dialog für manuelle Blind-Reihenfolge öffnen
                            }
                        }
                    }
                }

                // Spielgeschwindigkeit
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Spielgeschwindigkeit")

                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        columnSpacing: 12
                        rowSpacing: 8

                        Label {
                            text: qsTr("Spielgeschwindigkeit (1=langsam, 11=schnell):")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SpinBox {
                            id: gameSpeed
                            from: 1
                            to: 11
                            value: SettingsManager ? SettingsManager.readConfigInt("GameSpeed") : 6
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("GameSpeed", value)
                            }
                        }

                        CheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Pause zwischen den Händen")
                            checked: SettingsManager ? SettingsManager.readConfigInt("PauseBetweenHands") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("PauseBetweenHands", checked ? 1 : 0)
                            }
                        }

                        CheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Spiel-Einstellungsdialog bei neuem Spiel anzeigen")
                            checked: SettingsManager ? SettingsManager.readConfigInt("ShowGameSettingsDialogOnNewGame") !== 0 : true
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("ShowGameSettingsDialogOnNewGame", checked ? 1 : 0)
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
