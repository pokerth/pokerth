import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: localGameSettings
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    // Manuelle Blind-Liste (wird beim Laden initialisiert)
    property var manualBlindsList: []
    property int selectedBlindIndex: -1

    function loadManualBlindsList() {
        if (!SettingsManager) return
        var raw = SettingsManager.readConfigIntList("ManualBlindsList")
        var arr = []
        for (var i = 0; i < raw.length; i++) arr.push(raw[i])
        arr.sort(function(a, b) { return a - b })
        manualBlindsList = arr
    }

    function saveManualBlindsList() {
        if (!SettingsManager) return
        SettingsManager.writeConfigIntList("ManualBlindsList", manualBlindsList)
    }

    function addBlind(val) {
        var arr = manualBlindsList.slice()
        if (arr.indexOf(val) === -1) {
            arr.push(val)
            arr.sort(function(a, b) { return a - b })
        }
        manualBlindsList = arr
        saveManualBlindsList()
    }

    function removeBlindAt(idx) {
        var arr = manualBlindsList.slice()
        arr.splice(idx, 1)
        manualBlindsList = arr
        selectedBlindIndex = -1
        saveManualBlindsList()
    }

    Component.onCompleted: loadManualBlindsList()

    ColumnLayout {
        id: localGameSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Lokales Spiel"); topGap: 4 }

        ScrollView {
            id: localScrollView
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
                            Layout.fillWidth: true
                            text: qsTr("Anzahl der Spieler:")
                            color: Config.StaticData.palette.secondary.col200
                        }

                        CustomSpinBox {
                            id: numberOfPlayers
                            from: 2
                            to: 10
                            value: SettingsManager ? SettingsManager.readConfigInt("NumberOfPlayers") : 10
                            onValueModified: {
                                if (SettingsManager) SettingsManager.writeConfigInt("NumberOfPlayers", value)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Startkapital:")
                            color: Config.StaticData.palette.secondary.col200
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

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Erster Small Blind:")
                            color: Config.StaticData.palette.secondary.col200
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
                                Layout.fillWidth: true
                                text: qsTr("Small Blind erhöhen alle:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: raiseBlindsAtHands.checked
                            }

                            CustomSpinBox {
                                id: raiseSmallBlindEveryHands
                                from: 1
                                to: 100
                                value: SettingsManager ? SettingsManager.readConfigInt("RaiseSmallBlindEveryHands") : 8
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
                                Layout.fillWidth: true
                                text: qsTr("Small Blind erhöhen alle:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: raiseBlindsAtMinutes.checked
                            }

                            CustomSpinBox {
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

                        // Manuelle Blind-Reihenfolge – Listeneditor (nur aktiv wenn ausgewählt)
                        GroupBox {
                            Layout.fillWidth: true
                            title: qsTr("Manuelle Blind-Reihenfolge")
                            enabled: manualBlindsOrder.checked

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 6

                                // Liste der eingestellten Blinds
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 120
                                    color: Config.Theme.colorBox ?? "#222"
                                    border.color: Config.StaticData.palette.secondary.col500
                                    border.width: 1
                                    radius: 3
                                    clip: true

                                    ListView {
                                        id: blindsListView
                                        anchors.fill: parent
                                        anchors.margins: 2
                                        model: localGameSettings.manualBlindsList
                                        currentIndex: localGameSettings.selectedBlindIndex
                                        clip: true

                                        delegate: ItemDelegate {
                                            width: ListView.view.width
                                            height: 28
                                            highlighted: ListView.isCurrentItem
                                            text: "$" + modelData
                                            font.pixelSize: 13
                                            onClicked: localGameSettings.selectedBlindIndex = index
                                        }
                                    }
                                }

                                // Eingabe + Hinzufügen/Löschen
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    CustomSpinBox {
                                        id: newBlindInput
                                        Layout.fillWidth: true
                                        from: firstSmallBlind.value + 1
                                        to: 20000
                                        value: firstSmallBlind.value + 5
                                    }

                                    Button {
                                        text: qsTr("Hinzufügen")
                                        onClicked: localGameSettings.addBlind(newBlindInput.value)
                                    }

                                    Button {
                                        text: qsTr("Löschen")
                                        enabled: localGameSettings.selectedBlindIndex >= 0
                                        onClicked: localGameSettings.removeBlindAt(localGameSettings.selectedBlindIndex)
                                    }
                                }

                                // Danach-Optionen
                                GroupBox {
                                    Layout.fillWidth: true
                                    title: qsTr("Danach:")

                                    ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 4

                                        ButtonGroup { id: afterMBGroup }

                                        RadioButton {
                                            id: afterMBAlwaysDouble
                                            text: qsTr("Blinds immer verdoppeln")
                                            ButtonGroup.group: afterMBGroup
                                            checked: SettingsManager ? SettingsManager.readConfigInt("AfterMBAlwaysDoubleBlinds") !== 0 : true
                                            onCheckedChanged: {
                                                if (SettingsManager && checked) {
                                                    SettingsManager.writeConfigInt("AfterMBAlwaysDoubleBlinds", 1)
                                                    SettingsManager.writeConfigInt("AfterMBAlwaysRaiseAbout", 0)
                                                    SettingsManager.writeConfigInt("AfterMBStayAtLastBlind", 0)
                                                }
                                            }
                                        }

                                        RowLayout {
                                            spacing: 6
                                            RadioButton {
                                                id: afterMBAlwaysRaise
                                                text: qsTr("Immer erhöhen um:")
                                                ButtonGroup.group: afterMBGroup
                                                checked: SettingsManager ? SettingsManager.readConfigInt("AfterMBAlwaysRaiseAbout") !== 0 : false
                                                onCheckedChanged: {
                                                    if (SettingsManager && checked) {
                                                        SettingsManager.writeConfigInt("AfterMBAlwaysDoubleBlinds", 0)
                                                        SettingsManager.writeConfigInt("AfterMBAlwaysRaiseAbout", 1)
                                                        SettingsManager.writeConfigInt("AfterMBStayAtLastBlind", 0)
                                                    }
                                                }
                                            }
                                            CustomSpinBox {
                                                id: afterMBRaiseValue
                                                from: 1
                                                to: 20000
                                                value: SettingsManager ? SettingsManager.readConfigInt("AfterMBAlwaysRaiseValue") : 5
                                                enabled: afterMBAlwaysRaise.checked
                                                onValueModified: {
                                                    if (SettingsManager) SettingsManager.writeConfigInt("AfterMBAlwaysRaiseValue", value)
                                                }
                                            }
                                            Label {
                                                text: "$"
                                                color: Config.StaticData.palette.secondary.col200
                                                enabled: afterMBAlwaysRaise.checked
                                            }
                                        }

                                        RadioButton {
                                            id: afterMBStayAtLast
                                            text: qsTr("Letzten Blind beibehalten")
                                            ButtonGroup.group: afterMBGroup
                                            checked: SettingsManager ? SettingsManager.readConfigInt("AfterMBStayAtLastBlind") !== 0 : false
                                            onCheckedChanged: {
                                                if (SettingsManager && checked) {
                                                    SettingsManager.writeConfigInt("AfterMBAlwaysDoubleBlinds", 0)
                                                    SettingsManager.writeConfigInt("AfterMBAlwaysRaiseAbout", 0)
                                                    SettingsManager.writeConfigInt("AfterMBStayAtLastBlind", 1)
                                                }
                                            }
                                        }
                                    }
                                }
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
                            Layout.fillWidth: true
                            text: qsTr("Spielgeschwindigkeit\n(1=langsam, 11=schnell):")
                            color: Config.StaticData.palette.secondary.col200
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

                        ConfigCheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Pause zwischen den Händen")
                            configKey: "PauseBetweenHands"
                            defaultChecked: false
                        }

                        ConfigCheckBox {
                            Layout.columnSpan: 2
                            text: qsTr("Spiel-Einstellungsdialog bei neuem Spiel anzeigen")
                            configKey: "ShowGameSettingsDialogOnNewGame"
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
