import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: localGamePagePage
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Gibt eine lesbare Zusammenfassung der aktuellen Blinds-Einstellungen zurück
    function blindsSummary() {
        if (!SettingsManager) return ""
        var fsb = SettingsManager.readConfigInt("FirstSmallBlind")
        var atHands = SettingsManager.readConfigInt("RaiseBlindsAtHands") !== 0
        var atMins  = SettingsManager.readConfigInt("RaiseBlindsAtMinutes") !== 0
        var alwaysDouble = SettingsManager.readConfigInt("AlwaysDoubleBlinds") !== 0
        var manual = SettingsManager.readConfigInt("ManualBlindsOrder") !== 0

        var raise = ""
        if (atHands) {
            var h = SettingsManager.readConfigInt("RaiseSmallBlindEveryHands")
            raise = qsTr("alle %1 Hände").arg(h)
        } else if (atMins) {
            var m = SettingsManager.readConfigInt("RaiseSmallBlindEveryMinutes")
            raise = qsTr("alle %1 Minuten").arg(m)
        }
        var mode = alwaysDouble ? qsTr("verdoppeln") : (manual ? qsTr("manuell") : "")
        return qsTr("Small Blind: $%1  •  Erhöhen %2  •  %3").arg(fsb).arg(raise).arg(mode)
    }

    Flickable {
        id: scroller
        anchors.fill: parent
        contentWidth: scroller.width
        contentHeight: scrollContent.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ScrollBar.vertical: ScrollBar {
            // AlwaysOff statt AsNeeded: AsNeeded blendet die Bar bei jeder
            // contentHeight-Änderung transient ein (mehrsekündiges Fade) –
            // sichtbar als kurz aufblitzende Scrollbar beim Seitenaufbau,
            // obwohl nichts zu scrollen ist.
            policy: scroller.contentHeight > scroller.height + 1
                    ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        }

        Item {
            id: scrollContent
            width: scroller.width
            // Min-Höhe = Viewport (damit fillHeight-Spacer das vertikale
            // Zentrieren halten), aber wachsen sobald der Layout-Inhalt
            // mehr Platz braucht → dann scrollt der Flickable.
            height: Math.max(scroller.height,
                             pageColumn.implicitHeight + Config.Theme.margin * 2)

    ColumnLayout {
        id: pageColumn
        anchors.fill: parent
        anchors.margins: Config.Theme.margin
        spacing: Config.Theme.spacing

        AppLabel {
            text: qsTr("Lokales Spiel")
            font.bold: true
            font.pixelSize: 16
            color: Config.StaticData.palette.secondary.col200
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Config.StaticData.palette.secondary.col500
        }

        Item { Layout.fillHeight: true }

        // Einstellungs-Karte (entspricht dem Qt-Widgets newGameDialog)
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Lokale Spiel-Einstellungen")

            GridLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                columns: 2
                columnSpacing: 12
                rowSpacing: 10

                // --- Anzahl Spieler ---
                Label {
                    text: qsTr("Anzahl der Spieler:")
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }
                CustomSpinBox {
                    id: numPlayers
                    from: 2
                    to: 10
                    value: SettingsManager ? SettingsManager.readConfigInt("NumberOfPlayers") : 10
                    onValueModified: {
                        if (SettingsManager) SettingsManager.writeConfigInt("NumberOfPlayers", value)
                    }
                }

                // --- Startkapital ---
                Label {
                    text: qsTr("Startkapital:")
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }
                CustomSpinBox {
                    id: startCash
                    from: 1000
                    to: 1000000
                    stepSize: 50
                    value: SettingsManager ? SettingsManager.readConfigInt("StartCash") : 5000
                    onValueModified: {
                        if (SettingsManager) SettingsManager.writeConfigInt("StartCash", value)
                    }
                }

                // --- Blinds ---
                GroupBox {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    title: qsTr("Blinds")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        ButtonGroup { id: blindsChoiceGroup }

                        RadioButton {
                            id: useSavedBlinds
                            text: qsTr("Gespeicherte Blinds-Einstellungen verwenden")
                            ButtonGroup.group: blindsChoiceGroup
                            checked: true
                        }

                        // Zusammenfassung der aktuell gespeicherten Blinds
                        Label {
                            Layout.leftMargin: 28
                            text: localGamePagePage.blindsSummary()
                            color: Config.StaticData.palette.secondary.col400
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            visible: useSavedBlinds.checked
                        }

                        RadioButton {
                            id: changeBlinds
                            text: qsTr("Blinds-Einstellungen ändern …")
                            ButtonGroup.group: blindsChoiceGroup
                        }

                        // Inline-Blinds-Editor (nur sichtbar wenn "ändern" gewählt)
                        GridLayout {
                            Layout.leftMargin: 28
                            Layout.fillWidth: true
                            visible: changeBlinds.checked
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8

                            Label {
                                text: qsTr("Erster Small Blind:")
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                            CustomSpinBox {
                                id: firstSmallBlind
                                from: 5
                                to: 20000
                                stepSize: 5
                                value: SettingsManager ? SettingsManager.readConfigInt("FirstSmallBlind") : 10
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("FirstSmallBlind", value)
                                }
                            }

                            Label {
                                text: qsTr("Small Blind erhöhen:")
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }

                            ColumnLayout {
                                spacing: 4

                                ButtonGroup { id: raiseAtGroup }

                                RowLayout {
                                    RadioButton {
                                        id: raiseAtHands
                                        text: qsTr("alle")
                                        ButtonGroup.group: raiseAtGroup
                                        checked: SettingsManager ? SettingsManager.readConfigInt("RaiseBlindsAtHands") !== 0 : true
                                        onCheckedChanged: {
                                            if (SettingsManager && checked) {
                                                SettingsManager.writeConfigInt("RaiseBlindsAtHands", 1)
                                                SettingsManager.writeConfigInt("RaiseBlindsAtMinutes", 0)
                                            }
                                        }
                                    }
                                    CustomSpinBox {
                                        from: 1; to: 100
                                        value: SettingsManager ? SettingsManager.readConfigInt("RaiseSmallBlindEveryHands") : 8
                                        enabled: raiseAtHands.checked
                                        onValueModified: {
                                            if (SettingsManager) SettingsManager.writeConfigInt("RaiseSmallBlindEveryHands", value)
                                        }
                                    }
                                    Label { text: qsTr("Hände"); color: Config.StaticData.palette.secondary.col200; enabled: raiseAtHands.checked }
                                }

                                RowLayout {
                                    RadioButton {
                                        id: raiseAtMinutes
                                        text: qsTr("alle")
                                        ButtonGroup.group: raiseAtGroup
                                        checked: SettingsManager ? SettingsManager.readConfigInt("RaiseBlindsAtMinutes") !== 0 : false
                                        onCheckedChanged: {
                                            if (SettingsManager && checked) {
                                                SettingsManager.writeConfigInt("RaiseBlindsAtHands", 0)
                                                SettingsManager.writeConfigInt("RaiseBlindsAtMinutes", 1)
                                            }
                                        }
                                    }
                                    CustomSpinBox {
                                        from: 1; to: 60
                                        value: SettingsManager ? SettingsManager.readConfigInt("RaiseSmallBlindEveryMinutes") : 5
                                        enabled: raiseAtMinutes.checked
                                        onValueModified: {
                                            if (SettingsManager) SettingsManager.writeConfigInt("RaiseSmallBlindEveryMinutes", value)
                                        }
                                    }
                                    Label { text: qsTr("Minuten"); color: Config.StaticData.palette.secondary.col200; enabled: raiseAtMinutes.checked }
                                }
                            }

                            Label {
                                text: qsTr("Erhöhungsmodus:")
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }

                            ColumnLayout {
                                spacing: 4
                                ButtonGroup { id: raiseModeGroup }

                                RadioButton {
                                    text: qsTr("Blinds immer verdoppeln")
                                    ButtonGroup.group: raiseModeGroup
                                    checked: SettingsManager ? SettingsManager.readConfigInt("AlwaysDoubleBlinds") !== 0 : true
                                    onCheckedChanged: {
                                        if (SettingsManager && checked) {
                                            SettingsManager.writeConfigInt("AlwaysDoubleBlinds", 1)
                                            SettingsManager.writeConfigInt("ManualBlindsOrder", 0)
                                        }
                                    }
                                }
                                RadioButton {
                                    text: qsTr("Manuelle Blind-Reihenfolge")
                                    ButtonGroup.group: raiseModeGroup
                                    checked: SettingsManager ? SettingsManager.readConfigInt("ManualBlindsOrder") !== 0 : false
                                    onCheckedChanged: {
                                        if (SettingsManager && checked) {
                                            SettingsManager.writeConfigInt("AlwaysDoubleBlinds", 0)
                                            SettingsManager.writeConfigInt("ManualBlindsOrder", 1)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // --- Spielgeschwindigkeit ---
                Label {
                    text: qsTr("Spielgeschwindigkeit\n(1=langsam, 11=schnell):")
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }
                CustomSpinBox {
                    id: gameSpeed
                    from: 1
                    to: 11
                    value: SettingsManager ? SettingsManager.readConfigInt("GameSpeed") : 4
                    onValueModified: {
                        if (SettingsManager) SettingsManager.writeConfigInt("GameSpeed", value)
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: Config.Theme.spacing

            CustomButton {
                text: qsTr("Abbrechen")
                Layout.fillWidth: true
                onClicked: mainStackView.pop()
            }

            CustomButton {
                text: qsTr("Spiel starten")
                Layout.fillWidth: true
                onClicked: {
                    // Doppelklick-Schutz: sonst liegen zwei GamePages im Stack.
                    if (mainStackView.currentItem
                            && mainStackView.currentItem.objectName === "gamePage")
                        return
                    GameTable.startLocalGame()
                    mainStackView.push("GamePage.qml")
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
        }
    }
}
