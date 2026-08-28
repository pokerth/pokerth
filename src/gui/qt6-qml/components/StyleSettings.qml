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
    property var cardBackStyles: []
    // Aktuell ausgewählter Stil – initialisiert aus den Config-Keys, beim Klick
    // über den StyleProvider persistiert und sofort auf den Tisch angewendet.
    property string selectedTableStyle: ""
    property string selectedCardStyle: ""
    property string selectedCardBackStyle: ""

    Component.onCompleted: {
        if (typeof SettingsManager !== "undefined" && SettingsManager) {
            refreshStyles()
            selectedTableStyle = SettingsManager.readConfigString("QmlGameTableStyle")
            selectedCardStyle = SettingsManager.readConfigString("QmlCardDeckStyle")
            selectedCardBackStyle = SettingsManager.readConfigString("QmlCardBackStyle")
        }
    }

    function refreshStyles() {
        tableStyles = SettingsManager.availableTableStyles()
        cardStyles = SettingsManager.availableCardDeckStyles()
        cardBackStyles = SettingsManager.availableCardBackStyles()
    }

    // Ergebnis eines Stil-Imports (SettingsManager.import*Style) verarbeiten:
    // Liste auffrischen und eine evtl. Meldung (Warnung/Fehler) anzeigen.
    function handleImportResult(result) {
        if (!result || result.status === "cancelled")
            return
        refreshStyles()
        if (result.message)
            importResultPopup.openWith(qsTr("Stil hinzufügen"), result.message, qsTr("OK"))
    }

    // Stil als .zip exportieren: der Speichern-Dialog nennt bereits den Zielpfad,
    // daher läuft der Erfolgsfall still – nur Fehler werden gemeldet.
    function exportStyle(category, name) {
        var result = SettingsManager.exportStyle(category, name)
        if (result && result.status === "error" && result.message)
            importResultPopup.openWith(qsTr("Stil exportieren"), result.message, qsTr("OK"))
    }

    // Entfernen eines importierten Stils: war er gerade aktiv, zurück auf
    // "default" schalten, damit Auswahl und Tisch konsistent bleiben.
    function removeStyle(category, name) {
        if (!SettingsManager.removeUserStyle(category, name))
            return
        if (typeof StyleProvider !== "undefined" && StyleProvider) {
            if (category === "table" && selectedTableStyle === name) {
                selectedTableStyle = "default"
                StyleProvider.setTableStyle("default")
            } else if (category === "cards" && selectedCardStyle === name) {
                selectedCardStyle = "default"
                StyleProvider.setCardDeckStyle("default")
            } else if (category === "backside" && selectedCardBackStyle === name) {
                selectedCardBackStyle = "default"
                StyleProvider.setCardBackStyle("default")
            }
        }
        refreshStyles()
    }

    // Sitz-Stil der Spielerboxen umschalten (Einsatz im Sockel INNERHALB der
    // Box oder außerhalb daneben). Config.SeatStyle ist der einzige Schalter,
    // den Spielerboxen und Platzberechnung lesen – wirkt daher sofort auf einen
    // offenen Tisch; der Config-Key hält die Wahl über den Neustart.
    function applySeatStyle(variant) {
        Config.SeatStyle.variant = variant
        if (typeof SettingsManager !== "undefined" && SettingsManager)
            SettingsManager.writeConfigString("QmlSeatStyle", variant)
    }

    // Hinweis-Popup für Import-Warnungen und -Fehler (nur "OK").
    ConfirmPopup {
        id: importResultPopup
        showCancel: false
    }

    // Rückfrage vor dem Entfernen eines importierten Stils.
    ConfirmPopup {
        id: removeConfirmPopup
        property string category: ""
        property string styleName: ""
        onConfirmed: styleSettings.removeStyle(category, styleName)

        function askFor(cat, name, description) {
            category = cat
            styleName = name
            openWith(qsTr("Stil entfernen"),
                     qsTr("Den Stil \"%1\" wirklich entfernen?").arg(description || name),
                     qsTr("Entfernen"))
        }
    }

    ColumnLayout {
        id: styleSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Stil") }

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
                    // Ist die Scrollleiste eingeblendet, liegt sie über dem
                    // rechten Rand der Stil-Karten – dann Platz freihalten,
                    // damit deren Buttons nicht am Rand kleben.
                    readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

                    ColumnLayout {
                        width: gameTableTab.availableWidth - gameTableTab.scrollBarSpace
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Einsatzanzeige:")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        ButtonGroup { id: seatStyleGroup }

                        RadioButton {
                            text: qsTr("Einsatz in der Spielerbox")
                            checked: Config.SeatStyle.variant === "inset"
                            ButtonGroup.group: seatStyleGroup
                            onClicked: styleSettings.applySeatStyle("inset")
                        }

                        RadioButton {
                            text: qsTr("Einsatz neben der Spielerbox")
                            checked: Config.SeatStyle.variant === "classic"
                            ButtonGroup.group: seatStyleGroup
                            onClicked: styleSettings.applySeatStyle("classic")
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 8
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
                                    onRemoveRequested: removeConfirmPopup.askFor(
                                                           "table", modelData.name, modelData.description)
                                    onExportRequested: styleSettings.exportStyle("table", modelData.name)
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
                            onClicked: styleSettings.handleImportResult(
                                           SettingsManager.importTableStyle())
                        }
                    }
                }

                // Tab: Kartenstapel
                ScrollView {
                    id: cardsDeckTab
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

                    ColumnLayout {
                        width: cardsDeckTab.availableWidth - cardsDeckTab.scrollBarSpace
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
                                    forceLandscape: true
                                    selected: modelData.name === styleSettings.selectedCardStyle
                                    onClicked: {
                                        styleSettings.selectedCardStyle = modelData.name
                                        if (typeof StyleProvider !== "undefined" && StyleProvider)
                                            StyleProvider.setCardDeckStyle(modelData.name)
                                    }
                                    onRemoveRequested: removeConfirmPopup.askFor(
                                                           "cards", modelData.name, modelData.description)
                                    onExportRequested: styleSettings.exportStyle("cards", modelData.name)
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
                            onClicked: styleSettings.handleImportResult(
                                           SettingsManager.importCardDeckStyle())
                        }
                    }
                }

                // Tab: Kartenrückseite
                ScrollView {
                    id: cardsBackgroundTab
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    readonly property real scrollBarSpace: ScrollBar.vertical.visible ? 12 : 0

                    ColumnLayout {
                        width: cardsBackgroundTab.availableWidth - cardsBackgroundTab.scrollBarSpace
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Verfügbare Kartenrückseiten:")
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        Repeater {
                            model: styleSettings.cardBackStyles
                            delegate: Component {
                                StyleCard {
                                    styleEntry: modelData
                                    forceLandscape: true
                                    selected: modelData.name === styleSettings.selectedCardBackStyle
                                    onClicked: {
                                        styleSettings.selectedCardBackStyle = modelData.name
                                        if (typeof StyleProvider !== "undefined" && StyleProvider)
                                            StyleProvider.setCardBackStyle(modelData.name)
                                    }
                                    onRemoveRequested: removeConfirmPopup.askFor(
                                                           "backside", modelData.name, modelData.description)
                                    onExportRequested: styleSettings.exportStyle("backside", modelData.name)
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: styleSettings.cardBackStyles.length === 0
                            text: qsTr("Keine Kartenrückseiten gefunden.")
                            color: Config.StaticData.palette.secondary.col400
                            font.italic: true
                            wrapMode: Text.WordWrap
                        }

                        Button {
                            Layout.topMargin: 4
                            text: qsTr("Stil hinzufügen...")
                            onClicked: styleSettings.handleImportResult(
                                           SettingsManager.importCardBackStyle())
                        }
                    }
                }
            }
        }
    }
}
