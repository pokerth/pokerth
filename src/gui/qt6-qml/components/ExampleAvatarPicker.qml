import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Galerie der mitgelieferten Beispiel-Avatare (analog zum selectAvatarDialog
// des Qt-Widgets-Clients). Die Avatare haben für die Community einen
// historischen Wert. Vor dem Öffnen openPicker() aufrufen; bei Auswahl wird
// das Signal selected(path) mit dem absoluten Dateipfad emittiert.
Popup {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Verfügbarer Platz (Fenster-Overlay). Im Kompakt-/Mobile-Modus füllt das
    // Popup nahezu den ganzen Bildschirm, sonst eine feste, zentrierte Box.
    readonly property real availW: parent ? parent.width : 460
    readonly property real availH: parent ? parent.height : 460

    width:  Config.Theme.compact ? availW - 24 : Math.min(availW - 32, 460)
    height: Config.Theme.compact ? availH - 24 : Math.min(availH - 32, 480)

    // Alle Beispiel-Avatare (people + misc), einmalig beim Öffnen geladen.
    property var allAvatars: []
    property string category: "people"

    signal selected(string path)

    function openPicker() {
        allAvatars = SettingsManager ? SettingsManager.availableExampleAvatars() : []
        open()
    }

    function avatarsFor(cat) {
        var out = []
        for (var i = 0; i < allAvatars.length; ++i) {
            if (allAvatars[i].category === cat)
                out.push(allAvatars[i])
        }
        return out
    }

    background: Rectangle {
        color: Config.StaticData.palette.secondary.col700
        border.color: Config.StaticData.palette.secondary.col500
        border.width: 1
        radius: Config.Theme.radiusMedium
    }

    contentItem: ColumnLayout {
        spacing: 12

        // Kopfzeile: Titel + Schließen-Kreuz
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 12
            spacing: 8

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Beispiel-Avatar wählen")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
                elide: Text.ElideRight
            }

            CustomButton {
                Layout.preferredWidth: Config.Theme.touchTarget
                Layout.preferredHeight: Config.Theme.touchTarget
                text: "✕"
                onClicked: root.close()
            }
        }

        // Kategorie-Tabs (Personen / Verschiedenes)
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            spacing: 8

            Repeater {
                model: [
                    { key: "people", label: qsTr("Personen") },
                    { key: "misc",   label: qsTr("Verschiedenes") }
                ]

                delegate: ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 4

                    CustomButton {
                        Layout.fillWidth: true
                        text: modelData.label
                        onClicked: root.category = modelData.key
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 2
                        radius: 1
                        visible: root.category === modelData.key
                        color: Config.StaticData.chartColors[0]
                    }
                }
            }
        }

        // Avatar-Raster
        GridView {
            id: avatarGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 4
            clip: true
            cellWidth: width / Math.max(1, Math.floor(width / 80))
            cellHeight: 80
            model: root.avatarsFor(root.category)
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Item {
                id: cell
                required property var modelData
                width: avatarGrid.cellWidth
                height: avatarGrid.cellHeight

                Rectangle {
                    anchors.centerIn: parent
                    width: 64
                    height: 64
                    radius: Config.Theme.radiusSmall
                    color: Config.StaticData.palette.secondary.col600
                    border.color: hover.hovered
                                  ? Config.StaticData.chartColors[0]
                                  : Config.StaticData.palette.secondary.col500
                    border.width: hover.hovered ? 2 : 1
                    clip: true

                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: cell.modelData.url
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        asynchronous: true
                    }

                    HoverHandler {
                        id: hover
                    }

                    TapHandler {
                        onTapped: {
                            root.selected(cell.modelData.path)
                            root.close()
                        }
                    }
                }
            }
        }

        // Fußzeile: Abbrechen
        CustomButton {
            text: qsTr("Abbrechen")
            Layout.alignment: Qt.AlignRight
            Layout.bottomMargin: 16
            Layout.rightMargin: 16
            onClicked: root.close()
        }
    }
}
