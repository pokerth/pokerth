import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Switch between the ranking sources PokerTH / BBC / WEC – a button box with
// an active/inactive state. The embedding page sets `current` and reacts to
// selected(community): player pages replace themselves with it
// (StackView.replace), the table overview reloads its data.
//
// Source registry and routing (entries, base URL, player page URL+props) live
// centrally in Config.Community so that they do not have to be duplicated in
// every calling page.
Rectangle {
    id: sw

    // "pokerth" | "bbc" | "wec"
    property string current: "pokerth"
    // Click on a NON-active entry (the active entry does not trigger anything).
    signal selected(string community)

    implicitWidth: segmentRow.implicitWidth + 2
    implicitHeight: 26
    radius: Config.Theme.radiusSmall
    color: Config.StaticData.palette.secondary.col600
    border.color: Config.StaticData.palette.secondary.col500
    border.width: 1

    Row {
        id: segmentRow
        anchors.fill: parent
        anchors.margins: 1

        Repeater {
            model: Config.Community.entries

            Rectangle {
                id: segment
                required property var modelData
                readonly property bool active: sw.current === modelData.key

                width: segLabel.implicitWidth + 20
                height: segmentRow.height
                radius: 3
                color: active
                       ? Config.StaticData.palette.secondary.col500
                       : (segHover.hovered
                          ? Config.Theme.withAlpha(Config.StaticData.palette.secondary.col500, 0.5)
                          : "transparent")

                AppLabel {
                    id: segLabel
                    anchors.centerIn: parent
                    text: segment.modelData.label
                    color: segment.active || segHover.hovered
                           ? Config.StaticData.palette.secondary.col100
                           : Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeCaption
                    font.bold: segment.active
                }

                HoverHandler {
                    id: segHover
                    cursorShape: segment.active ? Qt.ArrowCursor : Qt.PointingHandCursor
                }
                TapHandler {
                    onTapped: {
                        if (!segment.active)
                            sw.selected(segment.modelData.key)
                    }
                }
            }
        }
    }
}
