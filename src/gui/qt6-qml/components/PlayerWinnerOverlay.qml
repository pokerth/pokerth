import QtQuick
import QtQuick.Effects

import "../config" as Config

// Winner highlight: golden frame around the box (does NOT cover the cards)
// + "WINNER" badge. `below`=true shows the badge below instead of above the
// box (for the topmost box in portrait, whose badge would otherwise bump into
// the top). Via `anchors` the overlay fills the box; the caller adds it as a child of the box.
Item {
    id: overlay
    property bool active: false
    property bool below: false
    property int gap: 6
    property int badgeHeight: 16
    property int badgeFontSize: 9
    property int hPadding: 12

    anchors.fill: parent
    visible: active

    // Goldener Rahmen.
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 6
        border.color: "#FFD700"
        border.width: 3
        z: 19
        layer.enabled: Config.Theme.effectsEnabled && overlay.active
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#FFD700"
            shadowOpacity: 1.0
            shadowBlur: 1.0
            shadowVerticalOffset: 0
            shadowHorizontalOffset: 0
        }
    }

    // "WINNER" badge – above or below the box.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: overlay.below ? (parent.height + overlay.gap) : (-height - overlay.gap)
        width: winnerLabel.width + overlay.hPadding
        height: overlay.badgeHeight
        radius: height / 2
        color: "#0d3d0d"
        border.color: "#FFD700"
        border.width: 1
        z: 30

        AppText {
            id: winnerLabel
            anchors.centerIn: parent
            text: qsTr("WINNER")
            color: "#FFD700"
            font.pixelSize: overlay.badgeFontSize
            font.bold: true
        }
    }
}
