import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: styleSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: styleSettingsContent
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 8
            Layout.bottomMargin: 0
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.fillHeight: false
            horizontalAlignment: Text.AlignLeft
            text: qsTr("Stil")
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

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12

            CustomTabBar {
                id: guiSettingsTabBar
                model: [qsTr("Spieltisch"), qsTr("Kartenstapel"), qsTr("Kartenrückseite")]
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: guiSettingsTabBar.currentIndex

                // Tab: Spieltisch
                ScrollView {
                    id: gameTableTab
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 8

                        Label {
                            text: qsTr("Spieltisch-Stil auswählen")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        Button {
                            text: qsTr("Stil hinzufügen...")
                            onClicked: {
                                // TODO: Datei-Auswahl-Dialog für Spieltisch-Stil
                            }
                        }

                        Button {
                            text: qsTr("Stil entfernen")
                            onClicked: {
                                // TODO: Ausgewählten Stil entfernen
                            }
                        }

                        Label {
                            Layout.topMargin: 8
                            text: qsTr("Hinweis: Die Stil-Auswahl mit Vorschau wird später implementiert")
                            color: Config.StaticData.palette.secondary.col400
                            font.italic: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // Tab: Kartenstapel
                ScrollView {
                    id: cardsDeckTab
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 8

                        Label {
                            text: qsTr("Kartenstapel-Stil auswählen")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        Button {
                            text: qsTr("Stil hinzufügen...")
                            onClicked: {
                                // TODO: Datei-Auswahl-Dialog für Kartenstapel-Stil
                            }
                        }

                        Button {
                            text: qsTr("Stil entfernen")
                            onClicked: {
                                // TODO: Ausgewählten Stil entfernen
                            }
                        }

                        Label {
                            Layout.topMargin: 8
                            text: qsTr("Hinweis: Die Stil-Auswahl mit Vorschau wird später implementiert")
                            color: Config.StaticData.palette.secondary.col400
                            font.italic: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // Tab: Kartenrückseite
                ScrollView {
                    id: cardsBackgroundTab
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 12

                        Label {
                            text: qsTr("Kartenrückseite auswählen")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        ButtonGroup {
                            id: flipsideGroup
                        }

                        RadioButton {
                            id: flipsideTux
                            text: qsTr("Standard (Tux)")
                            checked: SettingsManager ? SettingsManager.readConfigInt("FlipsideTux") !== 0 : true
                            ButtonGroup.group: flipsideGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("FlipsideTux", 1)
                                    SettingsManager.writeConfigInt("FlipsideOwn", 0)
                                }
                            }
                        }

                        RadioButton {
                            id: flipsideOwn
                            text: qsTr("Eigene Kartenrückseite")
                            checked: SettingsManager ? SettingsManager.readConfigInt("FlipsideOwn") !== 0 : false
                            ButtonGroup.group: flipsideGroup
                            onCheckedChanged: {
                                if (SettingsManager && checked) {
                                    SettingsManager.writeConfigInt("FlipsideTux", 0)
                                    SettingsManager.writeConfigInt("FlipsideOwn", 1)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 30

                            TextField {
                                id: ownFlipsideFilename
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("FlipsideOwnFile") : ""
                                enabled: flipsideOwn.checked
                                readOnly: true
                            }

                            Button {
                                text: qsTr("Durchsuchen...")
                                enabled: flipsideOwn.checked
                                onClicked: {
                                    // TODO: Datei-Auswahl-Dialog für Kartenrückseite
                                }
                            }
                        }

                        Label {
                            Layout.topMargin: 8
                            text: qsTr("Unterstützte Formate: PNG, JPG, GIF")
                            color: Config.StaticData.palette.secondary.col400
                            font.italic: true
                        }
                    }
                }
            }
        }
    }
}
