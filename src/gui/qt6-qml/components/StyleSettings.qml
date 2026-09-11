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

    // Styles found in the data directory (<AppDataDir>/gfx/qml/...), read in by
    // the SettingsManager (C++). Each entry: { name, description, maintainer,
    // dir, xml, preview, previewPortrait }.
    property var tableStyles: []
    property var cardStyles: []
    property var cardBackStyles: []
    // Currently selected style – initialized from the config keys, persisted via
    // the StyleProvider on a click and applied to the table right away.
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

    // Process the result of a style import (SettingsManager.import*Style):
    // refresh the list and show a possible message (warning/error).
    function handleImportResult(result) {
        if (!result || result.status === "cancelled")
            return
        refreshStyles()
        if (result.message)
            importResultPopup.openWith(qsTr("Stil hinzufügen"), result.message, qsTr("OK"))
    }

    // Export a style as .zip: the save dialog already names the target path,
    // so the success case runs silently – only errors are reported.
    function exportStyle(category, name) {
        var result = SettingsManager.exportStyle(category, name)
        if (result && result.status === "error" && result.message)
            importResultPopup.openWith(qsTr("Stil exportieren"), result.message, qsTr("OK"))
    }

    // Removing an imported style: if it was just active, switch back to
    // "default" so that the selection and the table stay consistent.
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

    // Switch the seat style of the player boxes (bet in the base INSIDE the
    // box or outside next to it). Config.SeatStyle is the only switch the
    // player boxes and the space calculation read – it therefore affects an open
    // table immediately; the config key keeps the choice across a restart.
    function applySeatStyle(variant) {
        Config.SeatStyle.variant = variant
        if (typeof SettingsManager !== "undefined" && SettingsManager)
            SettingsManager.writeConfigString("QmlSeatStyle", variant)
    }

    // Notice popup for import warnings and errors (only "OK").
    ConfirmPopup {
        id: importResultPopup
        showCancel: false
    }

    // Confirmation before removing an imported style.
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
                    // If the scrollbar is shown, it lies above the right edge of
                    // the style cards – then keep room free so that their
                    // buttons do not stick to the edge.
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

                // Tab: card back
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
