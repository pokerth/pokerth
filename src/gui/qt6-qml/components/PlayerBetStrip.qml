import QtQuick

import "../config" as Config

// Bet base INSIDE a player box (seat style "inset"). Sits flush at the lower
// box edge, 1 px inside the frame of PlayerBoxBackground, so that its border
// (clearly visible with table styles using <PlayerBoxAccent>) is not
// covered.
//
// The base only unfolds while the player has actually bet something
// (`open`); the box grows by its height. The SPACE for it is reserved
// permanently at the table (see tableZone.betStripH in GamePage) – so folding
// it in and out shifts neither the neighbouring boxes nor the table scaling.
// clip, so that the content does not stick out of the box during the animation.
Item {
    id: strip

    property int amount: 0
    // Unfolded? Controls the height – the box follows it.
    property bool open: false

    // Full base height in the unfolded state – exactly the height reserved for
    // it at the table. The 1 px indent from the box edge sits in the inner
    // rectangle so that the height calculation of the box stays clean.
    readonly property int openHeight: Config.SeatStyle.betStripHeight

    height: open ? openHeight : 0
    clip: true
    Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutQuad } }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 1
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        height: strip.openHeight - 1
        // Round only at the bottom – at the top the base joins the box body flush.
        bottomLeftRadius: 5
        bottomRightRadius: 5
        color: Qt.rgba(0, 0, 0, 0.26)

        // Separator to the info area above.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: Qt.rgba(1, 1, 1, 0.10)
        }

        BetChip {
            anchors.centerIn: parent
            visible: strip.amount > 0
            amount: strip.amount
            iconSize: 15
            fontSize: 13
            textColor: "#eff1f5"
        }
    }
}
