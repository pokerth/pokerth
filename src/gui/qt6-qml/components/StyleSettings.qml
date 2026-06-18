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

    // Im data-Verzeichnis (<AppDataDir>/gfx/qml/...) gefundene Stile, vom
    // SettingsManager (C++) eingelesen. Jeder Eintrag: { name, description,
    // maintainer, dir, xml, preview, previewPortrait }.
    property var tableStyles: []
    property var cardStyles: []
    // Aktuell ausgewählter Stil – initialisiert aus den Config-Keys, beim Klick
    // über den StyleProvider persistiert und sofort auf den Tisch angewendet.
    property string selectedTableStyle: ""
    property string selectedCardStyle: ""

    Component.onCompleted: {
        if (typeof SettingsManager !== "undefined" && SettingsManager) {
            tableStyles = SettingsManager.availableTableStyles()
            cardStyles = SettingsManager.availableCardDeckStyles()
            selectedTableStyle = SettingsManager.readConfigString("QmlGameTableStyle")
            selectedCardStyle = SettingsManager.readConfigString("QmlCardDeckStyle")
        }
    }

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
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: gameTableTab.availableWidth
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Verfügbare Spieltisch-Stile:")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        Repeater {
                            model: styleSettings.tableStyles
                            delegate: Component {
                                StyleCard {
                                    styleEntry: modelData
                                    selected: modelData.name === styleSettings.selectedTableStyle
                                    onClicked: {
                                        styleSettings.selectedTableStyle = modelData.name
                                        if (typeof StyleProvider !== "undefined" && StyleProvider)
                                            StyleProvider.setTableStyle(modelData.name)
                                    }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: styleSettings.tableStyles.length === 0
                            text: qsTr("Keine Spieltisch-Stile gefunden.")
                            color: Config.StaticData.palette.secondary.col400
                            font.italic: true
                            wrapMode: Text.WordWrap
                        }

                        Button {
                            Layout.topMargin: 4
                            text: qsTr("Stil hinzufügen...")
                            onClicked: {
                                // TODO: Datei-Auswahl-Dialog für Spieltisch-Stil
                            }
                        }
                    }
                }

                // Tab: Kartenstapel
                ScrollView {
                    id: cardsDeckTab
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: cardsDeckTab.availableWidth
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Verfügbare Kartenstapel-Stile:")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        Repeater {
                            model: styleSettings.cardStyles
                            delegate: Component {
                                StyleCard {
                                    styleEntry: modelData
                                    selected: modelData.name === styleSettings.selectedCardStyle
                                    onClicked: {
                                        styleSettings.selectedCardStyle = modelData.name
                                        if (typeof StyleProvider !== "undefined" && StyleProvider)
                                            StyleProvider.setCardDeckStyle(modelData.name)
                                    }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: styleSettings.cardStyles.length === 0
                            text: qsTr("Keine Kartenstapel-Stile gefunden.")
                            color: Config.StaticData.palette.secondary.col400
                            font.italic: true
                            wrapMode: Text.WordWrap
                        }

                        Button {
                            Layout.topMargin: 4
                            text: qsTr("Stil hinzufügen...")
                            onClicked: {
                                // TODO: Datei-Auswahl-Dialog für Kartenstapel-Stil
                            }
                        }
                    }
                }

                // Tab: Kartenrückseite
                ScrollView {
                    id: cardsBackgroundTab
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Kartenrückseite wählen:")
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
