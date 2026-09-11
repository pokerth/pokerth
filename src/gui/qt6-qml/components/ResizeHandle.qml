import QtQuick
import QtQuick.Controls

import "../config" as Config

// Handle for SplitView (lobby/game wait): thin bar with a central grip
// indicator. `horizontal` = true for a vertical separator between columns
// sitting next to each other (horizontal dragging), false for a horizontal
// separator between boxes stacked on top of each other.
Rectangle {
    id: handle
    property bool horizontal: true

    implicitWidth: 7
    implicitHeight: 7
    color: SplitHandle.pressed
           ? Config.StaticData.palette.secondary.col500
           : (SplitHandle.hovered ? Config.StaticData.palette.secondary.col600 : "transparent")

    Behavior on color { ColorAnimation { duration: 120 } }

    // Zentraler Greif-Indikator
    Rectangle {
        anchors.centerIn: parent
        width: handle.horizontal ? 2 : 26
        height: handle.horizontal ? 26 : 2
        radius: 1
        color: Config.StaticData.palette.secondary.col400
        opacity: (SplitHandle.hovered || SplitHandle.pressed) ? 1.0 : 0.5
    }

    HoverHandler {
        cursorShape: handle.horizontal ? Qt.SplitHCursor : Qt.SplitVCursor
    }
}
