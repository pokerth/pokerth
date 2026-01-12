import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: logsSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: logsettingsContent
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            horizontalAlignment: Text.AlignLeft
            text: qsTr("Log-Nachrichten")
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

                // Log-Aktivierung
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Log-Einstellungen")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        CheckBox {
                            id: logOnOff
                            text: qsTr("Logging aktivieren")
                            checked: SettingsManager ? SettingsManager.readConfigInt("LogOnOff") !== 0 : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.writeConfigInt("LogOnOff", checked ? 1 : 0)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: qsTr("Log-Verzeichnis:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: logOnOff.checked
                            }

                            TextField {
                                id: logDirectory
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("LogDir") : ""
                                enabled: logOnOff.checked
                                readOnly: true
                            }

                            Button {
                                text: qsTr("Durchsuchen...")
                                enabled: logOnOff.checked
                                onClicked: {
                                    // TODO: Verzeichnis-Auswahl-Dialog
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8

                            Label {
                                text: qsTr("Log-Speicherdauer (Tage):")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: logOnOff.checked
                            }

                            SpinBox {
                                id: logStoreDuration
                                from: 1
                                to: 365
                                value: SettingsManager ? SettingsManager.readConfigInt("LogStoreDuration") : 30
                                enabled: logOnOff.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("LogStoreDuration", value)
                                }
                            }

                            Label {
                                text: qsTr("Log-Intervall:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: logOnOff.checked
                            }

                            ComboBox {
                                id: logInterval
                                Layout.fillWidth: true
                                model: [
                                    qsTr("Jede Hand"),
                                    qsTr("Jedes Spiel"),
                                    qsTr("Jeden Tag"),
                                    qsTr("Jede Woche"),
                                    qsTr("Jeden Monat")
                                ]
                                currentIndex: SettingsManager ? SettingsManager.readConfigInt("LogInterval") : 1
                                enabled: logOnOff.checked
                                onActivated: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("LogInterval", currentIndex)
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
