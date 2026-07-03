import QtQuick

import "../config" as Config

// Aktions-Badge (Fold/Check/Call/Bet/Raise/All-In) der Spielerboxen. Farbe und
// Rand richten sich nach der Aktion (gleiche Logik wie die Action-Buttons, nur
// dunkler). Poppt beim Erscheinen und bei jedem Aktionswechsel kurz auf.
// Sichtbarkeit/Position setzt der Aufrufer.
Rectangle {
    id: badge
    property int action: 0
    property string label: ""
    property int hPadding: 14
    property int fontPixelSize: 12

    width: badgeText.width + hPadding
    height: 18
    radius: 9
    color: Config.Theme.actionBadgeColor(action)
    border.color: Config.Theme.actionBadgeBorder(action)
    border.width: 1
    transformOrigin: Item.Center
    Behavior on color { ColorAnimation { duration: 200 } }
    Behavior on border.color { ColorAnimation { duration: 200 } }

    onVisibleChanged: if (visible) pop.restart()
    onActionChanged: if (visible) pop.restart()
    SequentialAnimation {
        id: pop
        NumberAnimation { target: badge; property: "scale"; from: 0.6; to: 1.12; duration: 110; easing.type: Easing.OutQuad }
        NumberAnimation { target: badge; property: "scale"; to: 1.0; duration: 120; easing.type: Easing.OutBack }
    }

    AppText {
        id: badgeText
        anchors.centerIn: parent
        text: badge.label
        color: "#eaf1ff"
        font.pixelSize: badge.fontPixelSize
        font.bold: true
    }
}
