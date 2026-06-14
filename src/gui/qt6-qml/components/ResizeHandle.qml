import QtQuick
import QtQuick.Controls

import "../config" as Config

// Griff für SplitView (Lobby/GameWait): dünne Leiste mit zentralem
// Greif-Indikator. `horizontal` = true für vertikale Trennlinie zwischen
// nebeneinanderliegenden Spalten (waagerechtes Ziehen), false für eine
// waagerechte Trennlinie zwischen übereinanderliegenden Boxen.
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
