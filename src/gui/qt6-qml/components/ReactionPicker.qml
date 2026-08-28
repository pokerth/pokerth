import QtQuick
import QtQuick.Controls

import "../config" as Config

// Auswahl-Panel für Emoji-Reaktionen – wie der Reaction-Picker des
// Web-Clients: 90 Emojis auf drei thematischen Seiten (Emotionen /
// Stimmung & Gesten / Poker & Glück), je 30 in einem 6-spaltigen Raster.
// Zwischen den Seiten führen die Pfeile ‹ › (sie laufen um) und – auf
// Touch-Geräten – das seitliche Wischen; die zuletzt benutzte Seite wird in
// der Konfiguration gemerkt.
// Sichtbarkeit/Position/z setzt der Aufrufer; ausgewählte Emojis werden
// über picked() gemeldet.
Rectangle {
    id: root

    signal picked(string emoji)

    readonly property var pages: Config.ReactionCatalog.pages
    readonly property int columns: 6
    // Maße wie im Chat-Emoji-Picker (EmojiPicker.qml): Raster 38, Fläche 34,
    // Glyphe 24.
    readonly property int cell: 34
    readonly property int cellSpacing: 4
    readonly property int rows: Math.ceil(30 / columns)

    // Aktuelle Seite; wird in der Konfiguration gemerkt (Web-Client:
    // localStorage "pth_react_page").
    property int page: 0
    onPageChanged: {
        if (pager.currentIndex !== page)
            pager.currentIndex = page
        if (typeof SettingsManager !== "undefined" && SettingsManager)
            SettingsManager.writeConfigInt("ReactionPickerPage", page)
    }
    Component.onCompleted: {
        var saved = (typeof SettingsManager !== "undefined" && SettingsManager)
                    ? SettingsManager.readConfigInt("ReactionPickerPage") : 0
        page = Math.max(0, Math.min(pages.length - 1, saved))
    }

    // Die Pfeile laufen um (Seite 3 → 1), wie im Web-Client.
    function step(dir) {
        page = (page + dir + pages.length) % pages.length
    }

    // Pfeil-Schaltfläche des Pagers.
    component PagerArrow: Item {
        id: arrowRoot
        property string label: ""
        property int dir: 1
        width: 20; height: 22

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: arrowArea.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.06)
            border.color: Qt.rgba(1, 1, 1, 0.14)
            border.width: 1
        }
        Text {
            anchors.centerIn: parent
            text: arrowRoot.label
            color: "#FFFFFF"
            font.pixelSize: 14
        }
        MouseArea {
            id: arrowArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.step(arrowRoot.dir)
        }
    }

    width: content.width + 16
    height: content.height + 16
    radius: 8
    color: Qt.rgba(0, 0, 0, 0.88)
    border.color: Qt.rgba(1, 1, 1, 0.12)
    border.width: 1

    Column {
        id: content
        anchors.centerIn: parent
        spacing: 4
        width: root.columns * root.cell + (root.columns - 1) * root.cellSpacing

        // ── Seiten-Pager: ‹ 😀 1/3 › ──
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            PagerArrow { label: "‹"; dir: -1 }

            Rectangle {
                width: pageLabel.implicitWidth + 12
                height: 22
                radius: 4
                color: Qt.rgba(1, 1, 1, 0.06)
                border.color: Qt.rgba(1, 1, 1, 0.14)
                border.width: 1

                Text {
                    id: pageLabel
                    anchors.centerIn: parent
                    text: Config.ReactionCatalog.pageIcons[root.page]
                          + "  " + (root.page + 1) + "/" + root.pages.length
                    font.family: Config.StaticData.emojiFamily
                    font.pixelSize: 12
                    color: "#FFFFFF"
                }
            }

            PagerArrow { label: "›"; dir: 1 }
        }

        // ── Die drei Seiten (seitliches Wischen auf Touch-Geräten) ──
        SwipeView {
            id: pager
            width: parent.width
            height: root.rows * root.cell + (root.rows - 1) * root.cellSpacing
            clip: true
            onCurrentIndexChanged: root.page = currentIndex

            Repeater {
                model: root.pages

                delegate: Item {
                    id: pageItem
                    required property var modelData

                    Grid {
                        anchors.centerIn: parent
                        columns: root.columns
                        spacing: root.cellSpacing

                        Repeater {
                            model: pageItem.modelData

                            delegate: Rectangle {
                                id: reactCell
                                required property string modelData
                                width: root.cell; height: root.cell
                                radius: 6
                                color: reactArea.containsPress ? Qt.rgba(1, 1, 1, 0.25)
                                     : reactArea.containsMouse ? Qt.rgba(1, 1, 1, 0.12)
                                     : "transparent"
                                scale: reactArea.containsMouse && !reactArea.containsPress ? 1.15 : 1.0
                                Behavior on scale { NumberAnimation { duration: 100 } }

                                Text {
                                    anchors.centerIn: parent
                                    text: reactCell.modelData
                                    font.family: Config.StaticData.emojiFamily
                                    font.pixelSize: 24
                                }
                                MouseArea {
                                    id: reactArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.picked(reactCell.modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
