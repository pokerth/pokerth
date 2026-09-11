import QtQuick
import QtQuick.Effects

import "../config" as Config

// Gold highlight for the player to act: soft outer glow (layered, optional)
// + an always visible gold frame, with a calm pulse. The frame is deliberately
// kept as a separate layer-less element so that it stays visible even if the
// MultiEffect glow does not render on a system. `active` switches it on.
Item {
    id: glow
    property bool active: false
    property int borderWidth: 1

    anchors.fill: parent
    anchors.margins: -2
    z: 10
    visible: active

    // Pulse only with effects enabled – otherwise an endless animation runs that
    // makes the ENTIRE scene redraw at 60 fps.
    SequentialAnimation on opacity {
        running: Config.Theme.effectsEnabled && glow.active
        loops: Animation.Infinite
        NumberAnimation { from: 0.65; to: 1.0; duration: 750; easing.type: Easing.InOutSine }
        NumberAnimation { from: 1.0; to: 0.65; duration: 750; easing.type: Easing.InOutSine }
    }

    // Soft outer glow (layered) – pure eye candy, optional.
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 6
        border.color: "#FFD54A"
        border.width: glow.borderWidth
        layer.enabled: Config.Theme.effectsEnabled && glow.active
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#FFD700"
            shadowOpacity: 0.9
            shadowBlur: 1.0
            shadowVerticalOffset: 0
            shadowHorizontalOffset: 0
        }
    }

    // Gold frame (always visible, NO layer).
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 6
        border.color: "#CCFFD54A"
        border.width: glow.borderWidth
    }
}
