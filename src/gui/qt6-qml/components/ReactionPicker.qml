import QtQuick
import QtQuick.Controls

import "../config" as Config

// Selection panel for emoji reactions – like the reaction picker of the
// web client: 90 emojis on three thematic pages (emotions /
// mood & gestures / poker & luck), 30 each in a 6 column grid.
// Between the pages you move with the arrows ‹ › (they wrap around) and – on
// touch devices – by swiping sideways; the page used last is remembered in
// the configuration.
// Visibility/position/z is set by the caller; selected emojis are reported
// via picked().
Rectangle {
    id: root

    signal picked(string emoji)

    readonly property var pages: Config.ReactionCatalog.pages
    readonly property int columns: 6
    // Dimensions as in the chat emoji picker (EmojiPicker.qml): grid 38, area 34,
    // glyph 24.
    readonly property int cell: 34
    readonly property int cellSpacing: 4
    readonly property int rows: Math.ceil(30 / columns)

    // Current page; it is remembered in the configuration (web client:
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

    // The arrows wrap around (page 3 → 1), as in the web client.
    function step(dir) {
        page = (page + dir + pages.length) % pages.length
    }

    // Arrow button of the pager.
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

        // ── The three pages (swiping sideways on touch devices) ──
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
